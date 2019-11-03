#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>
#include "d3dx12.h"


namespace vidf::dx12
{


#	define Assert(TEST)   if (!(TEST)) __debugbreak();
#	define AssertHr(hres) if ((hres) != S_OK) __debugbreak();


	const uint frameCount = 2;


	typedef Pointer<IDXGIFactory4>          PDXGIFactory4;
	typedef Pointer<IDXGIAdapter1>          PDXGIAdapter1;
	typedef Pointer<IDXGISwapChain3>        PDXGISwapChain3;

	typedef Pointer<ID3D12Debug>               PD3D12Debug;
	typedef Pointer<ID3D12Device>              PD3D12Device;
	typedef Pointer<ID3D12CommandQueue>        PD3D12CommandQueue;
	typedef Pointer<ID3D12DescriptorHeap>      PD3D12DescriptorHeap;
	typedef Pointer<ID3D12CommandAllocator>    PD3D12CommandAllocator;
	typedef Pointer<ID3D12GraphicsCommandList> PD3D12GraphicsCommandList;
	typedef Pointer<ID3D12Fence>               PD3D12Fence;
	typedef Pointer<ID3D12Resource>            PD3D12Resource;
	typedef Pointer<ID3D12RootSignature>       PD3D12RootSignature;
	typedef Pointer<ID3D12PipelineState>       PD3D12PipelineState;
	typedef Pointer<ID3DUserDefinedAnnotation> PD3DUserDefinedAnnotation;

	typedef Pointer<ID3D12Device5>              PD3D12Device5;
	typedef Pointer<ID3D12GraphicsCommandList4> PD3D12GraphicsCommandList4;


	class RenderDevice;
	class SwapChain;
	class RenderFence;
	class GPUBuffer;
	class RenderPass;
	class GraphicsPSO;
	class Compute;
	class ResourceSet;
	class ResourceLayout;
	typedef shared_ptr<RenderDevice>    RenderDevicePtr;
	typedef shared_ptr<SwapChain>       SwapChainPtr;
	typedef shared_ptr<GPUBuffer>       GPUBufferPtr;
	typedef shared_ptr<RenderPass>      RenderPassPtr;
	typedef shared_ptr<GraphicsPSO>     GraphicsPSOPtr;
	typedef shared_ptr<Compute>         ComputePtr;
	typedef shared_ptr<GraphicsPSO>     GraphicsPSOPtr;
	typedef shared_ptr<ResourceSet>     ResourceSetPtr;
	typedef shared_ptr<ResourceLayout>  ResourceLayoutPtr;



	struct Viewport
	{
		Viewport() = default;
		Viewport(uint _width, uint _height)
			: width(_width)
			, height(_height) {}

		float topLeftX = 0.0f;
		float topLeftY = 0.0f;
		float width    = 0.0f;
		float height   = 0.0f;
		float minDepth = 0.0f;
		float maxDepth = 1.0f;
	};



	template<typename OutInterface, typename InInterface>
	Pointer<OutInterface> QueryInterface(Pointer<InInterface> inInterface)
	{
		Pointer<OutInterface> outInterface;
		inInterface->QueryInterface(__uuidof(OutInterface), (void**)&outInterface.Get());
		return outInterface;
	}


}
