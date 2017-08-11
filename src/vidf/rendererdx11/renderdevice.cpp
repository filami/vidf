#include "pch.h"
#include "renderdevice.h"

namespace vidf {namespace dx11 {



	void LogError(HRESULT hr)
	{
		__debugbreak();
	}




	RenderDevicePtr RenderDevice::Create(const RenderDeviceDesc& desc)
	{
		HRESULT hr;

		RenderDevicePtr device = std::make_shared<RenderDevice>();
				
		D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_1;
		hr = D3D11CreateDevice(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			desc.enableValidation ? D3D11_CREATE_DEVICE_DEBUG : 0,
			&level, 1, D3D11_SDK_VERSION,
			&device->device.Get(), nullptr, &device->context.Get());
		device->device3 = QueryInterface<ID3D11Device3>(device->device);
		if (hr != S_OK)
		{
			LogError(hr);
			return RenderDevicePtr();
		}
		NameObject(device->device, "device");
		NameObject(device->context, "context");

		if (desc.enableValidation)
			device->deviceDebug = QueryInterface<ID3D11Debug>(device->device);
		device->userAnnotations = QueryInterface<ID3DUserDefinedAnnotation>(device->context);

		device->dxgiDevice = QueryInterface<IDXGIDevice>(device->device);
		device->dxgiDevice->GetAdapter(&device->dxgiAdapter.Get());
		device->dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&device->dxgiFactory.Get());

		return device;
	}



	SwapChainPtr RenderDevice::CreateSwapChain(const SwapChainDesc& _swapChainDesc)
	{
		SwapChainPtr swapChain = std::make_shared<SwapChain>();

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferCount = 2;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		swapChainDesc.BufferDesc.Width = _swapChainDesc.width;
		swapChainDesc.BufferDesc.Height = _swapChainDesc.height;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = static_cast<HWND>(_swapChainDesc.windowHandle);
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Windowed = true;
		dxgiFactory->CreateSwapChain(dxgiDevice, &swapChainDesc, &swapChain->swapChain.Get());
		NameObject(dxgiFactory, "swapChain");

		swapChain->swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&swapChain->backBuffer.Get());
		D3D11_RENDER_TARGET_VIEW_DESC backBufferRTVDesc{};
		backBufferRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		device->CreateRenderTargetView(swapChain->backBuffer, &backBufferRTVDesc, &swapChain->backBufferRTV.Get());
		NameObject(swapChain->backBufferRTV, "backBufferRTV");

		return swapChain;
	}



	void SwapChain::Present()
	{
		swapChain->Present(1, 0);
	}



} }
