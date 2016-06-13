#pragma once

#include "common.h"

namespace vidf
{


	struct RenderDeviceDesc
	{
		RenderDeviceDesc() {}
		const char* name = "vidf";
		bool enableValidation = true;
	};



	class RenderDevice
	{
	private:
		struct Layers
		{
			bool validationLayer = false;
		};

		struct DeviceExtensions
		{
			bool debugMarkers = false;
		};

	public:
		~RenderDevice();
		static RenderDevicePtr Create(const RenderDeviceDesc& desc);
		SwapChainPtr CreateSwapChain(const SwapChainDesc& desc);
		RenderContextPtr CreateRenderContext();

		VkInstance GetInstance() { return instance; }
		VkDevice GetDevice() { return device; }
		VkQueue GetQeueue() { return queue; }

	private:
		RenderDevice();

		void QueryLayers();
		void QueryExtensions();
		bool CreateInstance(const RenderDeviceDesc& desc);
		bool CreatePhysicalDevice();
		bool CreateDevice(const RenderDeviceDesc& desc);

	public:
		// private:
		Layers layers;
		DeviceExtensions extensions;
		VkInstance instance = nullptr;
		VkPhysicalDevice physicalDevice = nullptr;
		VkDevice device = nullptr;
		VkCommandPool commandPool = nullptr;
		VkCommandBuffer setupCmdBuffer = nullptr;
		VkQueue queue = nullptr;
		uint32_t queueIndex = 0;
	};



}
