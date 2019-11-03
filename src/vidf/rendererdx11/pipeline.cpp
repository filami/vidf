#include "pch.h"
#include "shaders.h"
#include "pipeline.h"
#include "renderdevice.h"
#include "resources.h"

namespace vidf { namespace dx11 {



	GraphicsPSOPtr GraphicsPSO::Create(RenderDevicePtr renderDevice, const GraphicsPSODesc& desc)
	{
		// valid vertex shader
		assert(desc.vertexShader && desc.vertexShader->IsValid() && desc.vertexShader->GetShaderType() == ShaderType::VertexShader);
		// either valid pixel shader or not pixel shader at all
		assert(!desc.pixelShader || (desc.pixelShader && desc.pixelShader->IsValid() && desc.pixelShader->GetShaderType() == ShaderType::PixelShader));
		// valid primitive
		assert(desc.topology >= D3D_PRIMITIVE_TOPOLOGY_POINTLIST && desc.topology <= D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST);

		PD3D11Device3 device = renderDevice->GetDevice3();
		GraphicsPSOPtr pso = std::make_shared<GraphicsPSO>();

		if (!desc.geometryDesc.empty())
		{
			PD3DBlob code = desc.vertexShader->GetByteCode();
			renderDevice->GetDevice()->CreateInputLayout(
				desc.geometryDesc.data(), desc.geometryDesc.size(),
				code->GetBufferPointer(), code->GetBufferSize(),
				&pso->inputAssembly.Get());
		}
		device->CreateRasterizerState2(&desc.rasterizer, &pso->rasterizer.Get());
		device->CreateDepthStencilState(&desc.depthStencil, &pso->depthStencil.Get());
		device->CreateBlendState1(&desc.blend, &pso->blend.Get());
		pso->vertexShader = desc.vertexShader;
		pso->geometryShader = desc.geometryShader;
		pso->pixelShader = desc.pixelShader;
		pso->topology = desc.topology;

		return pso;
	}



	RenderPass::~RenderPass()
	{
		for (auto ptr : rtvs)
		{
			if (ptr)
				ptr->Release();
		}
		for (auto ptr : uavs)
		{
			if (ptr)
				ptr->Release();
		}
	}



	RenderPassPtr RenderPass::Create(RenderDevicePtr /*renderDevice*/, const RenderPassDesc& desc)
	{
		RenderPassPtr pass = std::make_shared<RenderPass>();

		int lastSlot = 0;
		for (uint i = 0; i < RenderPassDesc::numRtvs; ++i)
			if (desc.rtvs[i]) lastSlot = i + 1;
		pass->rtvs.resize(lastSlot);
		for (uint i = 0; i < pass->rtvs.size(); ++i)
		{
			pass->rtvs[i] = const_cast<ID3D11RenderTargetView*>(desc.rtvs[i].Get());
			pass->rtvs[i]->AddRef();
		}

		pass->firstUAV = 0;
		for (pass->firstUAV = 0; pass->firstUAV < RenderPassDesc::numUavs; ++pass->firstUAV)
		{
			if (desc.uavs[pass->firstUAV])
				break;
		}
		for (uint i = pass->firstUAV; i < RenderPassDesc::numUavs; ++i)
		{
			if (!desc.uavs[i])
				break;
			pass->uavs.push_back(const_cast<ID3D11UnorderedAccessView*>(desc.uavs[i].Get()));
			pass->uavs.back()->AddRef();
		}
		if (pass->uavs.empty())
			pass->firstUAV = 0;

		pass->dsv = desc.dsv;

		pass->viewport = desc.viewport;
		pass->hasViewport = (pass->dsv != nullptr || !pass->rtvs.empty());

		return pass;
	}



	CommandBuffer::CommandBuffer(RenderDevicePtr _renderDevice)
		: renderDevice(_renderDevice)
	{
	}
	


	void CommandBuffer::BeginRenderPass(RenderPassPtr renderPass)
	{
		PD3D11DeviceContext context = renderDevice->GetContext();		
		if (renderPass->hasViewport)
			context->RSSetViewports(1, &renderPass->viewport);
		context->OMSetRenderTargetsAndUnorderedAccessViews(
			renderPass->rtvs.size(), renderPass->rtvs.data(),
			renderPass->dsv,
			renderPass->firstUAV, renderPass->uavs.size(), renderPass->uavs.data(), nullptr);
	}



	void CommandBuffer::EndRenderPass()
	{
		srvs.fill(PD3D11ShaderResourceView());
		vertexStreams.fill(VertexStream());
		renderDevice->GetContext()->ClearState();
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



	void CommandBuffer::SetIndexBuffer(PD3D11Buffer _indexBuffer, DXGI_FORMAT format, uint offset)
	{
		indexBuffer.indexBuffer = _indexBuffer;
		indexBuffer.format = format;
		indexBuffer.offset = offset;
	}



	void CommandBuffer::SetConstantBuffer(uint index, PD3D11Buffer cb)
	{
		cbs[index] = cb;
	}



	void CommandBuffer::SetConstantBuffer(uint index, GPUBuffer& cb)
	{
		assert(cb.desc.type == GPUBufferType::ConstantBuffer);
		cbs[index] = reinterpret_cast<ID3D11Buffer*>(cb.buffer.Get());
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



	void CommandBuffer::DrawIndexed(uint indexCount, uint startIndexLocation, int baseVertexLocation)
	{
		FlushGraphicsState();
		renderDevice->GetContext()->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
	}



	void CommandBuffer::DrawInstanced(uint vertexCountPerInstance, uint instanceCount, uint startVertexLocation, uint startInstanceLocation)
	{
		FlushGraphicsState();
		renderDevice->GetContext()->DrawInstanced(vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
	}



	void CommandBuffer::FlushCommonState()
	{
		PD3D11DeviceContext context = renderDevice->GetContext();

		std::array<ID3D11Buffer*, numCBs> cbsArray;
		for (uint i = 0; i < numCBs; ++i)
			cbsArray[i] = cbs[i];
		std::array<ID3D11ShaderResourceView*, numSrvs> srvsArray;
		for (uint i = 0; i < numSrvs; ++i)
			srvsArray[i] = srvs[i];

		context->VSSetConstantBuffers(0, numCBs, cbsArray.data());
		context->GSSetConstantBuffers(0, numCBs, cbsArray.data());
		context->PSSetConstantBuffers(0, numCBs, cbsArray.data());
		context->VSSetShaderResources(0, numSrvs, srvsArray.data());
		context->GSSetShaderResources(0, numSrvs, srvsArray.data());
		context->PSSetShaderResources(0, numSrvs, srvsArray.data());

		context->CSSetConstantBuffers(0, numCBs, cbsArray.data());
		context->CSSetShaderResources(0, numSrvs, srvsArray.data());
	}



	void CommandBuffer::FlushGraphicsState()
	{
		FlushCommonState();

		PD3D11DeviceContext context = renderDevice->GetContext();

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
		context->IASetIndexBuffer(indexBuffer.indexBuffer, indexBuffer.format, indexBuffer.offset);
		context->OMSetDepthStencilState(currentGraphicsPSO->depthStencil, 0);
		context->RSSetState(currentGraphicsPSO->rasterizer);
		context->OMSetBlendState(currentGraphicsPSO->blend, nullptr, ~0);
		context->IASetPrimitiveTopology(currentGraphicsPSO->topology);
		context->VSSetShader(currentGraphicsPSO->vertexShader->GetVertexShader(), nullptr, 0);
		if (currentGraphicsPSO->geometryShader)
			context->GSSetShader(currentGraphicsPSO->geometryShader->GetGeometryShader(), nullptr, 0);
		else
			context->GSSetShader(nullptr, nullptr, 0);
		if (currentGraphicsPSO->pixelShader)
			context->PSSetShader(currentGraphicsPSO->pixelShader->GetPixelShader(), nullptr, 0);
	}



	void CommandBuffer::FlushComputeState()
	{
		FlushCommonState();
		renderDevice->GetContext()->CSSetShader(currentCompute->GetComputeShader(), nullptr, 0);
	}



	void CommandBuffer::BeginComputePass(RenderPassPtr renderPass)
	{
		PD3D11DeviceContext context = renderDevice->GetContext();
		std::array<ID3D11UnorderedAccessView*, numUavs> uavArray;
		for (uint i = 0; i < renderPass->uavs.size(); ++i)
			uavArray[i] = renderPass->uavs[i];
		context->CSSetUnorderedAccessViews(renderPass->firstUAV, renderPass->uavs.size(), uavArray.data(), nullptr);
	}



	void CommandBuffer::EndComputePass()
	{
		EndRenderPass();
	}



	void CommandBuffer::SetCompute(ShaderPtr shader)
	{
		currentCompute = shader;
	}



	void CommandBuffer::Dispatch(Vector3i threadGroupCount)
	{
		FlushComputeState();
		renderDevice->GetContext()->Dispatch(threadGroupCount.x, threadGroupCount.y, threadGroupCount.z);
	}


} }
