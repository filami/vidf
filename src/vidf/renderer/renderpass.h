#pragma once

#include "common.h"

namespace vidf
{


	class BaseRenderPass
	{
	public:
		virtual ~BaseRenderPass();

	protected:
		void AppendColorView(VkImageView view, VkClearValue* clear = nullptr, bool keep = false);
		void AppendSwapChain(SwapChainPtr swapChain, VkClearValue* clear = nullptr);
		bool Cook(RenderDevicePtr device, VkExtent2D frameBufferExtents);
		void Begin(RenderContextPtr context);
		void Begin(RenderContextPtr context, SwapChainPtr swapChain);
		void End(RenderContextPtr context);

	private:
		void Begin(RenderContextPtr context, uint frameBufferIdx);

		std::vector<std::vector<VkImageView>> attachmentViews;
		std::vector<VkAttachmentDescription> attachmentDescs;
		std::vector<VkClearValue> clearValues;
		std::vector<VkFramebuffer> frameBuffers;
		VkRenderPass renderPass = VK_NULL_HANDLE;
		VkExtent2D frameBufferExtents;
		uint numFrameBuffers = 1;
		bool cooked = false;
		bool started = false;
	};



}
