#include "pch.h"
#include "vidf/rendererdx11/renderdevice.h"


using namespace vidf;
using namespace dx11;


class Dx11CanvasListener : public CanvasListener
{
public:
	virtual void Close()
	{
		PostQuitMessage();
	}
	virtual void KeyDown(KeyCode keyCode)
	{
		if (keyCode == KeyCode::Escape)
			PostQuitMessage();
	}
};



void TestDx11()
{
	const uint width = 1280;
	const uint height = 720;

	RenderDevicePtr renderDevice = RenderDevice::Create(RenderDeviceDesc());
	if (!renderDevice)
		return;

	Dx11CanvasListener canvasListener;

	CanvasDesc canvasDesc{};
	canvasDesc.width = width;
	canvasDesc.height = height;
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	canvas->AddListener(&canvasListener);
	if (!canvas)
		return;

	SwapChainDesc swapChainDesc{};
	swapChainDesc.width = canvasDesc.width;
	swapChainDesc.height = canvasDesc.height;
	swapChainDesc.windowHandle = canvas->GetHandle();
	SwapChainPtr swapChain = renderDevice->CreateSwapChain(swapChainDesc);
	if (!swapChain)
		return;

	struct Vertex
	{
		Vector2f position;
		uint32 color;
	};
	Vertex vertices[] =
	{
		{ Vector2f(0.25f, -0.25f), 0xffffff },
		{ Vector2f(0.25f, 0.75f), 0xffffff },
		{ Vector2f(-0.25f, 0.0f), 0xffffff },

		{ Vector2f(-0.05f, 0.0f), 0xffff0000 },
		{ Vector2f(0.0f, 0.5f), 0xff00ff00 },
		{ Vector2f(0.5f, 0.0f), 0xff0000ff },
	};

	D3D11_BUFFER_DESC vertexBufferDesc{};
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.ByteWidth = sizeof(vertices);
	vertexBufferDesc.StructureByteStride = sizeof(Vertex);
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	D3D11_SUBRESOURCE_DATA vertexData{};
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = sizeof(vertices);
	PD3D11Buffer vertexBuffer;
	renderDevice->GetDevice()->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer.Get());
	NameObject(vertexBuffer, "vertexBuffer");

	PD3DBlob output;
	PD3DBlob vertexCode;
	D3DCompileFromFile(
		L"data/shaders/shader.hlsl", nullptr, nullptr, "vsMain", "vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0,
		&vertexCode.Get(), &output.Get());
	if (output)
		std::cout << (const char*)output->GetBufferPointer() << std::endl;
	PD3D11VertexShader vertexShader;
	renderDevice->GetDevice()->CreateVertexShader(vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), nullptr, &vertexShader.Get());
	NameObject(vertexShader, "data/shaders/shader.hlsl : vsMain");

	PD3DBlob pixelCode;
	D3DCompileFromFile(
		L"data/shaders/shader.hlsl", nullptr, nullptr, "psMain", "ps_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0,
		&pixelCode.Get(), &output.Get());
	if (output)
		std::cout << (const char*)output->GetBufferPointer() << std::endl;
	PD3D11PixelShader pixelShader;
	renderDevice->GetDevice()->CreatePixelShader(pixelCode->GetBufferPointer(), pixelCode->GetBufferSize(), nullptr, &pixelShader.Get());
	NameObject(pixelShader, "data/shaders/shader.hlsl : psMain");

	PD3DBlob _vertexCode;
	D3DCompileFromFile(
		L"data/shaders/shader.hlsl", nullptr, nullptr, "vsFinalMain", "vs_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0,
		&_vertexCode.Get(), &output.Get());
	if (output)
		std::cout << (const char*)output->GetBufferPointer() << std::endl;
	PD3D11VertexShader finalVertexShader;
	renderDevice->GetDevice()->CreateVertexShader(_vertexCode->GetBufferPointer(), _vertexCode->GetBufferSize(), nullptr, &finalVertexShader.Get());
	NameObject(finalVertexShader, "data/shaders/shader.hlsl : vsFinalMain");

	D3DCompileFromFile(
		L"data/shaders/shader.hlsl", nullptr, nullptr, "psFinalMain", "ps_5_0",
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0,
		&pixelCode.Get(), &output.Get());
	if (output)
		std::cout << (const char*)output->GetBufferPointer() << std::endl;
	PD3D11PixelShader finalPixelShader;
	renderDevice->GetDevice()->CreatePixelShader(pixelCode->GetBufferPointer(), pixelCode->GetBufferSize(), nullptr, &finalPixelShader.Get());
	NameObject(finalPixelShader, "data/shaders/shader.hlsl : psFinalMain");

	D3D11_INPUT_ELEMENT_DESC elements[2]{};
	elements[0].SemanticName = "POSITION";
	elements[0].Format = DXGI_FORMAT_R32G32_FLOAT;
	elements[0].AlignedByteOffset = offsetof(Vertex, position);
	elements[1].SemanticName = "COLOR";
	elements[1].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	elements[1].AlignedByteOffset = offsetof(Vertex, color);
	PD3D11InputLayout inputLayout;
	renderDevice->GetDevice()->CreateInputLayout(elements, ARRAYSIZE(elements), vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), &inputLayout.Get());
	NameObject(inputLayout, "inputLayout");

	D3D11_TEXTURE2D_DESC rovTestDesc{};
	rovTestDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	rovTestDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
	rovTestDesc.ArraySize = 1;
	rovTestDesc.Width = canvasDesc.width;
	rovTestDesc.Height = canvasDesc.height;
	rovTestDesc.MipLevels = 1;
	rovTestDesc.SampleDesc.Count = 1;
	rovTestDesc.Usage = D3D11_USAGE_DEFAULT;
	PD3D11Texture2D rovTest;
	renderDevice->GetDevice()->CreateTexture2D(&rovTestDesc, nullptr, &rovTest.Get());
	NameObject(rovTest, "rovTest");

	D3D11_SHADER_RESOURCE_VIEW_DESC rovTestSRVDesc{};
	rovTestSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	rovTestSRVDesc.Texture2D.MipLevels = 1;
	rovTestSRVDesc.Texture2D.MostDetailedMip = 0;
	PD3D11ShaderResourceView robTestSRV;
	renderDevice->GetDevice()->CreateShaderResourceView(rovTest, &rovTestSRVDesc, &robTestSRV.Get());

	D3D11_UNORDERED_ACCESS_VIEW_DESC rovTestUAVDesc{};
	rovTestUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	rovTestUAVDesc.Texture2D.MipSlice = 0;
	PD3D11UnorderedAccessView rovTestUAV;
	renderDevice->GetDevice()->CreateUnorderedAccessView(rovTest, &rovTestUAVDesc, &rovTestUAV.Get());

	D3D11_RASTERIZER_DESC defaultRSDesc{};
	defaultRSDesc.CullMode = D3D11_CULL_NONE;
	defaultRSDesc.FillMode = D3D11_FILL_SOLID;
	PD3D11RasterizerState defaultRS;
	renderDevice->GetDevice()->CreateRasterizerState(&defaultRSDesc, &defaultRS.Get());

	D3D11_RASTERIZER_DESC1 msaaRSDesc{};
	msaaRSDesc.CullMode = D3D11_CULL_NONE;
	msaaRSDesc.FillMode = D3D11_FILL_SOLID;
	msaaRSDesc.MultisampleEnable = true;
	msaaRSDesc.ForcedSampleCount = 16;
	PD3D11RasterizerState1 msaaRS;
	renderDevice->GetDevice3()->CreateRasterizerState1(&msaaRSDesc, &msaaRS.Get());

	D3D11_DEPTH_STENCIL_DESC defaultDSDesc{};
	PD3D11DepthStencilState defaultDS;
	renderDevice->GetDevice()->CreateDepthStencilState(&defaultDSDesc, &defaultDS.Get());

	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		PD3D11DeviceContext context = renderDevice->GetContext();
		PD3DUserDefinedAnnotation userAnnotations = renderDevice->GetUserAnnotations();

		userAnnotations->BeginEvent(L"Frame");

		FLOAT gray[] = { 0.15f, 0.15f, 0.15f, 1.0f };
		context->ClearUnorderedAccessViewFloat(rovTestUAV, gray);

		userAnnotations->BeginEvent(L"Render");
		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = canvasDesc.width;
		viewport.Height = canvasDesc.height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		ID3D11UnorderedAccessView* uavs[] = { rovTestUAV };
		context->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 1, uavs, nullptr);
		context->RSSetViewports(1, &viewport);

		const UINT vertexStride = sizeof(Vertex);
		ID3D11Buffer* vertexStreams[] = { vertexBuffer };
		UINT vertexOffsets[] = { 0 };
		context->IASetInputLayout(inputLayout);
		context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->IASetVertexBuffers(0, 1, vertexStreams, &vertexStride, vertexOffsets);
		context->VSSetShader(vertexShader, nullptr, 0);
		context->PSSetShader(pixelShader, nullptr, 0);
		context->OMSetDepthStencilState(defaultDS, 0);
		context->RSSetState(msaaRS);
		userAnnotations->SetMarker(L"{Draw \"vertexBuffer\"}");
		context->Draw(ARRAYSIZE(vertices), 0);
		context->ClearState();
		userAnnotations->EndEvent();

		userAnnotations->BeginEvent(L"Finalize");
		ID3D11RenderTargetView* rtvs[] = { swapChain->GetBackBufferRTV() };
		context->OMSetRenderTargets(1, rtvs, nullptr);
		context->RSSetViewports(1, &viewport);
		context->IASetInputLayout(nullptr);
		context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->VSSetShader(finalVertexShader, nullptr, 0);
		context->PSSetShader(finalPixelShader, nullptr, 0);
		context->OMSetDepthStencilState(defaultDS, 0);
		context->RSSetState(defaultRS);
		context->PSSetShaderResources(0, 1, &robTestSRV.Get());
		context->Draw(3, 0);
		context->ClearState();
		userAnnotations->EndEvent();

		userAnnotations->EndEvent();
		swapChain->Present();
	}
}
