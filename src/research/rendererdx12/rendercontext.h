#pragma once

#include "common.h"



namespace vidf::dx12
{


enum class DrawBatchMode
{
	Draw,
	DrawInstanced,
	DrawIndexed,
	DrawIndexedInstanced,
};



struct DrawBatch
{
	DrawBatchMode          mode;
	D3D_PRIMITIVE_TOPOLOGY topology;
	GraphicsPSOPtr         pso;
	uint instanceCount = 1;
	uint vertexCountPerInstance = 0;
	uint startVertexLocation = 0;
	uint startInstanceLocation = 0;
	uint baseVertexLocation = 0;

	void AddVertexBuffer(GPUBufferPtr buffer);

	vector<D3D12_VERTEX_BUFFER_VIEW> vertexStream;
	vector<GPUBufferPtr> vertexBuffers;
	GPUBufferPtr         indexBuffer;
};



class RenderContext
{
public:
	void ClearRenderTarget(GPUBufferPtr buffer, Color color);
	void ClearDepthStencilTarget(GPUBufferPtr buffer);
	void BeginRenderPass(RenderPassPtr renderPass);
	void EndRenderPass();
	void SetResourceSet(uint index, ResourceSetPtr set);
	void Draw(const DrawBatch& batch);

	void ComputeDispatch(ComputePtr compute, uint x, uint y, uint z);

	void CopyResource(GPUBufferPtr buffer, const void* dataPtr, uint dataSize);
	template<typename Type>
	void CopyResource(GPUBufferPtr buffer, const Type& object) { CopyResource(buffer, &object, sizeof(Type)); }
	void CopyResource(GPUBufferPtr dst, GPUBufferPtr src);

	// private:
	void AddResourceBarrier(GPUBufferPtr buffer, D3D12_RESOURCE_STATES state);
	void FlushResourceBarriers();

	// private:
	RenderDevicePtr                renderDevice;
	PD3D12CommandAllocator         commandAllocator;
	PD3D12GraphicsCommandList      commandList;
	vector<D3D12_RESOURCE_BARRIER> barriers;
	ID3D12RootSignature* rootSignature = nullptr;
	ID3D12PipelineState* pipelineState = nullptr;
	ID3D12Resource*      indexBuffer = nullptr;
	array<ID3D12Resource*, 128> vertexBuffers;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	uint frameIndex;
};



}
