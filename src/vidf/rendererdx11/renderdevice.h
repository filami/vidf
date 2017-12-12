#pragma once

#include "common.h"

namespace vidf { namespace dx11
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



	class RenderDevice
	{
	public:
		static RenderDevicePtr Create(const RenderDeviceDesc& desc);
		SwapChainPtr CreateSwapChain(const SwapChainDesc& swapChainDesc);

		PD3D11Device        GetDevice() { return device; }
		PD3D11Device3       GetDevice3() { return device3; }
		PD3D11DeviceContext GetContext() { return context; }
		PD3D11Debug               GetDeviceDebug() { return deviceDebug; }
		PD3DUserDefinedAnnotation GetUserAnnotations() { return userAnnotations; }

	private:
		PDXGIDevice         dxgiDevice;
		PDXGIAdapter        dxgiAdapter;
		PDXGIFactory        dxgiFactory;
		PD3D11Device        device;
		PD3D11Device3       device3;
		PD3D11DeviceContext context;

		PD3D11Debug               deviceDebug;
		PD3DUserDefinedAnnotation userAnnotations;
	};



	class SwapChain
	{
	public:
		PD3D11RenderTargetView GetBackBufferRTV() { return backBufferRTV; }

		void Present(bool vsync = true);

	private:
		friend class RenderDevice;
		PDXGISwapChain         swapChain;
		PD3D11Texture2D        backBuffer;
		PD3D11RenderTargetView backBufferRTV;
	};


} }
