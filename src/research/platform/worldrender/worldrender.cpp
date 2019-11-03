#include "pch.h"
#include "worldrender.h"


namespace vi::retro::render
{



	WorldRender::WorldRender(CanvasPtr canvas)
		: renderDevice(RenderDevice::Create(RenderDeviceDesc()))
		, shaderManager(renderDevice)
		, commandBuffer(renderDevice)
		, wikiDraw(renderDevice, &shaderManager)
		, voxelModelRenderer(renderDevice, &shaderManager)
	//	, canvasListener(shaderManager)
	{
		const CanvasDesc& canvasDesc = canvas->GetCanvasDesc();
	//	canvas->AddListener(&canvasListener);

		width = canvasDesc.width;
		height = canvasDesc.height;

		SwapChainDesc swapChainDesc{};
		swapChainDesc.width = width;
		swapChainDesc.height = height;
		swapChainDesc.windowHandle = canvas->GetHandle();
		swapChain = renderDevice->CreateSwapChain(swapChainDesc);

		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		GPUBufferDesc depthStencilDesc;
		depthStencilDesc.type = GPUBufferType::Texture2D;
		depthStencilDesc.format = DXGI_FORMAT_D32_FLOAT;
		depthStencilDesc.usageFlags = GPUUsage_DepthStencil | GPUUsage_ShaderResource;
		depthStencilDesc.width = width;
		depthStencilDesc.height = height;
		depthStencilDesc.name = "depthBuffer";
		depthStencil = GPUBuffer::Create(renderDevice, depthStencilDesc);

		{
			RenderPassDesc renderPassDesc;
			renderPassDesc.viewport = viewport;
			renderPassDesc.dsv = depthStencil.dsv;
			preZRenderPass = RenderPass::Create(renderDevice, renderPassDesc);
		}

		{
			RenderPassDesc renderPassDesc;
			renderPassDesc.viewport = viewport;
			renderPassDesc.rtvs[0] = swapChain->GetBackBufferRTV();
			renderPassDesc.dsv = depthStencil.dsv;
			renderPass = RenderPass::Create(renderDevice, renderPassDesc);
		}

		GPUBufferDesc viewCBDesc;
		viewCBDesc.type = GPUBufferType::ConstantBuffer;
		viewCBDesc.elementStride = sizeof(ViewConsts);
		viewCBDesc.usageFlags = GPUUsage_Dynamic;
		viewCBDesc.name = "viewCB";
		viewCB = GPUBuffer::Create(renderDevice, viewCBDesc);
	}



	void WorldRender::SetView(const Matrix44f& _projTM, const Matrix44f& _viewTM, Vector3f _viewPosition)
	{
		projTM = _projTM;
		viewTM = _viewTM;
		viewPosition = _viewPosition;
	}



	void WorldRender::Draw()
	{
		wikiDraw.PushProjViewTM(Mul(viewTM, projTM));
		wikiDraw.PushWorldTM(Matrix44f(zero));

		ViewConsts viewConsts;
		viewConsts.projTM = projTM;
		viewConsts.viewTM = viewTM;
		viewConsts.projViewTM = Mul(viewConsts.viewTM, viewConsts.projTM);
		viewConsts.invProjViewTM = Inverse(viewConsts.projViewTM);
		viewConsts.viewportSize = Vector2f(width, height);
		viewConsts.invViewportSize = Vector2f(1.0f / width, 1.0f / height);
		viewConsts.viewPosition = viewPosition;
		viewCB.Update(renderDevice->GetContext(), viewConsts);

		FLOAT white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderDevice->GetContext()->ClearRenderTargetView(swapChain->GetBackBufferRTV(), white);
		renderDevice->GetContext()->ClearDepthStencilView(depthStencil.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

		voxelModelRenderer.Prepare(&commandBuffer);

		{
			commandBuffer.BeginRenderPass(preZRenderPass);
			commandBuffer.SetConstantBuffer(0, viewCB);
			voxelModelRenderer.DrawPreZ(&commandBuffer);
			commandBuffer.EndRenderPass();
		}

		{
			commandBuffer.BeginRenderPass(renderPass);

			commandBuffer.SetConstantBuffer(0, viewCB);

			voxelModelRenderer.Draw(&commandBuffer);
			wikiDraw.Flush(&commandBuffer);

			commandBuffer.EndRenderPass();
		}

		swapChain->Present();
	}



}
