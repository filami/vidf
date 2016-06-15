#include "pch.h"
#include "vidf/renderer/renderdevice.h"
#include "vidf/renderer/rendercontext.h"
#include "vidf/renderer/swapchain.h"
#include "vidf/renderer/renderpass.h"


using namespace vidf;


class SimplePass : public BaseRenderPass
{
public:
	bool Prepare(RenderDevicePtr device, SwapChainPtr swapChain);
	void Render(RenderContextPtr context, SwapChainPtr swapChain);
};



bool SimplePass::Prepare(RenderDevicePtr device, SwapChainPtr swapChain)
{
	VkClearValue clearValues;
	clearValues.color = { { 0.5f, 0.5f, 0.5f, 0.0f } };
	// clearValues[1].depthStencil = { 1.0f, 0 };
	AppendSwapChain(swapChain, &clearValues);

	if (!Cook(device, swapChain->GetExtent()))
		return false;
}



void SimplePass::Render(RenderContextPtr context, SwapChainPtr swapChain)
{
	Begin(context, swapChain);
	End(context);
}



bool TestVulkan()
{
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

	VkQueue queue = device->GetQeueue();
	VkCommandBuffer drawCmdBuffer = context->GetDrawCommandBuffer();

	auto& markerExt = GetVkExtDebugMarker();

	//////////////////////////////////////////////////////////////////////////

	SimplePass simplePass;
	if (!simplePass.Prepare(device, swapChain))
		return false;

	//////////////////////////////////////////////////////////////////////////

	// loop
	while (UpdateSystemMessages() == SMR_Continue)
	{
		context->Begin();

		VkDebugMarkerMarkerInfoEXT markerInfo;
		ZeroStruct(markerInfo);
		markerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_MARKER_MARKER_INFO_EXT;
		// memcpy(markerInfo.color, &color[0], sizeof(float) * 4);
		markerInfo.pMarkerName = "Test";
		markerExt.vkCmdDebugMarkerBeginEXT(drawCmdBuffer, &markerInfo);

		simplePass.Render(context, swapChain);

		markerExt.vkCmdDebugMarkerEndEXT(drawCmdBuffer);

		context->End();
		device->SubmitContext(context);
		swapChain->Present();
	}

	// end

	return true;
}
