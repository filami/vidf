#include "pch.h"
#include "vidf/renderer/renderdevice.h"
#include "vidf/renderer/rendercontext.h"
#include "vidf/renderer/swapchain.h"
#include "vidf/renderer/renderpass.h"


using namespace vidf;


class VulkanCanvasListener : public CanvasListener
{
public:
	virtual void Close()
	{
		PostQuitMessage();
	}
	virtual void KeyDown(KeyCode keyCode)
	{
		if (keyCode == KeyCode::Escape)
			PostQuitMessage();
	}
};


class SimplePass : public BaseRenderPass
{
public:
	SimplePass() : BaseRenderPass("SimplePass") {}
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

	VulkanCanvasListener canvasListener;
	CanvasDesc canvasDesc;
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	canvas->AddListener(&canvasListener);
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

	SimplePass simplePass;
	if (!simplePass.Prepare(device, swapChain))
		return false;

	// loop
	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		context->Begin();

		simplePass.Render(context, swapChain);

		context->End();
		device->SubmitContext(context);
		swapChain->Present();
	}

	// end

	return true;
}
