#include "pch.h"
#include "voxelmodelrenderer.h"
#include "vidf/rendererdx11/pipeline.h"
#include "vidf/rendererdx11/shaders.h"


namespace vi::retro::render
{


	VoxelModelRenderer::VoxelModelRenderer(RenderDevicePtr _renderDevice, ShaderManager* shaderManager)
		: renderDevice(_renderDevice)
	{
		ShaderPtr vsVoxel = shaderManager->CompileShaderFile("data/voxels/shader.hlsl", "vsVoxel", ShaderType::VertexShader);
		ShaderPtr psVoxel = shaderManager->CompileShaderFile("data/voxels/shader.hlsl", "psVoxel", ShaderType::PixelShader);
		ShaderPtr vsVoxelOutline = shaderManager->CompileShaderFile("data/voxels/shader.hlsl", "vsVoxelOutline", ShaderType::VertexShader);
		ShaderPtr gsVoxelOutline = shaderManager->CompileShaderFile("data/voxels/shader.hlsl", "gsVoxelOutline", ShaderType::GeometryShader);
		ShaderPtr psVoxelOutline = shaderManager->CompileShaderFile("data/voxels/shader.hlsl", "psVoxelOutline", ShaderType::PixelShader);

		{
			GraphicsPSODesc psoDesc;
			psoDesc.rasterizer.CullMode = D3D11_CULL_BACK;
			psoDesc.rasterizer.FillMode = D3D11_FILL_SOLID;
			psoDesc.rasterizer.FrontCounterClockwise = true;
			psoDesc.depthStencil.DepthEnable = true;
			psoDesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS;
			psoDesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			psoDesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			psoDesc.geometryDesc = VoxelModel::GetGeometryDesc();
			psoDesc.vertexShader = vsVoxel;
			voxelPreZPSO = GraphicsPSO::Create(renderDevice, psoDesc);
		}

		{
			GraphicsPSODesc psoDesc;
			psoDesc.rasterizer.CullMode = D3D11_CULL_BACK;
			psoDesc.rasterizer.FillMode = D3D11_FILL_SOLID;
			psoDesc.rasterizer.FrontCounterClockwise = true;
			psoDesc.depthStencil.DepthEnable = true;
			psoDesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS;
			psoDesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			psoDesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			psoDesc.geometryDesc = VoxelModel::GetGeometryDesc();
			psoDesc.vertexShader = vsVoxel;
			psoDesc.pixelShader = psVoxel;
			voxelPSO = GraphicsPSO::Create(renderDevice, psoDesc);
		}

		{
			GraphicsPSODesc psoDesc;
			psoDesc.rasterizer.CullMode = D3D11_CULL_NONE;
			psoDesc.rasterizer.FillMode = D3D11_FILL_SOLID;
			psoDesc.rasterizer.DepthBias = -800;
			psoDesc.rasterizer.DepthBias = 0;
			psoDesc.depthStencil.DepthEnable = true;
			psoDesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS;
			psoDesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
			psoDesc.topology = D3D10_PRIMITIVE_TOPOLOGY_LINELIST;
			psoDesc.geometryDesc = VoxelModel::GetOutlineGeometryDesc();
			psoDesc.vertexShader = vsVoxelOutline;
			psoDesc.geometryShader = gsVoxelOutline;
			psoDesc.pixelShader = psVoxelOutline;
			voxelOutlinePSO = GraphicsPSO::Create(renderDevice, psoDesc);
		}
	};


	VoxelModelRenderer::Handle VoxelModelRenderer::AddModel(shared_ptr<VoxelModel> model)
	{
		Handle handle;
		if (freeSlots.empty())
		{
			handle.id = models.size();
			models.emplace_back();

			GPUBufferDesc objectCBDesc;
			objectCBDesc.type = GPUBufferType::ConstantBuffer;
			objectCBDesc.elementStride = sizeof(ObjectCB);
			objectCBDesc.usageFlags = GPUUsage_Dynamic;
			objectCBDesc.name = "objectCB";
			models[handle.id].objectCB = GPUBuffer::Create(renderDevice, objectCBDesc);
		}
		else
		{
			handle.id = freeSlots.back();
			freeSlots.pop_back();
		}
		models[handle.id].model = model;
		models[handle.id].objectCBData.worldTM = Matrix44f(zero);
		models[handle.id].valid = true;
		models[handle.id].updateCB = true;
		model->Triangulate();
		model->CreateResources(renderDevice);
		return handle;
	}



	void VoxelModelRenderer::RemoveModel(Handle handle)
	{
		assert(handle.IsValid());
		freeSlots.push_back(handle.id);
		models[handle.id].valid = false;
		models[handle.id].model.reset();
	}



	void VoxelModelRenderer::SetModel(Handle handle, shared_ptr<VoxelModel> model)
	{
		assert(handle.IsValid());
		models[handle.id].model = model;
		model->Triangulate();
		model->CreateResources(renderDevice);
	}



	void VoxelModelRenderer::SetModelWorldTM(Handle handle, Matrix44f objectTM)
	{
		assert(handle.IsValid());
		models[handle.id].objectCBData.worldTM = objectTM;
		models[handle.id].updateCB = true;
		models[handle.id].flip = Determinant(objectTM) < 0.0f;
	}



	void VoxelModelRenderer::Prepare(CommandBuffer* commandBuffer)
	{
		toUpdateCB.clear();
		toDraw.clear();
		for (uint idx = 0; idx < models.size(); ++idx)
		{
			const Model& model = models[idx];
			if (!model.valid)
				continue;
			if (model.updateCB)
				toUpdateCB.push_back(idx);
			toDraw.push_back(idx);
		}

		for (uint idx : toUpdateCB)
		{
			models[idx].objectCB.Update(commandBuffer->GetContext(), models[idx].objectCBData);
			models[idx].updateCB = false;
		}
	}



	void VoxelModelRenderer::DrawPreZ(CommandBuffer* commandBuffer)
	{
		commandBuffer->SetGraphicsPSO(voxelPreZPSO);
		for (uint idx : toDraw)
		{
			Model& model = models[idx];
			auto indexBuffer = model.flip ? model.model->GetFlippedIndexBuffer() : model.model->GetIndexBuffer();
			commandBuffer->SetVertexStream(0, model.model->GetVertexBuffer().GetBuffer(), sizeof(VoxelModel::Vertex));
			commandBuffer->SetIndexBuffer(indexBuffer.GetBuffer(), DXGI_FORMAT_R16_UINT);
			commandBuffer->SetConstantBuffer(1, model.objectCB);
			commandBuffer->DrawIndexed(model.model->GetIndices().size(), 0, 0);
		}
	}



	void VoxelModelRenderer::Draw(CommandBuffer* commandBuffer)
	{
		commandBuffer->SetGraphicsPSO(voxelPSO);
		for (uint idx : toDraw)
		{
			Model& model = models[idx];
			auto indexBuffer = model.flip ? model.model->GetFlippedIndexBuffer() : model.model->GetIndexBuffer();
			commandBuffer->SetVertexStream(0, model.model->GetVertexBuffer().GetBuffer(), sizeof(VoxelModel::Vertex));
			commandBuffer->SetIndexBuffer(indexBuffer.GetBuffer(), DXGI_FORMAT_R16_UINT);
			commandBuffer->SetConstantBuffer(1, model.objectCB);
			commandBuffer->DrawIndexed(model.model->GetIndices().size(), 0, 0);
		}
		
	//	commandBuffer->SetGraphicsPSO(voxelOutlinePSO);
	//	for (uint idx : toDraw)
	//	{
	//		Model& model = models[idx];
	//		commandBuffer->SetVertexStream(0, model.model->GetOutlineVertexBuffer().GetBuffer(), sizeof(Vector3f));
	//		commandBuffer->SetIndexBuffer(model.model->GetOutlineIndexBuffer().GetBuffer(), DXGI_FORMAT_R32_UINT);
	//		commandBuffer->SetConstantBuffer(1, model.objectCB);
	//		commandBuffer->DrawIndexed(model.model->GetOutlineIndices().size(), 0, 0);
	//	}
	}


}
