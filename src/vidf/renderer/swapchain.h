#pragma once

#include "common.h"

namespace vidf
{



	struct SwapChainDesc
	{
		SwapChainDesc(void* _windowHandle, unsigned int _width, unsigned int _height)
			: windowHandle(_windowHandle)
			, width(_width)
			, height(_height) {}
		void* windowHandle;
		unsigned int width;
		unsigned int height;
	};



	class SwapChain
	{
	public:
		SwapChain(RenderDevicePtr _device);
		~SwapChain();

		VkFormat GetColorFormat() const { return colorFormat; }
		const std::vector<VkImage>& GetPresentImages() const { return presentImages; }
		const std::vector<VkImageView>& GetPresentImageViews() const { return presentImageViews; }
		uint32_t GetCurrentPresentId() const { return nextImageIdx; }
		VkExtent2D GetExtent() const { return swapchainExtent; }

		void Present();

	private:
		friend class RenderDevice;
		SwapChain();

		bool Initialize(const SwapChainDesc& desc);
		bool CreateSurface(const SwapChainDesc& desc);
		void QueryColorFormat();
		bool InitSwapChain(const SwapChainDesc& desc);
		bool ConvertFrameBuffers();

	private:
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
		PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
		PFN_vkGetPhysicalDeviceSurfacePresentModesKHR vkGetPhysicalDeviceSurfacePresentModesKHR;
		PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR;
		PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR;
		PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR;
		PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR;
		PFN_vkQueuePresentKHR vkQueuePresentKHR;

	private:
		RenderDevicePtr device;
		std::vector<VkImage> presentImages;
		std::vector<VkImageView> presentImageViews;
		VkSwapchainKHR swapChain = nullptr;
		VkSurfaceKHR surface = nullptr;
		VkExtent2D swapchainExtent;
		VkFormat colorFormat;
		VkColorSpaceKHR colorSpace;
		uint32_t nextImageIdx = 0;
	};



}
