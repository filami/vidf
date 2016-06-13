#pragma once

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
		~SwapChain();

		void Present();

	private:
		friend class RenderDevice;
		SwapChain();

		bool Connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device, VkCommandBuffer _setupCmdBuffer, VkQueue _queue);
		bool CreateSurface(const SwapChainDesc& desc);
		void QueryColorFormat();
		bool QueryQueueNodeIndex();
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

	public:
	// private:
		VkInstance instance;
		VkPhysicalDevice physicalDevice;
		VkDevice device;
		VkCommandBuffer setupCmdBuffer;
		std::vector<VkImage> presentImages;
		std::vector<VkImageView> presentImageViews;
		VkSwapchainKHR swapChain = nullptr;
		VkSurfaceKHR surface = nullptr;
		VkQueue queue = nullptr;
		VkFormat colorFormat;
		VkColorSpaceKHR colorSpace;
		uint32_t nextImageIdx = 0;
		uint32_t queueNodeIndex = UINT32_MAX;
	};



}
