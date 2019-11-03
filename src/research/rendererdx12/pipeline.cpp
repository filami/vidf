#include "pch.h"
#include "pipeline.h"
#include "renderdevice.h"
#include "resources.h"

#if 0

namespace vidf::dx12
{



	RenderPassPtr RenderPass::Create(RenderDevicePtr renderDevice, const RenderPassDesc& desc)
	{
		return RenderPassPtr();
	}



	CommandBuffer::CommandBuffer(RenderDevicePtr _renderDevice)
		: renderDevice(_renderDevice)
		, graphicsCommandList(_renderDevice->GetGraphicsCommandList())
	{
	}



	void CommandBuffer::Begin()
	{
		Assert(isOpen == false);
		isOpen = true;
	}



	void CommandBuffer::End()
	{
		Assert(isOpen == true);

		graphicsCommandList->Close();
		fence = renderDevice->SetFence();

		isOpen = false;
	}



}

#endif
