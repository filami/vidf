#include "pch.h"
#include "renderervulkan/swapchain.h"
#include "renderervulkan/common.h"
#include "renderervulkan/renderdevice.h"

namespace vidf
{


	SwapChain::SwapChain(RenderDevicePtr _device)
		: device(_device)
	{
	}



	SwapChain::~SwapChain()
	{
	}



	bool SwapChain::Initialize(const SwapChainDesc& desc)
	{
		if (!CreateSurface(desc))
			return false;
		QueryColorFormat();
		if (!InitSwapChain(desc))
			return false;
		if (!ConvertFrameBuffers())
			return false;

		return true;
	}


	
	bool SwapChain::CreateSurface(const SwapChainDesc& desc)
	{
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo;
		ZeroStruct(surfaceCreateInfo);
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = GetModuleHandle(0);
		surfaceCreateInfo.hwnd = (HWND)desc.windowHandle;

		VK_VERIFY_RETURN(
			vkCreateWin32SurfaceKHR(
				device->GetInstance(), &surfaceCreateInfo,
				nullptr, &surface));

		return true;
	}



	void SwapChain::QueryColorFormat()
	{
		uint32_t formatCount = 0;
		std::vector<VkSurfaceFormatKHR> surfaceFormats;
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device->GetPhysicalDevice(), surface,
			&formatCount, nullptr);
		surfaceFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			device->GetPhysicalDevice(), surface,
			&formatCount, surfaceFormats.data());

		if (formatCount == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
			colorFormat = VK_FORMAT_B8G8R8_UNORM;
		else
			colorFormat = surfaceFormats[0].format;
		colorSpace = surfaceFormats[0].colorSpace;
	}



	bool SwapChain::InitSwapChain(const SwapChainDesc& desc)
	{
		uint32_t presentModeCount;
		VkSurfaceCapabilitiesKHR surfCaps;
		std::vector<VkPresentModeKHR> presentModes;

		VK_VERIFY_RETURN(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), surface, &surfCaps));
		VK_VERIFY_RETURN(vkGetPhysicalDeviceSurfacePresentModesKHR(device->GetPhysicalDevice(), surface, &presentModeCount, nullptr));
		presentModes.resize(presentModeCount);
		VK_VERIFY_RETURN(vkGetPhysicalDeviceSurfacePresentModesKHR(device->GetPhysicalDevice(), surface, &presentModeCount, presentModes.data()));

		swapchainExtent = surfCaps.currentExtent;
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
		VK_VERIFY_RETURN(vkCreateSwapchainKHR(device->GetDevice(), &swapChainInfo, nullptr, &swapChain));

		return true;
	}



	bool SwapChain::ConvertFrameBuffers()
	{
		uint32_t imageCount = 0;
		vkGetSwapchainImagesKHR(device->GetDevice(), swapChain, &imageCount, nullptr);
		presentImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device->GetDevice(), swapChain, &imageCount, presentImages.data());

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
		vkCreateFence(device->GetDevice(), &fenceCreateInfo, nullptr, &submitFence);

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

			vkBeginCommandBuffer(device->GetSetupCommandBuffer(), &beginInfo);

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
				device->GetSetupCommandBuffer(),
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &layoutTransitionBarrier);

			vkEndCommandBuffer(device->GetSetupCommandBuffer());

			VkSubmitInfo submitInfo;
			ZeroStruct(submitInfo);
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 0;
			submitInfo.pWaitSemaphores = NULL;
			submitInfo.pWaitDstStageMask = waitStageMask;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &device->GetSetupCommandBuffer();
			submitInfo.signalSemaphoreCount = 0;
			submitInfo.pSignalSemaphores = NULL;
			vkQueueSubmit(device->GetQeueue(), 1, &submitInfo, submitFence);

			vkWaitForFences(device->GetDevice(), 1, &submitFence, VK_TRUE, UINT64_MAX);
			vkResetFences(device->GetDevice(), 1, &submitFence);

			vkResetCommandBuffer(device->GetSetupCommandBuffer(), 0);

			VK_VERIFY_RETURN(
				vkCreateImageView(
					device->GetDevice(), &presentImagesViewCreateInfo,
					nullptr, &presentImageViews[i]));
		}

		return true;
	}



	void SwapChain::Present()
	{
		vkAcquireNextImageKHR(
			device->GetDevice(), swapChain, UINT64_MAX,
			VK_NULL_HANDLE, VK_NULL_HANDLE, &nextImageIdx);

		VkPresentInfoKHR presentInfo;
		ZeroStruct(presentInfo);
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = &swapChain;
		presentInfo.pImageIndices = &nextImageIdx;
		vkQueuePresentKHR(device->GetQeueue(), &presentInfo);
	}



}
