#include "pch.h"
#include "shaders.h"
#include "pipeline.h"
#include "renderdevice.h"

namespace vidf { namespace dx11 {



	GraphicsPSOPtr GraphicsPSO::Create(RenderDevicePtr renderDevice, const GraphicsPSODesc& desc)
	{
		assert(desc.vertexShader && desc.vertexShader->IsValid() && desc.vertexShader->GetShaderType() == ShaderType::VertexShader);
		assert(desc.pixelShader && desc.pixelShader->IsValid() && desc.pixelShader->GetShaderType() == ShaderType::PixelShader);
		assert(desc.topology >= D3D_PRIMITIVE_TOPOLOGY_POINTLIST && desc.topology <= D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST);

		PD3D11Device3 device = renderDevice->GetDevice3();
		GraphicsPSOPtr pso = std::make_shared<GraphicsPSO>();

		if (desc.geometryDesc && desc.numGeomDesc)
		{
			PD3DBlob code = desc.vertexShader->GetByteCode();
			renderDevice->GetDevice()->CreateInputLayout(
				desc.geometryDesc, desc.numGeomDesc,
				code->GetBufferPointer(), code->GetBufferSize(),
				&pso->inputAssembly.Get());
		}
		device->CreateRasterizerState2(&desc.rasterizer, &pso->rasterizer.Get());
		device->CreateDepthStencilState(&desc.depthStencil, &pso->depthStencil.Get());
		pso->vertexShader = desc.vertexShader->GetVertexShader();
		pso->pixelShader = desc.pixelShader->GetPixelShader();
		pso->topology = desc.topology;

		return pso;
	}



	CommandBuffer::CommandBuffer(RenderDevicePtr _renderDevice)
		: renderDevice(_renderDevice)
	{
	}
	


	void CommandBuffer::SetGraphicsPSO(GraphicsPSOPtr pso)
	{
		currentGraphicsPSO = pso;
	}



	void CommandBuffer::SetVertexStream(uint index, PD3D11Buffer stream, uint stride, uint offset)
	{
		vertexStreams[index].buffer = stream;
		vertexStreams[index].offset = offset;
		vertexStreams[index].stride = stride;
	}



	void CommandBuffer::SetSRV(uint index, PD3D11ShaderResourceView srv)
	{
		srvs[index] = srv;
	}



	void CommandBuffer::Draw(uint vertexCount, uint startVertexLocation)
	{
		FlushGraphicsState();
		renderDevice->GetContext()->Draw(vertexCount, startVertexLocation);
	}



	void CommandBuffer::ClearState()
	{
		srvs.fill(PD3D11ShaderResourceView());
		vertexStreams.fill(VertexStream());
		renderDevice->GetContext()->ClearState();
	}



	void CommandBuffer::FlushGraphicsState()
	{
		PD3D11DeviceContext context = renderDevice->GetContext();

		std::array<ID3D11ShaderResourceView*, numSrvs> srvsArray;
		for (uint i = 0; i < numSrvs; ++i)
			srvsArray[i] = srvs[i];

		std::array<ID3D11Buffer*, numVertexStreams> vertexStreamArray;
		std::array<UINT, numVertexStreams> vertexStrideStrideArray;
		std::array<UINT, numVertexStreams> vertexStreamOffsetArray;
		for (uint i = 0; i < numSrvs; ++i)
		{
			vertexStreamArray[i] = vertexStreams[i].buffer;
			vertexStrideStrideArray[i] = vertexStreams[i].stride;
			vertexStreamOffsetArray[i] = vertexStreams[i].offset;
		}

		context->IASetInputLayout(currentGraphicsPSO->inputAssembly);
		context->IASetVertexBuffers(
			0, numVertexStreams, vertexStreamArray.data(), vertexStrideStrideArray.data(),
			vertexStreamOffsetArray.data());
		context->OMSetDepthStencilState(currentGraphicsPSO->depthStencil, 0);
		context->RSSetState(currentGraphicsPSO->rasterizer);
		context->IASetPrimitiveTopology(currentGraphicsPSO->topology);
		context->VSSetShader(currentGraphicsPSO->vertexShader, nullptr, 0);
		context->PSSetShader(currentGraphicsPSO->pixelShader, nullptr, 0);
		context->VSSetShaderResources(0, numSrvs, srvsArray.data());
		context->PSSetShaderResources(0, numSrvs, srvsArray.data());
	}


} }
