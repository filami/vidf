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
	void SetFence(RenderFence& fence);
	void WaitForFence(RenderFence& fence);

	void ClearRenderTarget(GPUBufferPtr buffer, Color color);
	void ClearDepthStencilTarget(GPUBufferPtr buffer);
	void BeginRenderPass(RenderPassPtr renderPass);
	void EndRenderPass();
	void SetFrameBuffer(GPUBufferPtr frameBuffer);
	void SetResourceSet(uint index, ResourceSetPtr set);
	void Draw(const DrawBatch& batch);

	void ComputeDispatch(ComputePtr compute, uint x, uint y, uint z);

	void CopyResource(GPUBufferPtr buffer, const void* dataPtr, uint dataSize);
	template<typename Type>
	void CopyResource(GPUBufferPtr buffer, const Type& object) { CopyResource(buffer, &object, sizeof(Type)); }

	// private:
	void AddResourceBarrier(GPUBufferPtr buffer, D3D12_RESOURCE_STATES state);
	void FlushResourceBarriers();

	// private:
	PD3D12CommandQueue             commandQueue;
	PD3D12GraphicsCommandList      commandList;
	vector<D3D12_RESOURCE_BARRIER> barriers;
	ID3D12RootSignature* rootSignature = nullptr;
	ID3D12PipelineState* pipelineState = nullptr;
	ID3D12Resource*      indexBuffer = nullptr;
	array<ID3D12Resource*, 128> vertexBuffers;
	D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	uint frameIndex;
};



class RenderDevice
{
public:
	static RenderDevicePtr Create(const RenderDeviceDesc& desc, const SwapChainDesc& swapChainDesc);
	SwapChainPtr CreateSwapChain(const SwapChainDesc& swapChainDesc);

	RenderFence       CreateFence();
	GPUBufferPtr      CreateBuffer(const GPUBufferDesc& desc);
	RenderPassPtr     CreateRenderPass(const RenderPassDesc& desc, const char* name);
	GraphicsPSOPtr    CreateGraphicsPSO(const GraphicsPSODesc& desc);
	ComputePtr        CreateCompute(const ComputeDesc& desc);
	ResourceSetPtr    CreateResourceSet();
	ResourceLayoutPtr CreateResourceLayout();
	void              PrepareResourceSet(ResourceSetPtr rs);
	void              PrepareResourceLayout(ResourceLayoutPtr rl);

	// private:
	PD3D12Device           device;
	PD3D12CommandAllocator allocatorDirect;
	unique_ptr<DescriptorHeap> viewHeap;
	unique_ptr<DescriptorHeap> viewTableHeap;
	unique_ptr<DescriptorHeap> rtvHeap;
	unique_ptr<DescriptorHeap> dsvHeap;
	unique_ptr<DescriptorHeap> samplerHeap;
	PD3D12GraphicsCommandList submitCL;
	deque<PD3D12Resource>  pendingResources;
	bool                   submiting = false;

	Pointer<ID3D12CommandQueue> commandQueue;
	RenderContext renderContext;

	Pointer<IDXGIFactory4> dxgiFactory;
	Pointer<IDXGIAdapter1> dxgiAdapter;
	Pointer<ID3D12Debug> debugController;
	Pointer<IDXGISwapChain3> swapChain3;
	GPUBufferPtr frameBuffer;
	Pointer<ID3D12GraphicsCommandList> commandList;
	uint rtvDescriptorSize;
	uint frameIndex;
	RenderFence frameFence;
	PD3DUserDefinedAnnotation userAnnotations;
};


	
class SwapChain
{
public:
	void Present(bool vsync = true);

	GPUBufferPtr GetBackBuffer() { return backBuffer; }

private:
	friend class RenderDevice;
	PDXGISwapChain3 swapChain;
	GPUBufferPtr    backBuffer;
	uint            frameIndex = 0;
};



}
