#include "pch.h"
#include "averagebrightness.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"


namespace vidf
{



	void AverageBrightness::Prepare(RenderDevicePtr renderDevice, ShaderManager& shaderManager, const AverageBrightnessDesc& desc)
	{
		assert(desc.inputBuffer.desc.type == GPUBufferType::Texture2D);

		const char* shaderFile = "data/shaders/passes/averagebrightness.hlsl";
		vsShader = shaderManager.CompileShaderFile(shaderFile, "vsFullscreen", ShaderType::VertexShader);
		psShader = shaderManager.CompileShaderFile(shaderFile, "psAverage", ShaderType::PixelShader);
		psShaderColor = shaderManager.CompileShaderFile(shaderFile, "psAverageColor", ShaderType::PixelShader);

		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		// samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = 0;
		renderDevice->GetDevice()->CreateSamplerState(&samplerDesc, &sampler.Get());

		bool first = true;
		Vector2i outputSize{ desc.inputBuffer.desc.width, desc.inputBuffer.desc.height };
		do
		{
			Vector2i inputSize = outputSize;
			outputSize.x = Max(1, outputSize.x / 4);
			outputSize.y = Max(1, outputSize.y / 4);

			CBuffer cbData;
			cbData.invSize = Vector2f(1.0f / inputSize.x, 1.0f / inputSize.y);
			GPUBufferDesc cbDesc;
			cbDesc.type = GPUBufferType::ConstantBuffer;
			cbDesc.usageFlags = GPUUsage_Dynamic;
			cbDesc.elementStride = sizeof(CBuffer);
			cbDesc.name = "AverageBrightnessCB";
			GPUBuffer cbuffer = GPUBuffer::Create(renderDevice, cbDesc);
			cbuffer.Update(renderDevice->GetContext(), cbData);

			GPUBufferDesc outputDesc;
			outputDesc.type = GPUBufferType::Texture2D;
			outputDesc.format = desc.format;
			outputDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_RenderTarget;
			outputDesc.width = outputSize.x;
			outputDesc.height = outputSize.y;
			outputDesc.name = "AverageBrightnessOutput";
			GPUBuffer output = GPUBuffer::Create(renderDevice, outputDesc);

			GraphicsPSODesc psoDesc;
			psoDesc.rasterizer.CullMode = D3D11_CULL_NONE;
			psoDesc.rasterizer.FillMode = D3D11_FILL_SOLID;
			psoDesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			psoDesc.vertexShader = vsShader;
			psoDesc.pixelShader = first ? psShaderColor : psShader;
			GraphicsPSOPtr pso = GraphicsPSO::Create(renderDevice, psoDesc);

			D3D11_VIEWPORT viewport{};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = uint(outputSize.x);
			viewport.Height = uint(outputSize.y);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			RenderPassDesc renderPassDesc;
			renderPassDesc.viewport = viewport;
			renderPassDesc.rtvs[0] = output.rtv;
			RenderPassPtr renderPass = RenderPass::Create(renderDevice, renderPassDesc);

			Pass pass;
			pass.cbuffer = cbuffer;
			pass.output = output;
			pass.pso = pso;
			pass.renderPass = renderPass;
			if (passes.empty())
				pass.inputSRV = desc.inputBuffer.srv;
			else
				pass.inputSRV = passes.back().output.srv;
			passes.push_back(pass);

			first = false;
		} while (!(outputSize.x == 1 && outputSize.y == 1));
	}



	void AverageBrightness::Draw(CommandBuffer& commandBuffer)
	{
		VIDF_GPU_EVENT(commandBuffer.GetRenderDevice(), AverageBrightness);

		for (auto& pass : passes)
		{
			commandBuffer.BeginRenderPass(pass.renderPass);
			commandBuffer.SetGraphicsPSO(pass.pso);
			commandBuffer.SetConstantBuffer(0, pass.cbuffer);
			commandBuffer.GetContext()->PSSetSamplers(0, 1, &sampler.Get());
			commandBuffer.SetSRV(0, pass.inputSRV);
			commandBuffer.Draw(3, 0);
			commandBuffer.EndRenderPass();
		}
	}


}
