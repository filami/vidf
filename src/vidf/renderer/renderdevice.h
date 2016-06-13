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



	class RenderDevice : public std::enable_shared_from_this<RenderDevice>
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

		const VkInstance& GetInstance() const { return instance; }
		const VkPhysicalDevice& GetPhysicalDevice() const { return physicalDevice; }
		const VkDevice& GetDevice() const { return device; }
		const VkQueue& GetQeueue() { return queue; }
		const VkCommandPool& GetCommandPool() const { return commandPool; }
		const VkCommandBuffer& GetSetupCommandBuffer() const { return setupCmdBuffer; }

	private:
		RenderDevice();

		void QueryLayers();
		void QueryExtensions();
		bool CreateInstance(const RenderDeviceDesc& desc);
		bool CreatePhysicalDevice();
		bool CreateDevice(const RenderDeviceDesc& desc);

	private:
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
