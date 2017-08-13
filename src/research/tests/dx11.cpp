#include "pch.h"
#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"


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
	CommandBuffer commandBuffer(renderDevice);

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
	
	RWTexture2DDesc rovTestDesc = RWTexture2DDesc(
		DXGI_FORMAT_R11G11B10_FLOAT,
		canvasDesc.width, canvasDesc.height,
		"rovTest");
	RWTexture2D rovTest = RWTexture2D::Create(renderDevice, rovTestDesc);

	GraphicsPSODesc PSODesc;
	PSODesc.geometryDesc = elements;
	PSODesc.numGeomDesc = ARRAYSIZE(elements);
	PSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
	PSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	PSODesc.rasterizer.MultisampleEnable = true;
	PSODesc.rasterizer.ForcedSampleCount = 16;
	PSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PSODesc.vertexShader = vertexShader;
	PSODesc.pixelShader = pixelShader;
	GraphicsPSOPtr pso = GraphicsPSO::Create(renderDevice, PSODesc);

	GraphicsPSODesc finalPSODesc;
	finalPSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
	finalPSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	finalPSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	finalPSODesc.vertexShader = finalVertexShader;
	finalPSODesc.pixelShader = finalPixelShader;
	GraphicsPSOPtr finalPSO = GraphicsPSO::Create(renderDevice, finalPSODesc);

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = canvasDesc.width;
	viewport.Height = canvasDesc.height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	RenderPassDesc renderPassDesc;
	renderPassDesc.viewport = viewport;
	renderPassDesc.uavs[0] = rovTest.uav;
	RenderPassPtr renderPass = RenderPass::Create(renderDevice, renderPassDesc);

	RenderPassDesc finalizePassDesc;
	finalizePassDesc.viewport = viewport;
	finalizePassDesc.rtvs[0] = swapChain->GetBackBufferRTV();
	RenderPassPtr finalizePass = RenderPass::Create(renderDevice, finalizePassDesc);

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

				commandBuffer.BeginRenderPass(renderPass);

				commandBuffer.SetVertexStream(0, vertexBuffer.buffer, sizeof(Vertex));
				commandBuffer.SetGraphicsPSO(pso);
				commandBuffer.Draw(ARRAYSIZE(vertices), 0);

				commandBuffer.EndRenderPass();
			}

			{
				VIDF_GPU_EVENT(renderDevice, Finalize);

				commandBuffer.BeginRenderPass(finalizePass);
								
				commandBuffer.SetSRV(0, rovTest.srv);
				commandBuffer.SetGraphicsPSO(finalPSO);
				commandBuffer.Draw(3, 0);

				commandBuffer.EndRenderPass();
			}
		}

		swapChain->Present();
	}
}
