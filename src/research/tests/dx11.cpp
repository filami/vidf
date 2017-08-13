#include "pch.h"
#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"
#include "vidf/proto/mesh.h"


using namespace vidf;
using namespace dx11;
using namespace proto;


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

	std::cout << "Loading Model . . . ";
	auto model = LoadObjModuleFromFile("data/leather_chair/leather_chair.obj");
	std::cout << "DONE" << std::endl;

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
		Vector3f position;
		Vector3f normal;
		Vector2f texCoord;
	};

	Boxf bBox;
	bBox.min = Vector3f( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
	bBox.max = Vector3f(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	std::vector<Vertex> vertices;
	for (uint geomIdx = 0; geomIdx < model->GetNumGeometries(); ++geomIdx)
	{
		const auto& geometry = model->GetGeometry(geomIdx);
		for (uint polyIdx = 0; polyIdx < geometry.numPolygons; ++polyIdx)
		{
			const uint numVertices = model->GetPolygonNumVertices(geometry.firstPolygon + polyIdx);
			if (numVertices < 3)
				continue;
						
			Vertex v0;
			v0.position = model->GetVertex(model->GetPolygonVertexIndex(polyIdx, 0));
			v0.normal = model->GetNormal(model->GetPolygonNormalIndex(polyIdx, 0));
			v0.texCoord = model->GetTexCoord(model->GetPolygonTexCoordIndex(polyIdx, 0));
						
			Vertex v1;
			v1.position = model->GetVertex(model->GetPolygonVertexIndex(polyIdx, 1));
			v1.normal = model->GetNormal(model->GetPolygonNormalIndex(polyIdx, 1));
			v1.texCoord = model->GetTexCoord(model->GetPolygonTexCoordIndex(polyIdx, 1));

			for (uint vertIdx = 2; vertIdx < numVertices; ++vertIdx)
			{
				Vertex v2;
				v2.position = model->GetVertex(model->GetPolygonVertexIndex(polyIdx, vertIdx));
				v2.normal = model->GetNormal(model->GetPolygonNormalIndex(polyIdx, vertIdx));
				v2.texCoord = model->GetTexCoord(model->GetPolygonTexCoordIndex(polyIdx, vertIdx));

				bBox = Union(bBox, v0.position);
				bBox = Union(bBox, v1.position);
				bBox = Union(bBox, v2.position);

				vertices.push_back(v0);
				vertices.push_back(v1);
				vertices.push_back(v2);
				v1 = v2;
			}
		}
	}
	VertexBuffer vertexBuffer = VertexBuffer::Create(
		renderDevice,
		VertexBufferDesc(vertices.data(), vertices.size(), "vertexBuffer"));
	
	ShaderPtr vertexShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "vsMain", ShaderType::VertexShader);
	ShaderPtr pixelShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "psMain", ShaderType::PixelShader);
	ShaderPtr oitClearPS = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "psOITClear", ShaderType::PixelShader);
	ShaderPtr finalVertexShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "vsFinalMain", ShaderType::VertexShader);
	ShaderPtr finalPixelShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "psFinalMain", ShaderType::PixelShader);

	D3D11_INPUT_ELEMENT_DESC elements[3]{};
	elements[0].SemanticName = "POSITION";
	elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elements[0].AlignedByteOffset = offsetof(Vertex, position);
	elements[1].SemanticName = "NORMAL";
	elements[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elements[1].AlignedByteOffset = offsetof(Vertex, normal);
	elements[2].SemanticName = "TEXCOORD";
	elements[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	elements[2].AlignedByteOffset = offsetof(Vertex, texCoord);
		
	/*
	struct OIT
	{
		Vector4<uint16> fragments[8];
		float           depth[8];
		uint            numFrags;
	};
	*/
	RWStructuredBufferDesc rovTestDesc(/*sizeof(OIT)*/ 164, canvasDesc.width * canvasDesc.height, "rovTest");
	RWStructuredBuffer rovTest = RWStructuredBuffer::Create(renderDevice, rovTestDesc);

	GraphicsPSODesc oitClearPSODesc;
	oitClearPSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
	oitClearPSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	oitClearPSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	oitClearPSODesc.vertexShader = finalVertexShader;
	oitClearPSODesc.pixelShader = oitClearPS;
	GraphicsPSOPtr oitClearPSO = GraphicsPSO::Create(renderDevice, oitClearPSODesc);

	GraphicsPSODesc PSODesc;
	PSODesc.geometryDesc = elements;
	PSODesc.numGeomDesc = ARRAYSIZE(elements);
	PSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
	PSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	// PSODesc.rasterizer.MultisampleEnable = true;
	// PSODesc.rasterizer.ForcedSampleCount = 16;
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

	OrbitalCamera camera(canvas);
	camera.SetPerspective(1.4f, 1.0f, 10000.0f);
	camera.SetCamera((bBox.max + bBox.min)*0.5f, Quaternionf(zero), Distance(bBox.min, bBox.max));

	struct ViewConsts
	{
		Matrix44f projTM;
		Matrix44f viewTM;
		Vector2f viewportSize;
		Vector2f invViewportSize;
	};
	ConstantBufferDesc viewCBDesc(sizeof(ViewConsts), "viewCB");
	ConstantBuffer viewCB = ConstantBuffer::Create(renderDevice, viewCBDesc);

	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		ViewConsts viewConsts;
		viewConsts.projTM = camera.PerspectiveMatrix();
		viewConsts.viewTM = camera.ViewMatrix();
		viewConsts.viewportSize = Vector2f(canvasDesc.width, canvasDesc.height);
		viewConsts.invViewportSize = Vector2f(1.0f / canvasDesc.width, 1.0f / canvasDesc.height);
		viewCB.Update(renderDevice->GetContext(), viewConsts);

		{
			VIDF_GPU_EVENT(renderDevice, Frame);
			
			{
				VIDF_GPU_EVENT(renderDevice, Render);

				commandBuffer.BeginRenderPass(renderPass);
				commandBuffer.SetConstantBuffer(0, viewCB.buffer);
								
				commandBuffer.SetGraphicsPSO(oitClearPSO);
				commandBuffer.Draw(3, 0);
				
				commandBuffer.SetVertexStream(0, vertexBuffer.buffer, sizeof(Vertex));
				commandBuffer.SetGraphicsPSO(pso);
				commandBuffer.Draw(vertices.size(), 0);
				
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
