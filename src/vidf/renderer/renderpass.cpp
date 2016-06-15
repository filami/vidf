#include "pch.h"
#include "renderpass.h"
#include "swapchain.h"
#include "renderdevice.h"
#include "rendercontext.h"


namespace vidf
{



	BaseRenderPass::~BaseRenderPass()
	{
	}



	void BaseRenderPass::AppendColorView(VkImageView view, VkClearValue* clear, bool keep)
	{
		assert(!cooked);

		VkAttachmentDescription attachmentDesc;
		ZeroStruct(attachmentDesc);
		// TODO
		attachmentDescs.push_back(attachmentDesc);

		for (uint i = 0; i < attachmentViews.size(); ++i)
			attachmentViews[i].push_back(view);

		clearValues.push_back(clear ? *clear : VkClearValue());
	}



	void BaseRenderPass::AppendSwapChain(SwapChainPtr swapChain, VkClearValue* clear)
	{
		assert(!cooked);
		assert(numFrameBuffers == 1);
		numFrameBuffers = swapChain->GetPresentImages().size();
		if (!attachmentViews.empty())
			attachmentViews.resize(numFrameBuffers, attachmentViews.front());
		else
			attachmentViews.resize(numFrameBuffers);

		VkAttachmentDescription attachmentDesc;
		ZeroStruct(attachmentDesc);
		attachmentDesc.format = swapChain->GetColorFormat();
		attachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDesc.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachmentDesc.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentDesc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachmentDescs.push_back(attachmentDesc);

		for (uint i = 0; i < attachmentViews.size(); ++i)
			attachmentViews[i].push_back(swapChain->GetPresentImageViews()[i]);

		clearValues.push_back(clear ? *clear : VkClearValue());
	}



	bool BaseRenderPass::Cook(RenderDevicePtr device, VkExtent2D _frameBufferExtents)
	{
		assert(!cooked);

		frameBufferExtents = _frameBufferExtents;

		VkAttachmentReference colorReference;
		ZeroStruct(colorReference);
		colorReference.attachment = 0;
		colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass;
		ZeroStruct(subpass);
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.flags = 0;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = NULL;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorReference;
		subpass.pResolveAttachments = NULL;
		//	subpass.pDepthStencilAttachment = &depthReference;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = NULL;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = attachmentDescs.size();
		renderPassInfo.pAttachments = attachmentDescs.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 0;
		VK_VERIFY_RETURN(vkCreateRenderPass(
			device->GetDevice(), &renderPassInfo,
			nullptr, &renderPass));

		VkFramebufferCreateInfo frameBufferCreateInfo;
		ZeroStruct(frameBufferCreateInfo);
		frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		frameBufferCreateInfo.renderPass = renderPass;
		frameBufferCreateInfo.width = frameBufferExtents.width;
		frameBufferCreateInfo.height = frameBufferExtents.height;
		frameBufferCreateInfo.layers = 1;

		frameBuffers.resize(numFrameBuffers);
		for (uint32_t i = 0; i < frameBuffers.size(); i++)
		{
			frameBufferCreateInfo.attachmentCount = attachmentViews[i].size();
			frameBufferCreateInfo.pAttachments = attachmentViews[i].data();
			VK_VERIFY_RETURN(vkCreateFramebuffer(
				device->GetDevice(), &frameBufferCreateInfo,
				nullptr, &frameBuffers[i]));
		}

		cooked = true;
		return true;
	}



	void BaseRenderPass::Begin(RenderContextPtr context)
	{
		assert(frameBuffers.size() == 1);
		Begin(context, 0);
	}



	void BaseRenderPass::Begin(RenderContextPtr context, SwapChainPtr swapChain)
	{
		Begin(context, swapChain->GetCurrentPresentId());
	}



	void BaseRenderPass::Begin(RenderContextPtr context, uint frameBufferIdx)
	{
		assert(cooked);
		assert(!started);
		VkRenderPassBeginInfo renderPassBeginInfo;
		ZeroStruct(renderPassBeginInfo);
		renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassBeginInfo.renderPass = renderPass;
		renderPassBeginInfo.renderArea.offset.x = 0;
		renderPassBeginInfo.renderArea.offset.y = 0;
		renderPassBeginInfo.renderArea.extent = frameBufferExtents;
		renderPassBeginInfo.clearValueCount = clearValues.size();
		renderPassBeginInfo.pClearValues = clearValues.data();
		renderPassBeginInfo.framebuffer = frameBuffers[frameBufferIdx];
		vkCmdBeginRenderPass(
			context->GetDrawCommandBuffer(), &renderPassBeginInfo,
			VK_SUBPASS_CONTENTS_INLINE);
		started = true;
	}



	void BaseRenderPass::End(RenderContextPtr context)
	{
		assert(started);
		vkCmdEndRenderPass(context->GetDrawCommandBuffer());
		started = false;
	}


}
