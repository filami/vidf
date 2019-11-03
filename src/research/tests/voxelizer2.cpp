#include "pch.h"
#include "proto/mesh.h"
#include "common/plane.h"
#include "common/intersect.h"
#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"
#include "vidf/rendererdx11/wikidraw.h"
#include <deque>

using namespace vidf;
using namespace proto;
using namespace dx11;
using namespace std;


namespace
{



class Dx11CanvasListener : public CanvasListener
{
public:
	Dx11CanvasListener(ShaderManager& _shaderManager)
		: shaderManager(_shaderManager) {}

	virtual void Close()
	{
		PostQuitMessage();
	}
	virtual void KeyDown(KeyCode keyCode)
	{
		if (keyCode == KeyCode::Escape)
			PostQuitMessage();
		else if (keyCode == KeyCode::F5)
			shaderManager.RecompileShaders();
	}
	ShaderManager& shaderManager;
};




Vector3i operator&(Vector3i v, int mask)
{
	return Vector3i(v.x & mask, v.y & mask, v.z & mask);
}

Vector3i operator>>(Vector3i v, int sh)
{
	return Vector3i(v.x >> sh, v.y >> sh, v.z >> sh);
}

Vector3i operator<<(Vector3i v, int sh)
{
	return Vector3i(v.x << sh, v.y << sh, v.z << sh);
}



const uint brickShift = 3;
const uint mapLevels = 5;
const uint brickSize = 1 << brickShift;
const uint brickMask = brickSize - 1;
const uint mapSize = 1 << (mapLevels * brickShift);



template<typename T>
using Brick = array<T, brickSize * brickSize * brickSize>;

template<typename T>
using BrickDeque = deque<Brick<T>>;

typedef BrickDeque<uint32> SparceBricks;



size_t CoordIdx(Vector3i coord)
{
	assert(coord.x >= 0 && coord.x < brickSize);
	assert(coord.y >= 0 && coord.y < brickSize);
	assert(coord.z >= 0 && coord.z < brickSize);
	return coord.z << (brickShift * 2) | (coord.y << brickShift) | coord.x;
}



struct VoxelMap
{
	VoxelMap()
	{
		sparceBricks.emplace_back();
		sparceBricks.back().fill(-1);
	}
	BrickDeque<uint8> voxelBricks;
	SparceBricks      sparceBricks;
};



struct VoxelMapGPU
{
	void Build(RenderDevicePtr renderDevice, PD3D11DeviceContext context, const VoxelMap& voxelMap)
	{
		sparceBricks = BuildBricks(renderDevice, context, voxelMap.sparceBricks, DXGI_FORMAT_R32_UINT);
		voxelBricks = BuildBricks(renderDevice, context, voxelMap.voxelBricks, DXGI_FORMAT_R8_UINT);
	}

	template<typename T>
	GPUBuffer BuildBricks(RenderDevicePtr renderDevice, PD3D11DeviceContext context, const BrickDeque<T>& bricks, DXGI_FORMAT format)
	{
		typedef BrickDeque<T>::value_type BrickType;
		typedef BrickType::value_type ElementType;

		GPUBufferDesc bricksDesc;

		const uint maxSize_ = 2048;
		const uint maxSizeMask_ = maxSize_ - 1;
		const uint maxSize = 2048 >> brickShift;
		const uint maxSizeMask = maxSize - 1;
		const uint maxSizeShift = 8;

		const uint size = bricks.size();
		bricksDesc.type = GPUBufferType::Texture3D;
		bricksDesc.usageFlags = GPUUsage_ShaderResource;
		bricksDesc.format = format;
		bricksDesc.width = Clamp(size * brickSize, brickSize, 2048);
		bricksDesc.height = Clamp(((size / maxSize) + 1) * brickSize, brickSize, 2048);
		bricksDesc.depth = Clamp(((size / maxSize / maxSize) + 1) * brickSize, brickSize, 2048);
		bricksDesc.name = "Bricks";
				
		vector<ElementType> stageBuffer;
		stageBuffer.resize(bricksDesc.width * bricksDesc.height * bricksDesc.depth);
		bricksDesc.dataPtr = stageBuffer.data();
		bricksDesc.dataSize = bricksDesc.width * sizeof(ElementType);
		bricksDesc.dataSlice = bricksDesc.width * bricksDesc.height * sizeof(ElementType);

		for (uint i = 0; i < bricks.size(); ++i)
		{
			const Vector3i brickIdx = Vector3i(i & maxSizeMask, (i >> maxSizeShift) & maxSizeMask, (i >> (maxSizeShift * 2)) & maxSizeMask);
			for (uint j = 0; j < brickSize * brickSize * brickSize; ++j)
			{
				const Vector3i coord = Vector3i(j & brickMask, (j >> brickShift) & brickMask, (j >> (brickShift * 2)) & brickMask);
				const Vector3i voxelId = brickIdx * brickSize + coord;
				ElementType dst = bricks[i][j];
				stageBuffer[(voxelId.x % bricksDesc.width) + (voxelId.y % bricksDesc.height) * bricksDesc.width + voxelId.z * (bricksDesc.width * bricksDesc.height)] = dst;
			}
		}

		return GPUBuffer::Create(renderDevice, bricksDesc);
	}

	GPUBuffer voxelBricks;
	GPUBuffer sparceBricks;
};



template<typename T>
void InsertVoxel(SparceBricks* sparceBricks, BrickDeque<T>* voxelBricks, Vector3i coord, T value)
{
	int sparceIdx = 0;
	int level = mapLevels;
	Vector3i subCoord;
	for (int level = mapLevels-1; level != 0; --level)
	{
		subCoord = coord >> (brickShift * level) & brickMask;
		int newSparceIdx = (*sparceBricks)[sparceIdx][CoordIdx(subCoord)];
		if (newSparceIdx == -1 && level > 1)
		{
			newSparceIdx = sparceBricks->size();
			sparceBricks->emplace_back();
			sparceBricks->back().fill(-1);
			(*sparceBricks)[sparceIdx][CoordIdx(subCoord)] = newSparceIdx;
		}
		else if (newSparceIdx == -1 && level == 1)
		{
			newSparceIdx = voxelBricks->size();
			voxelBricks->emplace_back();
			(*sparceBricks)[sparceIdx][CoordIdx(subCoord)] = newSparceIdx;
		}
		sparceIdx = newSparceIdx;
	}
	subCoord = coord & brickMask;
	(*voxelBricks)[sparceIdx][CoordIdx(subCoord)] = value;
}



void RasterizeTriangle(VoxelMap* voxels, Trianglef triangle)
{
	const auto Floor = [](Vector3f v)
	{
		return Vector3f(std::floor(v.x), std::floor(v.y), std::floor(v.z));
	};
	const auto Ceil = [](Vector3f v)
	{
		return Vector3f(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z));
	};

	Boxf box = Boxf(
		Floor(Min(Min(triangle.v0, triangle.v1), triangle.v2)),
		Ceil(Max(Max(triangle.v0 + Vector3f(1, 1, 1), triangle.v1), triangle.v2)));

	for (float z = box.min.z; z < box.max.z; z += 1.0f)
	{
		for (float y = box.min.y; y < box.max.y; y += 1.0f)
		{
			for (float x = box.min.x; x < box.max.x; x += 1.0f)
			{
				Boxf voxel;
				voxel.min = Vector3f(x, y, z);
				voxel.max = voxel.min + Vector3f(1, 1, 1);
				if (!BoxTriangleIntersect(voxel, triangle))
					continue;
				InsertVoxel(&voxels->sparceBricks, &voxels->voxelBricks, Vector3i(x, y, z), uint8(1));
			}
		}
	}
}



void RasterizeModelGeometry(VoxelMap* voxels, const Module& model, const Matrix44f& objectTM, uint geomIdx)
{
	const Module::Geometry& geometry = model.GetGeometry(geomIdx);

	const uint lastPolygon = geometry.firstPolygon + geometry.numPolygons;
	for (uint polyIdx = geometry.firstPolygon; polyIdx != lastPolygon; ++polyIdx)
	{
		const uint numVertices = model.GetPolygonNumVertices(polyIdx);

		Trianglef triangle;
		triangle.v0 = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, 0));
		triangle.v1 = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, 1));

		for (uint vertIdx = 2; vertIdx < numVertices; ++vertIdx)
		{
			triangle.v2 = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, vertIdx));
			Trianglef transTriangle = triangle;
			transTriangle.v0 = Mul(triangle.v0, objectTM);
			transTriangle.v1 = Mul(triangle.v1, objectTM);
			transTriangle.v2 = Mul(triangle.v2, objectTM);
			RasterizeTriangle(voxels, transTriangle);
			triangle.v1 = triangle.v2;
		}
	}
}



void RasterizeModel(VoxelMap* voxels, const Module& model, const Matrix44f& objectTM)
{
	const auto Floor = [](Vector3f v)
	{
		return Vector3f(std::floor(v.x), std::floor(v.y), std::floor(v.z));
	};

	Vector3f minPos = Vector3f(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	for (uint v = 0; v < model.GetNumVertices(); ++v)
	{
		Vector3f vertex = Mul(model.GetVertex(v), objectTM);
		minPos = Min(minPos, Floor(vertex));
	}
	Matrix44f translatedTM = Mul(objectTM, Translate(-minPos));

	const uint numGeoms = model.GetNumGeometries();
	for (uint geomIdx = 0; geomIdx < numGeoms; ++geomIdx)
		RasterizeModelGeometry(voxels, model, translatedTM, geomIdx);
}



void DrawBoxWireframe(WikiDraw* wikiDraw, Boxi box)
{
	wikiDraw->Begin(WikiDraw::Lines);

	wikiDraw->PushVertex(Vector3f(box.min.x, box.min.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.min.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.min.x, box.max.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.max.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.min.x, box.max.y, box.max.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.max.y, box.max.z));
	wikiDraw->PushVertex(Vector3f(box.min.x, box.min.y, box.max.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.min.y, box.max.z));

	wikiDraw->PushVertex(Vector3f(box.min.x, box.min.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.min.x, box.max.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.min.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.max.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.min.y, box.max.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.max.y, box.max.z));
	wikiDraw->PushVertex(Vector3f(box.min.x, box.min.y, box.max.z));
	wikiDraw->PushVertex(Vector3f(box.min.x, box.max.y, box.max.z));

	wikiDraw->PushVertex(Vector3f(box.min.x, box.min.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.min.x, box.min.y, box.max.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.min.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.min.y, box.max.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.max.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.max.x, box.max.y, box.max.z));
	wikiDraw->PushVertex(Vector3f(box.min.x, box.max.y, box.min.z));
	wikiDraw->PushVertex(Vector3f(box.min.x, box.max.y, box.max.z));

	wikiDraw->End();
}



void DrawBoxFilled(WikiDraw* wikiDraw, Boxi box)
{
	const Vector3f vertices[] =
	{
		{ box.min.x, box.min.y, box.min.z },
		{ box.max.x, box.min.y, box.min.z },
		{ box.min.x, box.max.y, box.min.z },
		{ box.max.x, box.max.y, box.min.z },
		{ box.min.x, box.min.y, box.max.z },
		{ box.max.x, box.min.y, box.max.z },
		{ box.min.x, box.max.y, box.max.z },
		{ box.max.x, box.max.y, box.max.z },
	};

	wikiDraw->Begin(WikiDraw::Quads);

	wikiDraw->SetColor(255, 128, 0, 255);
	wikiDraw->PushVertex(vertices[0]);
	wikiDraw->PushVertex(vertices[2]);
	wikiDraw->PushVertex(vertices[3]);
	wikiDraw->PushVertex(vertices[1]);

	wikiDraw->SetColor(0, 128, 255, 255);
	wikiDraw->PushVertex(vertices[4]);
	wikiDraw->PushVertex(vertices[5]);
	wikiDraw->PushVertex(vertices[7]);
	wikiDraw->PushVertex(vertices[6]);

	wikiDraw->SetColor(128, 255, 0, 255);
	wikiDraw->PushVertex(vertices[0]);
	wikiDraw->PushVertex(vertices[4]);
	wikiDraw->PushVertex(vertices[6]);
	wikiDraw->PushVertex(vertices[2]);

	wikiDraw->SetColor(128, 0, 255, 255);
	wikiDraw->PushVertex(vertices[1]);
	wikiDraw->PushVertex(vertices[3]);
	wikiDraw->PushVertex(vertices[7]);
	wikiDraw->PushVertex(vertices[5]);

	wikiDraw->SetColor(0, 255, 128, 255);
	wikiDraw->PushVertex(vertices[0]);
	wikiDraw->PushVertex(vertices[1]);
	wikiDraw->PushVertex(vertices[5]);
	wikiDraw->PushVertex(vertices[4]);

	wikiDraw->SetColor(255, 0, 128, 255);
	wikiDraw->PushVertex(vertices[2]);
	wikiDraw->PushVertex(vertices[6]);
	wikiDraw->PushVertex(vertices[7]);
	wikiDraw->PushVertex(vertices[3]);

	wikiDraw->End();
}



struct BrickInstance
{
	Vector3i offset;
	Vector3i brickId;
};




void DebugDrawVoxelMapBrick(WikiDraw* wikiDraw, const VoxelMap& voxels, uint brick, Vector3i coord, uint level)
{
	wikiDraw->SetColor(0, 0, 255, 255);

	for (uint z = 0; z < brickSize; ++z)
	{
		for (uint y = 0; y < brickSize; ++y)
		{
			for (uint x = 0; x < brickSize; ++x)
			{
				Vector3i subCoord = Vector3i(x, y, z);
				if (voxels.voxelBricks[brick][CoordIdx(subCoord)] == 0)
					continue;
				Boxi box;
				box.min = coord + subCoord;
				box.max = box.min + Vector3i(1, 1, 1);
				DrawBoxFilled(wikiDraw, box);
			}
		}
	}
}


void DebugDrawVoxelMapLevel(WikiDraw* wikiDraw, vector<BrickInstance>* brickInstances, const VoxelMap& voxels, uint brick, Vector3i coord, uint level)
{
	if (brick == -1)
		return;

	Boxi box;
	box.min = coord;
	box.max = box.min + (Vector3i(1, 1, 1) << ((level + 1) * brickShift));

	if (level == 0)
	{
		if (wikiDraw)
		{
			wikiDraw->SetColor(0, 255, 0, 255);
			DrawBoxWireframe(wikiDraw, box);
		}
		if (brickInstances)
		{
			const uint maxSize_ = 2048;
			const uint maxSizeMask_ = maxSize_ - 1;
			const uint maxSize = 2048 >> brickShift;
			const uint maxSizeMask = maxSize - 1;
			const uint maxSizeShift = 8;
			const Vector3i brickIdx = Vector3i(brick & maxSizeMask, (brick >> maxSizeShift) & maxSizeMask, (brick >> (maxSizeShift * 2)) & maxSizeMask);
			brickInstances->push_back({ coord, brickIdx });
		}
	}
	else
	{
		if (wikiDraw)
		{
			wikiDraw->SetColor(255, 128, 128, 255);
			DrawBoxWireframe(wikiDraw, box);
		}

		for (uint z = 0; z < brickSize; ++z)
		{
			for (uint y = 0; y < brickSize; ++y)
			{
				for (uint x = 0; x < brickSize; ++x)
				{
					Vector3i subCoord = Vector3i(x, y, z);
					DebugDrawVoxelMapLevel(
						wikiDraw, brickInstances, voxels,
						voxels.sparceBricks[brick][CoordIdx(subCoord)],
						box.min + (subCoord << (level * brickShift)),
						level - 1);
				}
			}
		}
	}
}



void DebugDrawVoxelMap(WikiDraw* wikiDraw, vector<BrickInstance>* brickInstances, const VoxelMap& voxels)
{
	DebugDrawVoxelMapLevel(wikiDraw, brickInstances, voxels, 0, Vector3i(zero), mapLevels - 1);
}



}



void Voxelizer2()
{
	std::cout << "Loading Model . . . ";
	auto model = LoadObjModuleFromFile("data/sponza/sponza.obj");
	// auto model = LoadObjModuleFromFile("data/leather_chair/leather_chair.obj");
	// auto model = LoadObjModuleFromFile("data/primitives/box.obj");
	std::cout << "DONE" << std::endl;

	//

	VoxelMap voxelMap;
	// RasterizeModel(&voxelMap, *model, Matrix44f(zero));
	RasterizeModel(&voxelMap, *model, Scale(1.0f / 10.0f));
	/*
	for (int y = 0; y < 64; y += 3)
	{
		for (int x = 0; x < 64; x += 3)
		{
			InsertVoxel(&voxelMap.sparceBricks, &voxelMap.voxelBricks, Vector3i(x, y + 2, 3), uint8(1));
		}
	}
	*/

	const uint vxPerBrick = brickSize * brickSize * brickSize;
	const uint vxMemory = voxelMap.voxelBricks.size() * vxPerBrick + voxelMap.sparceBricks.size() * vxPerBrick * sizeof(uint32);
	std::cout << "Voxel Mem: " << vxMemory / 1024 / 1024 << "." << vxMemory / 1024 % 1024 << "Mb" << std::endl;

	//

	RenderDevicePtr renderDevice = RenderDevice::Create(RenderDeviceDesc());
	ShaderManager shaderManager(renderDevice);
	CommandBuffer commandBuffer(renderDevice);
	WikiDraw wikiDraw(renderDevice, &shaderManager);

	Dx11CanvasListener canvasListener{ shaderManager };
	CanvasDesc canvasDesc{};
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	canvas->AddListener(&canvasListener);

	SwapChainDesc swapChainDesc{};
	swapChainDesc.width = canvasDesc.width;
	swapChainDesc.height = canvasDesc.height;
	swapChainDesc.windowHandle = canvas->GetHandle();
	SwapChainPtr swapChain = renderDevice->CreateSwapChain(swapChainDesc);

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = canvasDesc.width;
	viewport.Height = canvasDesc.height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	DepthStencilDesc depthStencilDesc(
		DXGI_FORMAT_D32_FLOAT,
		canvasDesc.width, canvasDesc.height,
		"depthBuffer");
	DepthStencil depthStencil = DepthStencil::Create(renderDevice, depthStencilDesc);

	RenderPassDesc renderPassDesc;
	renderPassDesc.viewport = viewport;
	renderPassDesc.rtvs[0] = swapChain->GetBackBufferRTV();
	renderPassDesc.dsv = depthStencil.dsv;
	RenderPassPtr renderPass = RenderPass::Create(renderDevice, renderPassDesc);

	VoxelMapGPU voxelMapGPU;
	voxelMapGPU.Build(renderDevice, renderDevice->GetContext(), voxelMap);

	ShaderPtr vertexShader = shaderManager.CompileShaderFile("data/shaders/voxels/voxels.hlsl", "vsVoxels", ShaderType::VertexShader);
	ShaderPtr pixelShader = shaderManager.CompileShaderFile("data/shaders/voxels/voxels.hlsl", "psVoxels", ShaderType::PixelShader);

	ShaderPtr vsVoxelRaytrace = shaderManager.CompileShaderFile("data/shaders/voxels/voxelraytrace.hlsl", "vsVoxelRaytrace", ShaderType::VertexShader);
	ShaderPtr psVoxelRaytrace = shaderManager.CompileShaderFile("data/shaders/voxels/voxelraytrace.hlsl", "psVoxelRaytrace", ShaderType::PixelShader);

	GraphicsPSODesc voxelPSODesc;
	voxelPSODesc.rasterizer.CullMode = D3D11_CULL_BACK;
	voxelPSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	voxelPSODesc.rasterizer.FrontCounterClockwise = false;
	voxelPSODesc.depthStencil.DepthEnable = true;
	voxelPSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS;
	voxelPSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	voxelPSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	voxelPSODesc.vertexShader = vertexShader;
	voxelPSODesc.pixelShader = pixelShader;
	GraphicsPSOPtr voxelPSO = GraphicsPSO::Create(renderDevice, voxelPSODesc);

	GraphicsPSODesc voxelRaytracePSODesc;
	voxelRaytracePSODesc.rasterizer.CullMode = D3D11_CULL_BACK;
	voxelRaytracePSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	voxelRaytracePSODesc.rasterizer.FrontCounterClockwise = false;
	voxelRaytracePSODesc.depthStencil.DepthEnable = true;
	voxelRaytracePSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS;
	voxelRaytracePSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	voxelRaytracePSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	voxelRaytracePSODesc.vertexShader = vsVoxelRaytrace;
	voxelRaytracePSODesc.pixelShader = psVoxelRaytrace;
	GraphicsPSOPtr voxelRaytracePSO = GraphicsPSO::Create(renderDevice, voxelRaytracePSODesc);

	struct ViewConsts
	{
		Matrix44f projTM;
		Matrix44f viewTM;
		Matrix44f projViewTM;
		Matrix44f invProjViewTM;
		Vector2f  viewportSize;
		Vector2f  invViewportSize;
		Vector3f  viewPosition;
		uint      frameId;
	};
	ConstantBufferDesc viewCBDesc(sizeof(ViewConsts), "viewCB");
	ConstantBuffer viewCB = ConstantBuffer::Create(renderDevice, viewCBDesc);

	vector<BrickInstance> brickInstances;
	DebugDrawVoxelMap(nullptr, &brickInstances, voxelMap);
	GPUBufferDesc brickInstancesDesc;
	brickInstancesDesc.type = GPUBufferType::Structured;
	brickInstancesDesc.usageFlags = GPUUsage_ShaderResource;
	brickInstancesDesc.elementStride = sizeof(BrickInstance);
	brickInstancesDesc.elementCount = brickInstances.size();
	brickInstancesDesc.name = "brickInstances";
	brickInstancesDesc.dataPtr = brickInstances.data();
	brickInstancesDesc.dataSize = brickInstancesDesc.elementCount * brickInstancesDesc.elementStride;
	GPUBuffer brickInstancesBuffer = GPUBuffer::Create(renderDevice, brickInstancesDesc);

	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MinLOD = -std::numeric_limits<float>::max();
	samplerDesc.MaxLOD = std::numeric_limits<float>::max();
	PD3D11SamplerState diffuseSS;
	renderDevice->GetDevice()->CreateSamplerState(&samplerDesc, &diffuseSS.Get());

	//

	voxelRaytracePSODesc.pixelShader = shaderManager.CompileShaderFile("data/shaders/voxels/voxelraytrace.hlsl", "psQuarterResVoxelRaytrace", ShaderType::PixelShader);
	GraphicsPSOPtr quarterResRaytracePSO = GraphicsPSO::Create(renderDevice, voxelRaytracePSODesc);

	const uint div = 2;
	GPUBufferDesc quarterResDesc;
	quarterResDesc.type = GPUBufferType::Texture2D;
	quarterResDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	quarterResDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_RenderTarget;
	quarterResDesc.width = canvasDesc.width / div;
	quarterResDesc.height = canvasDesc.height / div;
	quarterResDesc.name = "quarterRes";
	GPUBuffer quarterRes = GPUBuffer::Create(renderDevice, quarterResDesc);

	quarterResDesc.type = GPUBufferType::Texture2D;
	quarterResDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	quarterResDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_RenderTarget;
	quarterResDesc.width = canvasDesc.width / div;
	quarterResDesc.height = canvasDesc.height / div;
	quarterResDesc.name = "normals";
	GPUBuffer normals = GPUBuffer::Create(renderDevice, quarterResDesc);

	GPUBufferDesc quarterResAccumDesc;
	quarterResAccumDesc.type = GPUBufferType::Texture2D;
	quarterResAccumDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	quarterResAccumDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_RenderTarget;
	quarterResAccumDesc.width = canvasDesc.width / div;
	quarterResAccumDesc.height = canvasDesc.height / div;
	quarterResAccumDesc.name = "quarterResAccum";
	GPUBuffer quarterResAccum = GPUBuffer::Create(renderDevice, quarterResAccumDesc);

	RenderPassDesc quarterResPassDesc;
	quarterResPassDesc.viewport = viewport;
	quarterResPassDesc.viewport.Width /= div;
	quarterResPassDesc.viewport.Height /= div;
	quarterResPassDesc.rtvs[0] = quarterRes.rtv;
	quarterResPassDesc.rtvs[1] = normals.rtv;
	RenderPassPtr quarterResPasses = RenderPass::Create(renderDevice, quarterResPassDesc);

	RenderPassDesc quarterResPassAccumDesc;
	quarterResPassAccumDesc.viewport = viewport;
	quarterResPassAccumDesc.viewport.Width /= div;
	quarterResPassAccumDesc.viewport.Height /= div;
	quarterResPassAccumDesc.rtvs[0] = quarterResAccum.rtv;
	RenderPassPtr quarterResPassesAccum = RenderPass::Create(renderDevice, quarterResPassAccumDesc);

	GraphicsPSODesc quarterResAccumPSODesc;
	quarterResAccumPSODesc.rasterizer.CullMode = D3D11_CULL_BACK;
	quarterResAccumPSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	quarterResAccumPSODesc.rasterizer.FrontCounterClockwise = false;
	quarterResAccumPSODesc.depthStencil.DepthEnable = true;
	quarterResAccumPSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS;
	quarterResAccumPSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	quarterResAccumPSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	quarterResAccumPSODesc.vertexShader = shaderManager.CompileShaderFile("data/shaders/voxels/voxelraytrace.hlsl", "vsVoxelRaytrace", ShaderType::VertexShader);
	quarterResAccumPSODesc.pixelShader = shaderManager.CompileShaderFile("data/shaders/voxels/voxelraytrace.hlsl", "psAccumQuarterRes", ShaderType::PixelShader);
	GraphicsPSOPtr quarterResAccumPSO = GraphicsPSO::Create(renderDevice, quarterResAccumPSODesc);

	//
		
	OrbitalCamera camera(canvas);
	camera.SetPerspective(1.4f, 1.0f, 1000.0f);
	camera.SetCamera(
		Vector3f(388.397919f, -5.89890289f, 122.766502f),
		Quaternionf(-0.683579028f, 0.142710492f, -0.159888670f, 0.697763085f),
		310.844086f);

	ViewConsts viewConsts;
	viewConsts.frameId = 0;

	TimeCounter counter;
	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		Time deltaTime = counter.GetElapsed();

		camera.Update(deltaTime);
		wikiDraw.PushProjViewTM(Mul(camera.ViewMatrix(), camera.PerspectiveMatrix()));
		wikiDraw.PushWorldTM(Matrix44f(zero));

		viewConsts.projTM = camera.PerspectiveMatrix();
		viewConsts.viewTM = camera.ViewMatrix();
		viewConsts.projViewTM = Mul(viewConsts.viewTM, viewConsts.projTM);
		viewConsts.invProjViewTM = Inverse(viewConsts.projViewTM);
		viewConsts.viewportSize = Vector2f(canvasDesc.width, canvasDesc.height);
		viewConsts.invViewportSize = Vector2f(1.0f / canvasDesc.width, 1.0f / canvasDesc.height);
		viewConsts.viewPosition = camera.Position();
		viewConsts.frameId++;
		viewCB.Update(renderDevice->GetContext(), viewConsts);

		FLOAT white[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		renderDevice->GetContext()->ClearRenderTargetView(swapChain->GetBackBufferRTV(), white);
		renderDevice->GetContext()->ClearDepthStencilView(depthStencil.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

		{
			VIDF_GPU_EVENT(renderDevice, RaytraceLight);

			commandBuffer.BeginRenderPass(quarterResPasses);

			commandBuffer.SetConstantBuffer(0, viewCB.buffer);
			commandBuffer.SetSRV(0, voxelMapGPU.sparceBricks.srv);
			commandBuffer.SetSRV(1, voxelMapGPU.voxelBricks.srv);
			commandBuffer.SetGraphicsPSO(quarterResRaytracePSO);
			commandBuffer.Draw(3, 0);

			commandBuffer.EndRenderPass();
		}
		{
			VIDF_GPU_EVENT(renderDevice, RayReuse);

			commandBuffer.BeginRenderPass(quarterResPassesAccum);

			commandBuffer.SetConstantBuffer(0, viewCB.buffer);
			commandBuffer.SetSRV(3, normals.srv);
			commandBuffer.SetSRV(4, quarterRes.srv);
			commandBuffer.SetGraphicsPSO(quarterResAccumPSO);
			commandBuffer.Draw(3, 0);

			commandBuffer.EndRenderPass();
		}
		{
			VIDF_GPU_EVENT(renderDevice, RaytraceScene);

			commandBuffer.BeginRenderPass(renderPass);

			commandBuffer.SetConstantBuffer(0, viewCB.buffer);
			commandBuffer.SetSRV(0, voxelMapGPU.sparceBricks.srv);
			commandBuffer.SetSRV(1, voxelMapGPU.voxelBricks.srv);
			commandBuffer.SetSRV(2, brickInstancesBuffer.srv);
			commandBuffer.SetSRV(3, quarterResAccum.srv);
			commandBuffer.GetContext()->PSSetSamplers(0, 1, &diffuseSS.Get());
			commandBuffer.SetGraphicsPSO(voxelPSO);
		//	commandBuffer.DrawInstanced(36 * brickSize * brickSize * brickSize, brickInstances.size(), 0, 0);

			commandBuffer.SetConstantBuffer(0, viewCB.buffer);
			commandBuffer.SetSRV(0, voxelMapGPU.sparceBricks.srv);
			commandBuffer.SetSRV(1, voxelMapGPU.voxelBricks.srv);
			commandBuffer.SetGraphicsPSO(voxelRaytracePSO);
			commandBuffer.Draw(3, 0);

		//	DebugDrawVoxelMap(&wikiDraw, nullptr, voxelMap);
			wikiDraw.Flush(&commandBuffer);

			commandBuffer.EndRenderPass();
		}

		swapChain->Present();

		Time time = GetTime();
	}
}
