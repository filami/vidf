#include "pch.h"
#include "rendercontext.h"
#include "renderdevice.h"


namespace vidf::dx12
{



void RenderContext::ClearRenderTarget(GPUBufferPtr buffer, Color color)
{
	Assert((buffer->desc.usage & GPUUsage_RenderTarget) != 0);
	AddResourceBarrier(buffer, D3D12_RESOURCE_STATE_RENDER_TARGET);
	FlushResourceBarriers();
	commandList->ClearRenderTargetView(buffer->rtvs[frameIndex].cpu, &color.r, 0, nullptr);
}



void RenderContext::ClearDepthStencilTarget(GPUBufferPtr buffer)
{
	Assert((buffer->desc.usage & GPUUsage_DepthStencil) != 0);
	AddResourceBarrier(buffer, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	FlushResourceBarriers();
	commandList->ClearDepthStencilView(buffer->dsv.cpu, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
}



void RenderContext::BeginRenderPass(RenderPassPtr renderPass)
{
	D3D12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr;
	if (renderPass->dsv.ptr != 0)
	{
		dsv = &renderPass->dsv;
		AddResourceBarrier(renderPass->desc.dsv, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	}
	for (uint i = 0; i < renderPass->desc.rtvs.size(); ++i)
		AddResourceBarrier(renderPass->desc.rtvs[i], D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->OMSetRenderTargets(
		renderPass->rtvs[frameIndex].size(), renderPass->rtvs[frameIndex].data(),
		false, dsv);
	commandList->RSSetViewports(1, &renderPass->desc.viewport);
	commandList->RSSetScissorRects(1, &renderPass->scissor);
}



void RenderContext::EndRenderPass()
{
}



void RenderContext::SetResourceSet(uint index, const ResourceSetPtr set)
{
	// TODO : add this in flush instead
	// TODO : add GPUBuffer usage flag immutable which means that barriers will not be needed
	Assert(set->dirty == false);
	if (set->cb)
		AddResourceBarrier(set->cb, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	// TODO : create correct barrier
//	if (set.srv)
//		AddResourceBarrier(set.srv, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	if (set->uav)
		AddResourceBarrier(set->uav, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	FlushResourceBarriers();
	commandList->SetGraphicsRootDescriptorTable(index, set->descTable.gpu);
}



void RenderContext::Draw(const DrawBatch& batch)
{
	if (rootSignature != batch.pso->rootSignature)
	{
		commandList->SetGraphicsRootSignature(batch.pso->rootSignature);
		rootSignature = batch.pso->rootSignature;
	}
	if (pipelineState != batch.pso->pipelineState)
	{
		commandList->SetPipelineState(batch.pso->pipelineState);
		pipelineState = batch.pso->pipelineState;
	}
	if (topology != batch.topology)
	{
		commandList->IASetPrimitiveTopology(batch.topology);
		topology = batch.topology;
	}
	bool setVBs = false;
	for (uint i = 0; i < batch.vertexStream.size(); ++i)
	{
		AddResourceBarrier(batch.vertexBuffers[i], D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		if (vertexBuffers[i] != batch.vertexBuffers[i]->resource[0].resource)
		{
			setVBs = true;
			vertexBuffers[i] = batch.vertexBuffers[i]->resource[0].resource;
		}
	}
	if (setVBs)
		commandList->IASetVertexBuffers(0, batch.vertexStream.size(), batch.vertexStream.data());

	if (batch.indexBuffer)
	{
		AddResourceBarrier(batch.indexBuffer, D3D12_RESOURCE_STATE_INDEX_BUFFER);
		if (indexBuffer != batch.indexBuffer->resource[0].resource)
		{
			commandList->IASetIndexBuffer(&batch.indexBuffer->ibv);
			indexBuffer = batch.indexBuffer->resource[0].resource;
		}
	}

	FlushResourceBarriers();

	switch (batch.mode)
	{
	case DrawBatchMode::Draw:
		commandList->DrawInstanced(
			batch.vertexCountPerInstance, 1,
			batch.startVertexLocation, 0);
		break;
	case DrawBatchMode::DrawInstanced:
		commandList->DrawInstanced(
			batch.vertexCountPerInstance, batch.instanceCount,
			batch.startVertexLocation, batch.startInstanceLocation);
		break;
	case DrawBatchMode::DrawIndexed:
		commandList->DrawIndexedInstanced(
			batch.vertexCountPerInstance, 1,
			batch.startVertexLocation,
			batch.baseVertexLocation, 0);
		break;
	case DrawBatchMode::DrawIndexedInstanced:
		commandList->DrawIndexedInstanced(
			batch.vertexCountPerInstance, batch.instanceCount,
			batch.startVertexLocation,
			batch.baseVertexLocation, batch.startInstanceLocation);
		break;
	default:
		__debugbreak(); // unknown
		break;
	}
}



void RenderContext::ComputeDispatch(ComputePtr compute, uint x, uint y, uint z)
{
	commandList->SetComputeRootSignature(compute->rl->rootSignature);
	commandList->SetPipelineState(compute->pso);
	for (uint i = 0; i < compute->rl->sets.size(); ++i)
	{
		const auto& set = *compute->rl->sets[i];
		commandList->SetComputeRootDescriptorTable(i, set.descTable.gpu);
		for (uint j = 0; j < set.entries.size(); ++j)
		{
			const auto& entry = set.entries[j];
			D3D12_RESOURCE_STATES state;
			switch (entry.type)
			{
			case D3D12_DESCRIPTOR_RANGE_TYPE_SRV: state = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; break;
			case D3D12_DESCRIPTOR_RANGE_TYPE_UAV: state = D3D12_RESOURCE_STATE_UNORDERED_ACCESS; break;
			case D3D12_DESCRIPTOR_RANGE_TYPE_CBV: state = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; break;
			}
			AddResourceBarrier(entry.buffer, state);
		}
	}
	FlushResourceBarriers();
	commandList->Dispatch(x, y, z);
}



void RenderContext::AddResourceBarrier(GPUBufferPtr buffer, D3D12_RESOURCE_STATES state)
{
	if (buffer->resource[frameIndex].state != state)
	{
		D3D12_RESOURCE_BARRIER barrier{};
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = buffer->resource[frameIndex].resource;
		barrier.Transition.StateBefore = buffer->resource[frameIndex].state;
		barrier.Transition.StateAfter = state;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		buffer->resource[frameIndex].state = state;
		barriers.push_back(barrier);
	}
}



void RenderContext::FlushResourceBarriers()
{
	if (barriers.empty())
		return;
	commandList->ResourceBarrier(
		barriers.size(),
		barriers.data());
	barriers.clear();
}



void RenderContext::CopyResource(GPUBufferPtr buffer, const void* dataPtr, uint dataSize)
{
	AddResourceBarrier(buffer, D3D12_RESOURCE_STATE_COPY_DEST);
	FlushResourceBarriers();

	D3D12_SUBRESOURCE_DATA data{};
	data.pData = dataPtr;
	data.RowPitch = dataSize;
	data.SlicePitch = 0;
	UpdateSubresources(
		commandList, buffer->resource[0].resource,
		buffer->copyBuffer, 0, 0, 1, &data);
}



void RenderContext::CopyResource(GPUBufferPtr dst, GPUBufferPtr src)
{
	AddResourceBarrier(src, D3D12_RESOURCE_STATE_COPY_SOURCE);
	AddResourceBarrier(dst, D3D12_RESOURCE_STATE_COPY_DEST);
	FlushResourceBarriers();

	const uint srcFrameIndex = src->frameIndex;
	const uint dstFrameIndex = dst->frameIndex;
	commandList->CopyResource(dst->resource[dstFrameIndex].resource, src->resource[srcFrameIndex].resource);
}



void DrawBatch::AddVertexBuffer(GPUBufferPtr buffer)
{
	D3D12_VERTEX_BUFFER_VIEW view;
	view.BufferLocation = buffer->gpuHandle;
	view.SizeInBytes = buffer->desc.dataSize;
	view.StrideInBytes = buffer->desc.dataStride;
	vertexStream.push_back(view);
	vertexBuffers.push_back(buffer);
}



}
