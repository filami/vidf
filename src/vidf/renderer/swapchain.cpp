#include "pch.h"
#include "renderer/swapchain.h"
#include "renderer/common.h"

namespace vidf
{


	SwapChain::SwapChain()
	{
	}



	SwapChain::~SwapChain()
	{
	}



	bool SwapChain::Connect(VkInstance _instance, VkPhysicalDevice _physicalDevice, VkDevice _device, VkCommandBuffer _setupCmdBuffer, VkQueue _queue)
	{
		instance = _instance;
		physicalDevice = _physicalDevice;
		device = _device;
		setupCmdBuffer = _setupCmdBuffer;
		queue = _queue;
		GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceSupportKHR);
		GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
		GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfaceFormatsKHR);
		GET_INSTANCE_PROC_ADDR(instance, GetPhysicalDeviceSurfacePresentModesKHR);
		GET_DEVICE_PROC_ADDR(device, CreateSwapchainKHR);
		GET_DEVICE_PROC_ADDR(device, DestroySwapchainKHR);
		GET_DEVICE_PROC_ADDR(device, GetSwapchainImagesKHR);
		GET_DEVICE_PROC_ADDR(device, AcquireNextImageKHR);
		GET_DEVICE_PROC_ADDR(device, QueuePresentKHR);
		return true;
	}



	bool SwapChain::CreateSurface(const SwapChainDesc& desc)
	{
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
		ZeroStruct(surfaceCreateInfo);
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = GetModuleHandle(0);
		surfaceCreateInfo.hwnd = (HWND)desc.windowHandle;

		VK_VERIFY_RETURN(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface));

		return true;
	}



	void SwapChain::QueryColorFormat()
	{
		uint32_t formatCount = 0;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
		surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());

		if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			colorFormat = VK_FORMAT_B8G8R8_UNORM;
		else
			colorFormat = surfaceFormats[0].format;
		colorSpace = surfaceFormats[0].colorSpace;
	}



	bool SwapChain::QueryQueueNodeIndex()
	{
		uint32_t queueCount;
		std::vector<VkQueueFamilyProperties> queueProps;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, nullptr);
		queueProps.resize(queueCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueCount, queueProps.data());
		std::vector<VkBool32> supportsPresent(queueCount);
		for (uint32_t i = 0; i < queueCount; i++)
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &supportsPresent[i]);

		uint32_t graphicsQueueNodeIndex = UINT32_MAX;
		uint32_t presentQueueNodeIndex = UINT32_MAX;
		for (uint32_t i = 0; i < queueCount; i++)
		{
			if ((queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
			{
				if (graphicsQueueNodeIndex == UINT32_MAX)
				{
					graphicsQueueNodeIndex = i;
				}

				if (supportsPresent[i] == VK_TRUE)
				{
					graphicsQueueNodeIndex = i;
					presentQueueNodeIndex = i;
					break;
				}
			}
		}
		if (presentQueueNodeIndex == UINT32_MAX)
		{
			for (uint32_t i = 0; i < queueCount; ++i)
			{
				if (supportsPresent[i] == VK_TRUE)
				{
					presentQueueNodeIndex = i;
					break;
				}
			}
		}

		if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
		{
			// vkTools::exitFatal("Could not find a graphics and/or presenting queue!", "Fatal error");
			return false;
		}

		if (graphicsQueueNodeIndex != presentQueueNodeIndex)
		{
			// vkTools::exitFatal("Separate graphics and presenting queues are not supported yet!", "Fatal error");
			return false;
		}

		queueNodeIndex = graphicsQueueNodeIndex;

		return true;
	}



	bool SwapChain::InitSwapChain(const SwapChainDesc& desc)
	{
		uint32_t presentModeCount;
		VkSurfaceCapabilitiesKHR surfCaps;
		std::vector<VkPresentModeKHR> presentModes;

		VK_VERIFY_RETURN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCaps));
		VK_VERIFY_RETURN(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));
		presentModes.resize(presentModeCount);
		VK_VERIFY_RETURN(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

		VkExtent2D swapchainExtent = surfCaps.currentExtent;
		if (swapchainExtent.width == -1)
		{
			swapchainExtent.width = desc.width;
			swapchainExtent.height = desc.height;
		}

		VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
		for (uint32_t i = 0; i < presentModeCount; ++i)
		{
			if (presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
			{
				swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
				break;
			}
		}

		uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
		if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
			desiredNumberOfSwapchainImages = surfCaps.maxImageCount;

		VkSurfaceTransformFlagBitsKHR preTransform = surfCaps.currentTransform;
		if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
			preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

		VkSwapchainCreateInfoKHR swapChainInfo;
		ZeroStruct(swapChainInfo);
		swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swapChainInfo.surface = surface;
		swapChainInfo.minImageCount = desiredNumberOfSwapchainImages;
		swapChainInfo.imageFormat = colorFormat;
		swapChainInfo.imageColorSpace = colorSpace;
		swapChainInfo.imageExtent = swapchainExtent;
		swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swapChainInfo.preTransform = preTransform;
		swapChainInfo.imageArrayLayers = 1;
		swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapChainInfo.presentMode = swapchainPresentMode;
		swapChainInfo.clipped = true;
		swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		VK_VERIFY_RETURN(vkCreateSwapchainKHR(device, &swapChainInfo, nullptr, &swapChain));

		return true;
	}



	bool SwapChain::ConvertFrameBuffers()
	{
		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		presentImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, presentImages.data());

		const VkComponentMapping components =
		{
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A,
		};

		VkImageViewCreateInfo presentImagesViewCreateInfo;
		ZeroStruct(presentImagesViewCreateInfo);
		presentImagesViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		presentImagesViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		presentImagesViewCreateInfo.format = colorFormat;
		presentImagesViewCreateInfo.components = components;
		presentImagesViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		presentImagesViewCreateInfo.subresourceRange.levelCount = 1;
		presentImagesViewCreateInfo.subresourceRange.layerCount = 1;

		VkCommandBufferBeginInfo beginInfo;
		ZeroStruct(beginInfo);
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		VkFenceCreateInfo fenceCreateInfo;
		ZeroStruct(fenceCreateInfo);
		fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		VkFence submitFence;
		vkCreateFence(device, &fenceCreateInfo, nullptr, &submitFence);

		const VkImageSubresourceRange resourceRange =
		{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0, 1, 0, 1,
		};
		const VkPipelineStageFlags waitStageMask[] =
		{
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		};

		presentImageViews.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; ++i)
		{
			presentImagesViewCreateInfo.image = presentImages[i];

			vkBeginCommandBuffer(setupCmdBuffer, &beginInfo);

			VkImageMemoryBarrier layoutTransitionBarrier;
			ZeroStruct(layoutTransitionBarrier);
			layoutTransitionBarrier.srcAccessMask = 0;
			layoutTransitionBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			layoutTransitionBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			layoutTransitionBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			layoutTransitionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			layoutTransitionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			layoutTransitionBarrier.image = presentImages[i];
			layoutTransitionBarrier.subresourceRange = resourceRange;

			vkCmdPipelineBarrier(
				setupCmdBuffer,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &layoutTransitionBarrier);

			vkEndCommandBuffer(setupCmdBuffer);

			VkSubmitInfo submitInfo;
			ZeroStruct(submitInfo);
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = NULL;
			submitInfo.pWaitDstStageMask = waitStageMask;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &setupCmdBuffer;
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = NULL;
			vkQueueSubmit(queue, 1, &submitInfo, submitFence);

			vkWaitForFences(device, 1, &submitFence, VK_TRUE, UINT64_MAX);
			vkResetFences(device, 1, &submitFence);

			vkResetCommandBuffer(setupCmdBuffer, 0);

			VK_VERIFY_RETURN(
				vkCreateImageView(
					device, &presentImagesViewCreateInfo,
					nullptr, &presentImageViews[i]));
		}

		return true;
	}



	void SwapChain::Present()
	{
		vkAcquireNextImageKHR(
			device, swapChain, UINT64_MAX,
			VK_NULL_HANDLE, VK_NULL_HANDLE, &nextImageIdx);

		VkPresentInfoKHR presentInfo;
		ZeroStruct(presentInfo);
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &nextImageIdx;
		vkQueuePresentKHR(queue, &presentInfo);
	}



}
