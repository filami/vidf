#include "pch.h"
#include "common.h"



struct ViewCB
{
	Vector3f camPosition;
	float _pad0;
	Vector3f camFront;
	float _pad1;
	Vector3f camRight;
	float _pad2;
	Vector3f camUp;
	float _pad3;

	Vector2i viewportSz;
	Vector2i _pad4;
};



void KaleidoscopeIFS()
{
	const uint width = 1280;
	const uint height = 720;

	RenderDevicePtr renderDevice = RenderDevice::Create(RenderDeviceDesc());
	if (!renderDevice)
		return;
	ShaderManager shaderManager(renderDevice);
	CommandBuffer commandBuffer(renderDevice);

	Dx11CanvasListener canvasListener{ shaderManager };

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

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = width;
	viewport.Height = height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	GPUBufferDesc viewCBDesc;
	viewCBDesc.type = GPUBufferType::ConstantBuffer;
	viewCBDesc.elementStride = sizeof(ViewCB);
	viewCBDesc.usageFlags = GPUUsage_Dynamic;
	viewCBDesc.name = "viewCB";
	GPUBuffer viewCB = GPUBuffer::Create(renderDevice, viewCBDesc);

	ShaderPtr vsFullscreen = shaderManager.CompileShaderFile("data/shaders/fractals/kaleidoscopeifs.hlsl", "vsFullscreen", ShaderType::VertexShader);
	ShaderPtr psKaleidoscopeIFS = shaderManager.CompileShaderFile("data/shaders/fractals/kaleidoscopeifs.hlsl", "psKaleidoscopeIFS", ShaderType::PixelShader);

	GraphicsPSODesc kifsPSODesc;
	kifsPSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
	kifsPSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	kifsPSODesc.rasterizer.FrontCounterClockwise = true;
	kifsPSODesc.depthStencil.DepthEnable = true;
	kifsPSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS;
	kifsPSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	kifsPSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	kifsPSODesc.vertexShader = vsFullscreen;
	kifsPSODesc.pixelShader = psKaleidoscopeIFS;
	GraphicsPSOPtr kifsPSO = GraphicsPSO::Create(renderDevice, kifsPSODesc);

	RenderPassDesc renderPassDesc;
	renderPassDesc.viewport = viewport;
	renderPassDesc.rtvs[0] = swapChain->GetBackBufferRTV();
	RenderPassPtr renderPass = RenderPass::Create(renderDevice, renderPassDesc);

	OrbitalCamera camera{ canvas };
	camera.SetCamera(Vector3f{ zero }, Quaternionf{ zero }, 20.0f);

	TimeCounter counter;
	Time time;
	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		auto context = renderDevice->GetContext();

		Time deltaTime = counter.GetElapsed();
		time += deltaTime;
		camera.Update(deltaTime);

		ViewCB viewCBData;
		viewCBData.camPosition = camera.Position();
		viewCBData.camFront = camera.Front();
		viewCBData.camRight = camera.Side();
		viewCBData.camUp = camera.Up();
		viewCBData.viewportSz.x = width;
		viewCBData.viewportSz.y = height;
		viewCB.Update(context, viewCBData);

		{
			commandBuffer.BeginRenderPass(renderPass);
			commandBuffer.SetConstantBuffer(0, viewCB);
			commandBuffer.SetGraphicsPSO(kifsPSO);
			commandBuffer.Draw(3, 0);
			commandBuffer.EndRenderPass();
		}

		swapChain->Present();
	}
}
