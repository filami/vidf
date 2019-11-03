#include "pch.h"
#include "utilitypasses.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"


namespace vidf
{


	const char* shaderFile = "data/shaders/passes/utilities.hlsl";



	void ReducePass::Prepare(RenderDevicePtr renderDevice, ShaderManager& shaderManager, GPUBuffer& inputBuffer, Mode mode)
	{
		assert(inputBuffer.desc.type == GPUBufferType::Texture2D);

		input = inputBuffer;

		vsShader = shaderManager.CompileShaderFile(shaderFile, "vsFullscreen", ShaderType::VertexShader);

		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		switch (mode)
		{
		case HalfResolution:
			psShader = shaderManager.CompileShaderFile(shaderFile, "psReduceHalfSize", ShaderType::PixelShader);
			viewport.Width = uint(inputBuffer.desc.width / 2);
			viewport.Height = uint(inputBuffer.desc.height / 2);
			break;
		case QuarterResolution:
			psShader = shaderManager.CompileShaderFile(shaderFile, "psReduceQuarterSize", ShaderType::PixelShader);
			viewport.Width = uint(inputBuffer.desc.width / 4);
			viewport.Height = uint(inputBuffer.desc.height / 4);
			break;
		}

		GPUBufferDesc outputDesc;
		outputDesc.type = GPUBufferType::Texture2D;
		outputDesc.format = inputBuffer.desc.format;
		outputDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_RenderTarget;
		outputDesc.width = viewport.Width;
		outputDesc.height = viewport.Height;
		outputDesc.name = "ReducePassOutput";
		output = GPUBuffer::Create(renderDevice, outputDesc);

		RenderPassDesc renderPassDesc;
		renderPassDesc.viewport = viewport;
		renderPassDesc.rtvs[0] = output.rtv;
		renderPass = RenderPass::Create(renderDevice, renderPassDesc);

		GraphicsPSODesc psoDesc;
		psoDesc.rasterizer.CullMode = D3D11_CULL_NONE;
		psoDesc.rasterizer.FillMode = D3D11_FILL_SOLID;
		psoDesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		psoDesc.vertexShader = vsShader;
		psoDesc.pixelShader = psShader;
		pso = GraphicsPSO::Create(renderDevice, psoDesc);
	}



	void ReducePass::Draw(CommandBuffer& commandBuffer)
	{
		VIDF_GPU_EVENT(commandBuffer.GetRenderDevice(), ReducePass);

		commandBuffer.BeginRenderPass(renderPass);
		commandBuffer.SetGraphicsPSO(pso);
		commandBuffer.SetSRV(0, input.srv);
		commandBuffer.Draw(3, 0);
		commandBuffer.EndRenderPass();
	}



	void GaussianBlurPass::Prepare(RenderDevicePtr renderDevice, ShaderManager& shaderManager, GPUBuffer& inputBuffer, Mode mode)
	{
		assert(inputBuffer.desc.type == GPUBufferType::Texture2D);

		input = inputBuffer;
				
		vsShader = shaderManager.CompileShaderFile(shaderFile, "vsFullscreen", ShaderType::VertexShader);

		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = uint(inputBuffer.desc.width);
		viewport.Height = uint(inputBuffer.desc.height);
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		switch (mode)
		{
		case Gauss7Samples:
			psShader = shaderManager.CompileShaderFile(shaderFile, "psGaussianBlur7", ShaderType::PixelShader);
			break;
		case Gauss15Samples:
			psShader = shaderManager.CompileShaderFile(shaderFile, "psGaussianBlur15", ShaderType::PixelShader);
			break;
		}

		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = 0;
		renderDevice->GetDevice()->CreateSamplerState(&samplerDesc, &sampler.Get());

		CBuffer cbData;
		cbData.invSize = Vector2f(1.0f / viewport.Width, 1.0f / viewport.Height);
		cbData.direction = Vector2f(1.0f, 0.0f);
		GPUBufferDesc cbDesc;
		cbDesc.type = GPUBufferType::ConstantBuffer;
		cbDesc.elementStride = sizeof(CBuffer);
		cbDesc.dataPtr = &cbData;
		cbDesc.dataSize = sizeof(cbDesc.elementStride);
		cbDesc.name = "GaussianBlurPassCB0";
		cbuffer0 = GPUBuffer::Create(renderDevice, cbDesc);
		cbData.direction = Vector2f(0.0f, 1.0f);
		cbDesc.name = "GaussianBlurPassCB1";
		cbuffer1 = GPUBuffer::Create(renderDevice, cbDesc);

		GPUBufferDesc rtvDesc;
		rtvDesc.type = GPUBufferType::Texture2D;
		rtvDesc.format = inputBuffer.desc.format;
		rtvDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_RenderTarget;
		rtvDesc.width = viewport.Width;
		rtvDesc.height = viewport.Height;
		rtvDesc.name = "GaussianBlurTemp";
		tempBuffer = GPUBuffer::Create(renderDevice, rtvDesc);
		rtvDesc.name = "GaussianBlurOutput";
		output = GPUBuffer::Create(renderDevice, rtvDesc);

		RenderPassDesc renderPassDesc;
		renderPassDesc.viewport = viewport;
		renderPassDesc.rtvs[0] = tempBuffer.rtv;
		renderPass0 = RenderPass::Create(renderDevice, renderPassDesc);
		renderPassDesc.rtvs[0] = output.rtv;
		renderPass1 = RenderPass::Create(renderDevice, renderPassDesc);

		GraphicsPSODesc psoDesc;
		psoDesc.rasterizer.CullMode = D3D11_CULL_NONE;
		psoDesc.rasterizer.FillMode = D3D11_FILL_SOLID;
		psoDesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		psoDesc.vertexShader = vsShader;
		psoDesc.pixelShader = psShader;
		pso = GraphicsPSO::Create(renderDevice, psoDesc);
	}



	void GaussianBlurPass::Draw(CommandBuffer& commandBuffer)
	{
		VIDF_GPU_EVENT(commandBuffer.GetRenderDevice(), GaussianBlurPass);

		commandBuffer.BeginRenderPass(renderPass0);
		commandBuffer.SetGraphicsPSO(pso);
		commandBuffer.GetContext()->PSSetSamplers(0, 1, &sampler.Get());
		commandBuffer.SetConstantBuffer(0, cbuffer0);		
		commandBuffer.SetSRV(0, input.srv);
		commandBuffer.Draw(3, 0);
		commandBuffer.EndRenderPass();

		commandBuffer.BeginRenderPass(renderPass1);
		commandBuffer.SetGraphicsPSO(pso);
		commandBuffer.GetContext()->PSSetSamplers(0, 1, &sampler.Get());
		commandBuffer.SetConstantBuffer(0, cbuffer1);
		commandBuffer.SetSRV(0, tempBuffer.srv);
		commandBuffer.Draw(3, 0);
		commandBuffer.EndRenderPass();
	}



}
