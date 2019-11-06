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
class RenderContext;
typedef shared_ptr<RenderDevice>    RenderDevicePtr;
typedef shared_ptr<SwapChain>       SwapChainPtr;
typedef shared_ptr<GPUBuffer>       GPUBufferPtr;
typedef shared_ptr<RenderPass>      RenderPassPtr;
typedef shared_ptr<GraphicsPSO>     GraphicsPSOPtr;
typedef shared_ptr<Compute>         ComputePtr;
typedef shared_ptr<GraphicsPSO>     GraphicsPSOPtr;
typedef shared_ptr<ResourceSet>     ResourceSetPtr;
typedef shared_ptr<ResourceLayout>  ResourceLayoutPtr;
typedef shared_ptr<RenderContext>   RenderContextPtr;



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



inline uint GetFormatSize(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
		return 1;
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return 4;
	default:
		__debugbreak();
	}
}



inline DXGI_FORMAT GetFormatTypeless(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_UNKNOWN:
		return DXGI_FORMAT_UNKNOWN;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
		return DXGI_FORMAT_R8_TYPELESS;

	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;

	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
		return DXGI_FORMAT_R16_TYPELESS;

	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_D32_FLOAT:
		return DXGI_FORMAT_R32_TYPELESS;

	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
		return DXGI_FORMAT_R16G16B16A16_TYPELESS;

	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_TYPELESS;

	case DXGI_FORMAT_R11G11B10_FLOAT:
		return DXGI_FORMAT_R11G11B10_FLOAT;

	default:
		__debugbreak();
	}
}



inline DXGI_FORMAT GetDepthStencilFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_D16_UNORM:
		return DXGI_FORMAT_D16_UNORM;
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_D32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;
	default:
		__debugbreak();
	}
}


inline DXGI_FORMAT GetShaderResourceFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_D16_UNORM:
		return DXGI_FORMAT_R16_UNORM;
	case DXGI_FORMAT_D32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;
	default:
		return format;
	}
}



inline DXGI_FORMAT GetUAVFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_UNKNOWN:
	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
		return format;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM;	// apperently cannot use SRGB on UAVs on Dx12. Go figure.
	default:
		__debugbreak();
	}
}


}
