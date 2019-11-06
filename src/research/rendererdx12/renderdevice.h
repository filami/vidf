#pragma once

#include "common.h"
#include "shadermanager.h"
#include "descriptorheap.h"
#include "resources.h"



namespace vidf::dx12
{


struct RenderDeviceDesc
{
	RenderDeviceDesc() {}
	const char* name = "vidf";
	bool enableValidation = true;
};



struct SwapChainDesc
{
	void* windowHandle = nullptr;
	unsigned int width = 0;
	unsigned int height = 0;
};



struct ComputeDesc
{
	ShaderPtr         shader;
	ResourceLayoutPtr resourceLayout;
	array<GPUBufferPtr, 4>  cb;
	array<GPUBufferPtr, 64> srv;
	array<GPUBufferPtr, 16> uav;
};



struct GraphicsPSODesc
{
	GraphicsPSODesc()
	{
		sampleDesc.Count = 1;
		sampleDesc.Quality = 0;
	}
	vector<D3D12_INPUT_ELEMENT_DESC> geometryDesc;
	ResourceLayout*          resourceLayout = nullptr;
	RenderPassPtr            renderPass;
	D3D12_RASTERIZER_DESC    rasterizer = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	D3D12_DEPTH_STENCIL_DESC depthStencil = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);;
	D3D12_BLEND_DESC         blend = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	DXGI_SAMPLE_DESC         sampleDesc;
	ShaderPtr                vertexShader;
	ShaderPtr                pixelShader;
	uint                     sampleMask = UINT_MAX;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
};



class GraphicsPSO
{
public:

	// private:
	friend class RenderDevice;
	friend class RenderContext;
	GraphicsPSODesc     desc;
	PD3D12RootSignature rootSignature;
	PD3D12PipelineState pipelineState;
};



struct RenderPassDesc
{
	RenderPassDesc()
	{
		viewport.MaxDepth = 1.0f;
	}
	static const uint numRtvs = 8;
	typedef std::vector<GPUBufferPtr> RTVArray;
	RTVArray       rtvs;
	GPUBufferPtr   dsv;
	D3D12_VIEWPORT viewport{};
};



class RenderPass
{
public:

private:
	friend class RenderDevice;
	friend class RenderContext;
	string         name;
	RenderPassDesc desc;
	vector<D3D12_CPU_DESCRIPTOR_HANDLE> rtvs[frameCount]{};
	D3D12_CPU_DESCRIPTOR_HANDLE dsv{};
	D3D12_RECT scissor;
};



class Compute
{
	// private:
public:
	friend class RenderDevice;
	friend class RenderContext;
	PD3D12PipelineState  pso;
	ResourceLayoutPtr    rl;

	PD3D12RootSignature  rs;
	vector<GPUBufferPtr> cbs;
	vector<GPUBufferPtr> srvs;
	vector<GPUBufferPtr> uavs;

	DescriptorHandle descTable;
	uint tableSize;
};



class RenderFence
{
public:
private:
	friend class RenderDevice;
	friend class RenderContext;
	PD3D12Fence fence;
	HANDLE      event;
	UINT64      value = -1;
	UINT64      waitValue = -1;
};



class RenderDevice : public enable_shared_from_this<RenderDevice>
{
public:
	static RenderDevicePtr Create(const RenderDeviceDesc& desc);

	SwapChainPtr      CreateSwapChain(const SwapChainDesc& swapChainDesc);
	RenderFence       CreateFence();
	GPUBufferPtr      CreateBuffer(const GPUBufferDesc& desc);
	RenderPassPtr     CreateRenderPass(const RenderPassDesc& desc, const char* name);
	GraphicsPSOPtr    CreateGraphicsPSO(const GraphicsPSODesc& desc);
	ComputePtr        CreateCompute(const ComputeDesc& desc);
	ResourceSetPtr    CreateResourceSet();
	ResourceLayoutPtr CreateResourceLayout();
	DescriptorHandle  CreateSampler(const D3D12_SAMPLER_DESC& desc);	// TODO - abstract
	void              PrepareResourceSet(ResourceSetPtr rs);
	void              PrepareResourceLayout(ResourceLayoutPtr rl);

	void              BeginRender(SwapChainPtr swapChain);
	RenderContextPtr  BeginRenderContext();
	void              Present();

	void              SetFence(RenderFence& fence);
	void              WaitForFence(RenderFence& fence);

	// TODO - remove when not needed
	PD3D12Device  GetDevice() const { return device; }
	PD3D12Device5 GetDevice5() const { return device5; }
	ScratchAllocator* GetUploadScratch() const { return uploadScratch.get(); }
	ScratchAllocator* GetUavScratch() const { return uavScratch.get(); }

private:
	PD3D12Device  device;
	PD3D12Device5 device5;
	unique_ptr<DescriptorHeap> viewHeap;
	unique_ptr<DescriptorHeap> viewTableHeap;
	unique_ptr<DescriptorHeap> rtvHeap;
	unique_ptr<DescriptorHeap> dsvHeap;
	unique_ptr<DescriptorHeap> samplerHeap;
	unique_ptr<ScratchAllocator> uploadScratch;
	unique_ptr<ScratchAllocator> uavScratch;

	PD3D12CommandAllocator    submitCLAlloc;
	PD3D12GraphicsCommandList submitCL;
	bool                   submitingResources = false;

	PD3D12CommandQueue         commandQueue;
	vector<RenderContextPtr>   freeContexts;
	vector<RenderContextPtr>   commitedContexts;
	vector<ID3D12CommandList*> commitedCLs;

	PDXGIFactory4   dxgiFactory;
	PDXGIAdapter1   dxgiAdapter;
	PD3D12Debug     debugController;

	PD3DUserDefinedAnnotation userAnnotations;

	SwapChainPtr curSwapChain;
};


	
class SwapChain
{
public:
	SwapChain(const SwapChainDesc& swapChainDesc, ID3D12Device* device, IDXGIFactory4* dxgiFactory, DescriptorHeap* rtvHeap, RenderDevice* renderDevice, ID3D12CommandQueue* commandQueue);

	GPUBufferPtr GetBackBuffer() const { return frameBuffer; }

private:
	friend class RenderDevice;
	void Present(RenderDevice* renderDevice);
	uint GetFrameIndex() const { return frameBuffer->frameIndex; }

private:
	PDXGISwapChain3 swapChain3;
	GPUBufferPtr    frameBuffer;
	uint rtvDescriptorSize;
	RenderFence frameFence;
};



}
