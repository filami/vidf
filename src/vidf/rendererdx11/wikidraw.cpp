#include "pch.h"
#include "wikidraw.h"
#include "shaders.h"
#include "pipeline.h"

namespace vidf { namespace dx11
{



	WikiDraw::WikiDraw(RenderDevicePtr _renderDevice, ShaderManager* _shaderManager)
		: renderDevice(_renderDevice)
	{
		vertices.reserve(128);

		vertexShader = _shaderManager->CompileShaderFile("data/shaders/wikidraw.hlsl", "vsMain", ShaderType::VertexShader);
		pixelShader = _shaderManager->CompileShaderFile("data/shaders/wikidraw.hlsl", "psMain", ShaderType::PixelShader);

		D3D11_INPUT_ELEMENT_DESC elements[2]{};
		elements[0].SemanticName = "POSITION";
		elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		elements[0].AlignedByteOffset = offsetof(Vertex, vertex);
		elements[1].SemanticName = "COLOR";
		elements[1].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		elements[1].AlignedByteOffset = offsetof(Vertex, color);

		PSOs.resize(NumPsos);
		GraphicsPSODesc PSODesc;
		PSODesc.geometryDesc = elements;
		PSODesc.numGeomDesc = ARRAYSIZE(elements);
		PSODesc.rasterizer.CullMode = D3D11_CULL_BACK;
		PSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
		PSODesc.depthStencil.DepthEnable = true;
		PSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		PSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS;
		PSODesc.rasterizer.FrontCounterClockwise = true;
		PSODesc.vertexShader = vertexShader;
		PSODesc.pixelShader = pixelShader;
		PSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;
		PSOs[PointsPso] = GraphicsPSO::Create(renderDevice, PSODesc);
		PSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_LINELIST;
		PSOs[LinesPso] = GraphicsPSO::Create(renderDevice, PSODesc);
		PSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		PSOs[TrianglesPso] = GraphicsPSO::Create(renderDevice, PSODesc);

		ConstantBufferDesc viewCBDesc(sizeof(CBuffer), "cBuffer");
		cBuffer = ConstantBuffer::Create(renderDevice, viewCBDesc);
	}



	void WikiDraw::Begin(WikiDraw::StreamType type)
	{
		assert(type != Undefined);
		assert(!streaming);
		assert(!viewProjTMs.empty());
		assert(!worldTMs.empty());
		uint psoIdx;
		switch (type)
		{
		case vidf::dx11::WikiDraw::Points:    psoIdx = PointsPso; break;
		case vidf::dx11::WikiDraw::Lines:     psoIdx = LinesPso; break;
		case vidf::dx11::WikiDraw::Triangles: psoIdx = TrianglesPso; break;
		case vidf::dx11::WikiDraw::Quads:     psoIdx = TrianglesPso; break;
		}
		if (curBatch.psoIdx != psoIdx)
			RestartBatch();
		curBatch.psoIdx = psoIdx;
		curBatch.type = type;
		streaming = true;
	}



	void WikiDraw::End()
	{
		assert(streaming);
		assert(primitive.empty());
		streaming = false;
	}



	void WikiDraw::SetColor(uint8 r, uint8 g, uint8 b, uint8 a)
	{
		curVertex.color = (a << 24) | (b << 16) | (g << 8) | r;
	}



	void WikiDraw::PushVertex(Vector3f vertex)
	{
		curVertex.vertex = vertex;
		PushVertex(curVertex);
	}



	void WikiDraw::PushVertex(Vertex vertex)
	{
		assert(streaming);

		curVertex = vertex;
		primitive.push_back(vertex);

		switch (curBatch.type)
		{
		case vidf::dx11::WikiDraw::Points:
			vertices.push_back(curVertex);
			primitive.clear();
			curBatch.numVertices++;
			break;
		case vidf::dx11::WikiDraw::Lines:
			if (primitive.size() == 2)
			{
				vertices.insert(vertices.end(), primitive.begin(), primitive.end());
				primitive.clear();
				curBatch.numVertices += 2;
			}
			break;
		case vidf::dx11::WikiDraw::Triangles:
			if (primitive.size() == 3)
			{
				vertices.insert(vertices.end(), primitive.begin(), primitive.end());
				primitive.clear();
				curBatch.numVertices += 3;
			}
			break;
		case vidf::dx11::WikiDraw::Quads:
			if (primitive.size() == 4)
			{
				vertices.push_back(primitive[0]);
				vertices.push_back(primitive[1]);
				vertices.push_back(primitive[2]);
				vertices.push_back(primitive[0]);
				vertices.push_back(primitive[2]);
				vertices.push_back(primitive[3]);
				primitive.clear();
				curBatch.numVertices += 6;
			}
			break;
		}
	}



	void WikiDraw::PushProjViewTM(const Matrix44f& tm)
	{
		assert(!streaming);
		if (curBatch.psoIdx != -1)
			RestartBatch();
		curBatch.projViewTMIdx = viewProjTMs.size();
		curBatch.psoIdx = -1;
		viewProjTMs.push_back(tm);
	}



	void WikiDraw::PushWorldTM(const Matrix44f& tm)
	{
		assert(!streaming);
		if (curBatch.psoIdx != -1)
			RestartBatch();
		curBatch.worldTMIdx = worldTMs.size();
		curBatch.psoIdx = -1;
		worldTMs.push_back(tm);
	}



	void WikiDraw::Flush(CommandBuffer* commandBuffer)
	{
		assert(!streaming);
		assert(!viewProjTMs.empty());
		assert(!worldTMs.empty());

		if (curBatch.numVertices != 0)
			RestartBatch();
				
		uint lastWorldTM = -1;
		uint lastProjViewTM = -1;
		UpdateVertexBuffer(commandBuffer);
		commandBuffer->SetVertexStream(0, vertexBuffer.buffer, sizeof(Vertex));
		commandBuffer->SetConstantBuffer(0, cBuffer.buffer);
		for (auto& batch : batches)
		{
			if (lastProjViewTM != batch.projViewTMIdx || lastWorldTM != batch.worldTMIdx)
				UpdateCBuffer(commandBuffer, batch);

			commandBuffer->SetGraphicsPSO(PSOs[batch.psoIdx]);
			commandBuffer->Draw(batch.numVertices, batch.firstVertex);

			lastWorldTM = batch.worldTMIdx;
			lastProjViewTM = batch.projViewTMIdx;
		}

		vertices.clear();
		batches.clear();
		viewProjTMs.clear();
		worldTMs.clear();
		curBatch = Batch();
		curVertex = Vertex();
	}



	void WikiDraw::RestartBatch()
	{
		if (curBatch.numVertices != 0)
			batches.push_back(curBatch);
		curBatch.type = Undefined;
		curBatch.firstVertex = vertices.size();
		curBatch.numVertices = 0;
		streaming = false;
	}



	void WikiDraw::UpdateCBuffer(CommandBuffer* commandBuffer, const Batch& batch)
	{
		CBuffer cBufferData;
		cBufferData.viewProjTM = viewProjTMs[batch.projViewTMIdx];
		cBufferData.worldTM = worldTMs[batch.worldTMIdx];
		cBuffer.Update(commandBuffer->GetContext(), cBufferData);
	}



	void WikiDraw::UpdateVertexBuffer(CommandBuffer* commandBuffer)
	{
		if (gpuVertexCount != vertices.capacity())
		{
			VertexBufferDesc vertexBufferDesc(uint(sizeof(Vertex)), vertices.capacity(), "WikiGeomVB");
			vertexBufferDesc.dynamic = true;
			vertexBuffer = VertexBuffer::Create(commandBuffer->GetRenderDevice(), vertexBufferDesc);
			gpuVertexCount = vertices.capacity();
		}
		vertexBuffer.Update(commandBuffer->GetContext(), vertices.data(), vertices.size() * sizeof(Vertex));
	}



} }
