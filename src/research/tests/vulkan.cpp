#include "pch.h"
#include "vidf/renderer/renderdevice.h"
#include "vidf/renderer/rendercontext.h"
#include "vidf/renderer/swapchain.h"



bool TestVulkan()
{
	using namespace vidf;

	RenderDevicePtr device = RenderDevice::Create(RenderDeviceDesc());
	if (!device)
		return false;

	CanvasDesc canvasDesc;
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	if (!canvas)
		return false;

	SwapChainDesc swapChainDesc(
		canvas->GetHandle(),
		canvasDesc.width,
		canvasDesc.height);
	SwapChainPtr swapChain = device->CreateSwapChain(swapChainDesc);
	if (!swapChain)
		return false;
	RenderContextPtr context = device->CreateRenderContext();
	if (!context)
		return false;

	//////////////////////////////////////////////////////////////////////////

	VkDevice _device = device->GetDevice();
	VkQueue queue = device->GetQeueue();
	VkCommandBuffer drawCmdBuffer = context->GetDrawCommandBuffer();

	//	VkImageView attachments[2];
	VkImageView attachments[1];

	// Depth/Stencil attachment is the same for all frame buffers
	//	attachments[1] = depthStencil.view;

	VkAttachmentDescription attachmentDescs[1] = {};
	//	VkAttachmentDescription attachments[2] = {};

	// Color attachment
	attachmentDescs[0].format = swapChain->GetColorFormat();
	attachmentDescs[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescs[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachmentDescs[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	// Depth attachment
	/*
	attachmentDescs[1].format = VK_FORMAT_D32_SFLOAT;
	attachmentDescs[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescs[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescs[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	attachmentDescs[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	*/

	VkAttachmentReference colorReference;
	ZeroStruct(colorReference);
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	/*
	VkAttachmentReference depthReference;
	ZeroStruct(depthReference);
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	*/

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

	VkRenderPass renderPass;
	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = attachmentDescs;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 0;
	vkCreateRenderPass(_device, &renderPassInfo, nullptr, &renderPass);

	VkClearValue clearValues[2];
	clearValues[0].color = { { 0.5f, 0.5f, 0.5f, 0.0f } };
	clearValues[1].depthStencil = { 1.0f, 0 };

	//////////////////////////////////////////////////////////////////////////

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = renderPass;
	frameBufferCreateInfo.attachmentCount = 1;
	frameBufferCreateInfo.pAttachments = attachments;
	//	frameBufferCreateInfo.attachmentCount = 2;
	//	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = canvasDesc.width;
	frameBufferCreateInfo.height = canvasDesc.height;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	std::vector<VkFramebuffer> frameBuffers;
	frameBuffers.resize(swapChain->GetPresentImages().size());
	for (uint32_t i = 0; i < frameBuffers.size(); i++)
	{
		attachments[0] = swapChain->GetPresentImageViews()[i];
		VK_VERIFY_RETURN(vkCreateFramebuffer(_device, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
	}
	
	//////////////////////////////////////////////////////////////////////////

	VkRenderPassBeginInfo renderPassBeginInfo;
	ZeroStruct(renderPassBeginInfo);
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = renderPass;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = canvasDesc.width;
	renderPassBeginInfo.renderArea.extent.height = canvasDesc.height;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;

	//////////////////////////////////////////////////////////////////////////

	// loop
	while (UpdateSystemMessages() == SMR_Continue)
	{
		/*	context->Begin();
		context->End();*/

		VkCommandBufferBeginInfo cmdBufferBeginInfo;
		ZeroStruct(cmdBufferBeginInfo);
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		VK_VERIFY_RETURN(vkBeginCommandBuffer(drawCmdBuffer, &cmdBufferBeginInfo));

		renderPassBeginInfo.framebuffer = frameBuffers[swapChain->GetCurrentPresentId()];
		vkCmdBeginRenderPass(drawCmdBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

		VK_VERIFY_RETURN(vkEndCommandBuffer(drawCmdBuffer));

		VkSubmitInfo submitInfo;
		ZeroStruct(submitInfo);
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &drawCmdBuffer;
		VK_VERIFY_RETURN(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));

		swapChain->Present();
	}

	// end

	return true;
}
