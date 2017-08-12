#include "pch.h"
#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"


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
	ShaderManager shaderManager(renderDevice);

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
	VertexBuffer vertexBuffer = VertexBuffer::Create(
		renderDevice,
		VertexBufferDesc(vertices, ARRAYSIZE(vertices), "vertexBuffer"));
	
	ShaderPtr vertexShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "vsMain", ShaderType::VertexShader);
	ShaderPtr pixelShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "psMain", ShaderType::PixelShader);
	ShaderPtr finalVertexShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "vsFinalMain", ShaderType::VertexShader);
	ShaderPtr finalPixelShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "psFinalMain", ShaderType::PixelShader);

	D3D11_INPUT_ELEMENT_DESC elements[2]{};
	elements[0].SemanticName = "POSITION";
	elements[0].Format = DXGI_FORMAT_R32G32_FLOAT;
	elements[0].AlignedByteOffset = offsetof(Vertex, position);
	elements[1].SemanticName = "COLOR";
	elements[1].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	elements[1].AlignedByteOffset = offsetof(Vertex, color);
	PD3D11InputLayout inputLayout;
	renderDevice->GetDevice()->CreateInputLayout(elements, ARRAYSIZE(elements), vertexShader->GetByteCode()->GetBufferPointer(), vertexShader->GetByteCode()->GetBufferSize(), &inputLayout.Get());
	NameObject(inputLayout, "inputLayout");
		
	RWTexture2DDesc rovTestDesc = RWTexture2DDesc(
		DXGI_FORMAT_R11G11B10_FLOAT,
		canvasDesc.width, canvasDesc.height,
		"rovTest");
	RWTexture2D rovTest = RWTexture2D::Create(renderDevice, rovTestDesc);

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
		{
			VIDF_GPU_EVENT(renderDevice, Frame);

			D3D11_VIEWPORT viewport{};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = canvasDesc.width;
			viewport.Height = canvasDesc.height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			PD3D11DeviceContext context = renderDevice->GetContext();

			FLOAT gray[] = { 0.15f, 0.15f, 0.15f, 1.0f };
			context->ClearUnorderedAccessViewFloat(rovTest.uav, gray);

			{
				VIDF_GPU_EVENT(renderDevice, Render);

				ID3D11UnorderedAccessView* uavs[] = { rovTest.uav };
				context->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 1, uavs, nullptr);
				context->RSSetViewports(1, &viewport);

				const UINT vertexStride = sizeof(Vertex);
				ID3D11Buffer* vertexStreams[] = { vertexBuffer.buffer };
				UINT vertexOffsets[] = { 0 };
				context->IASetInputLayout(inputLayout);
				context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				context->IASetVertexBuffers(0, 1, vertexStreams, &vertexStride, vertexOffsets);
				context->VSSetShader(vertexShader->GetVertexShader(), nullptr, 0);
				context->PSSetShader(pixelShader->GetPixelShader(), nullptr, 0);
				context->OMSetDepthStencilState(defaultDS, 0);
				context->RSSetState(msaaRS);
				context->Draw(ARRAYSIZE(vertices), 0);
				context->ClearState();
			}

			{
				VIDF_GPU_EVENT(renderDevice, Finalize);

				ID3D11RenderTargetView* rtvs[] = { swapChain->GetBackBufferRTV() };
				context->OMSetRenderTargets(1, rtvs, nullptr);
				context->RSSetViewports(1, &viewport);
				context->IASetInputLayout(nullptr);
				context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				context->VSSetShader(finalVertexShader->GetVertexShader(), nullptr, 0);
				context->PSSetShader(finalPixelShader->GetPixelShader(), nullptr, 0);
				context->OMSetDepthStencilState(defaultDS, 0);
				context->RSSetState(defaultRS);
				context->PSSetShaderResources(0, 1, &rovTest.srv.Get());
				context->Draw(3, 0);
				context->ClearState();
			}
		}

		swapChain->Present();
	}
}
