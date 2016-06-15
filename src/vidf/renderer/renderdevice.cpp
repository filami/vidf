#include "pch.h"
#include "renderdevice.h"
#include "rendercontext.h"
#include "swapchain.h"

namespace vidf
{



	RenderDevice::RenderDevice()
	{
	}


	RenderDevice::~RenderDevice()
	{
	}



	RenderDevicePtr RenderDevice::Create(const RenderDeviceDesc& desc)
	{
		RenderDevicePtr device = RenderDevicePtr(new RenderDevice());

		device->QueryLayers();
		if (!device->CreateInstance(desc))
			return RenderDevicePtr();
		if (!device->CreatePhysicalDevice())
			return RenderDevicePtr();
		device->QueryExtensions();
		if (!device->CreateDevice(desc))
			return RenderDevicePtr();

		GetVkExtDebugMarker(device);

		return device;
	}



	SwapChainPtr RenderDevice::CreateSwapChain(const SwapChainDesc& desc)
	{
		SwapChainPtr swapChain(new SwapChain(shared_from_this()));
		if (!swapChain->Initialize(desc))
			return SwapChainPtr();
		return swapChain;
	}



	RenderContextPtr RenderDevice::CreateRenderContext()
	{
		RenderContextPtr context(new RenderContext(shared_from_this()));
		if (!context->Initialize())
			return RenderContextPtr();
		return context;
	}



	void RenderDevice::QueryLayers()
	{
		uint32_t count = 0;
		std::vector<VkLayerProperties> layersAvailable(count);
		std::vector<VkExtensionProperties> extensions;

		vkEnumerateInstanceLayerProperties(&count, nullptr);
		layersAvailable.resize(count);
		vkEnumerateInstanceLayerProperties(&count, layersAvailable.data());
		for (auto& layerProp : layersAvailable)
		{
			if (strcmp(layerProp.layerName, "VK_LAYER_LUNARG_standard_validation") == 0)
				layers.validationLayer = true;
		}
	}



	void RenderDevice::QueryExtensions()
	{
		uint32_t count = 0;
		std::vector<VkExtensionProperties> extensionAvailable;

		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, nullptr);
		extensionAvailable.resize(count);
		vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &count, extensionAvailable.data());
		for (auto& extProp : extensionAvailable)
		{
			if (strcmp(extProp.extensionName, VK_EXT_DEBUG_MARKER_EXTENSION_NAME) == 0)
				extensions.debugMarkers = true;
		}
	}



	bool RenderDevice::CreateInstance(const RenderDeviceDesc& desc)
	{
		VkResult result;

		VkApplicationInfo appInfo;
		ZeroStruct(appInfo);
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = desc.name;
		appInfo.pEngineName = desc.name;
		appInfo.apiVersion = VK_API_VERSION_1_0;

		std::vector<const char*> enabledLayers;
		std::vector<const char*> enabledExtensions =
		{
			VK_KHR_SURFACE_EXTENSION_NAME,
			VK_KHR_WIN32_SURFACE_EXTENSION_NAME
		};

		if (desc.enableValidation && layers.validationLayer)
		{
			enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			enabledLayers.push_back("VK_LAYER_LUNARG_standard_validation");
		}

		VkInstanceCreateInfo instanceCreateInfo;
		ZeroStruct(instanceCreateInfo);
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
		instanceCreateInfo.enabledLayerCount = enabledLayers.size();
		instanceCreateInfo.ppEnabledLayerNames = enabledLayers.data();

		VK_VERIFY_RETURN(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

		return true;
	}



	bool RenderDevice::CreatePhysicalDevice()
	{
		uint32_t gpuCount = 0;

		VK_VERIFY_RETURN(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));

		assert(gpuCount > 0);
		std::vector<VkPhysicalDevice> physicalDevices(gpuCount);
		VK_VERIFY_RETURN(vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data()));

		physicalDevice = physicalDevices[0];
		uint32_t queueCount;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, NULL);
		assert(queueCount >= 1);

		std::vector<VkQueueFamilyProperties> queueProps;
		queueProps.resize(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());

		for (queueIndex = 0; queueIndex < queueCount; queueIndex++)
		{
			if (queueProps[queueIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				break;
		}
		assert(queueIndex < queueCount);

		return true;
	}



	bool RenderDevice::CreateDevice(const RenderDeviceDesc& desc)
	{
		std::vector<const char*> enabledLayers;
		std::vector<const char*> enabledExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		if (extensions.debugMarkers)
			enabledExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
		if (desc.enableValidation && layers.validationLayer)
			enabledLayers.push_back("VK_LAYER_LUNARG_standard_validation");

		std::array<float, 1> queuePriorities = { 0.0f };
		VkDeviceQueueCreateInfo queueCreateInfo;
		ZeroStruct(queueCreateInfo);
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueIndex;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = queuePriorities.data();

		VkDeviceCreateInfo deviceCreateInfo;
		ZeroStruct(deviceCreateInfo);
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.enabledExtensionCount = (uint32_t)enabledExtensions.size();
		deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
		deviceCreateInfo.enabledLayerCount = (uint32_t)enabledLayers.size();
		deviceCreateInfo.ppEnabledLayerNames = enabledLayers.data();

		VK_VERIFY_RETURN(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));

		VkCommandPoolCreateInfo commandPoolCreateInfo;
		ZeroStruct(commandPoolCreateInfo);
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = queueIndex;
		VK_VERIFY_RETURN(vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool));

		VkCommandBufferAllocateInfo commandBufferAllocationInfo;
		ZeroStruct(commandBufferAllocationInfo);
		commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocationInfo.commandPool = commandPool;
		commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocationInfo.commandBufferCount = 1;
		VK_VERIFY_RETURN(vkAllocateCommandBuffers(device, &commandBufferAllocationInfo, &setupCmdBuffer));

		vkGetDeviceQueue(device, queueIndex, 0, &queue);

		return true;
	}



	bool RenderDevice::SubmitContext(RenderContextPtr context)
	{
		VkCommandBuffer cmdBuffer = context->GetDrawCommandBuffer();
		VkSubmitInfo submitInfo;
		ZeroStruct(submitInfo);
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &cmdBuffer;
		VK_VERIFY_RETURN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	}



}
