#pragma once


namespace vidf { namespace dx11
{


	typedef Pointer<IDXGIDevice>               PDXGIDevice;
	typedef Pointer<IDXGIAdapter>              PDXGIAdapter;
	typedef Pointer<IDXGIFactory>              PDXGIFactory;
	typedef Pointer<IDXGISwapChain>            PDXGISwapChain;
	typedef Pointer<ID3D11Debug>               PD3D11Debug;
	typedef Pointer<ID3D11Device>              PD3D11Device;
	typedef Pointer<ID3D11Device3>             PD3D11Device3;
	typedef Pointer<ID3D11DeviceContext>       PD3D11DeviceContext;
	typedef Pointer<ID3DUserDefinedAnnotation> PD3DUserDefinedAnnotation;
	typedef Pointer<ID3D11Texture2D>           PD3D11Texture2D;
	typedef Pointer<ID3D11RenderTargetView>    PD3D11RenderTargetView;
	typedef Pointer<ID3D11ShaderResourceView>  PD3D11ShaderResourceView;
	typedef Pointer<ID3D11UnorderedAccessView> PD3D11UnorderedAccessView;
	typedef Pointer<ID3D11DepthStencilView>    PD3D11DepthStencilView;
	typedef Pointer<ID3D11Buffer>              PD3D11Buffer;
	typedef Pointer<ID3D11VertexShader>        PD3D11VertexShader;
	typedef Pointer<ID3D11PixelShader>         PD3D11PixelShader;
	typedef Pointer<ID3D11InputLayout>         PD3D11InputLayout;
	typedef Pointer<ID3D11RasterizerState>     PD3D11RasterizerState;
	typedef Pointer<ID3D11RasterizerState1>    PD3D11RasterizerState1;
	typedef Pointer<ID3D11RasterizerState2>    PD3D11RasterizerState2;
	typedef Pointer<ID3D11DepthStencilState>   PD3D11DepthStencilState;
	typedef Pointer<ID3DBlob>                  PD3DBlob;



	class RenderDevice;
	class SwapChain;
	class Shader;
	typedef std::shared_ptr<RenderDevice> RenderDevicePtr;
	typedef std::shared_ptr<SwapChain>    SwapChainPtr;
	typedef std::shared_ptr<Shader>       ShaderPtr;



	template<typename OutInterface, typename InInterface>
	Pointer<OutInterface> QueryInterface(Pointer<InInterface> inInterface)
	{
		Pointer<OutInterface> outInterface;
		inInterface->QueryInterface(__uuidof(OutInterface), (void**)&outInterface.Get());
		return outInterface;
	}



	template<typename Object>
	void NameObject(Object object, const char* name)
	{
		object->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
	}



} }
