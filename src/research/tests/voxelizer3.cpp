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
#include "platform/magicavoxel/vox.h"
#include "pathtracer/halton.h"

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



class ComputePass
{
private:
	static const uint numCBs = 8;
	static const uint numSrvs = 8;
	static const uint numUavs = 8;

	typedef std::array<GPUBuffer, numCBs>  CBArray;
	typedef std::array<GPUBuffer, numSrvs> SRVArray;
	typedef std::array<GPUBuffer, numUavs> UAVSArray;

public:
	ComputePass(const wchar_t* _name)
		: name(_name) {}

	void SetShader(ShaderPtr _shader);
	void SetCB(uint index, GPUBuffer cb);
	void SetSRV(uint index, GPUBuffer srv);
	void SetUAV(uint index, GPUBuffer srv);

	void Dispatch(CommandBuffer& commandBuffer, uint x, uint y, uint z);

private:
	ShaderPtr shader;
	wstring   name;
	CBArray   cbs;
	SRVArray  srvs;
	UAVSArray uavs;
};



void ComputePass::SetShader(ShaderPtr _shader)
{
	assert(_shader->GetComputeShader() != nullptr);
	shader = _shader;
}



void ComputePass::SetCB(uint index, GPUBuffer cb)
{
	cbs[index] = cb;
}



void ComputePass::SetSRV(uint index, GPUBuffer srv)
{
	srvs[index] = srv;
}



void ComputePass::SetUAV(uint index, GPUBuffer uav)
{
	uavs[index] = uav;
}



void ComputePass::Dispatch(CommandBuffer& commandBuffer, uint x, uint y, uint z)
{
	Event event{ commandBuffer.GetRenderDevice(), name.c_str() };

	auto& context = commandBuffer.GetContext();

	std::array<ID3D11Buffer*, numCBs> cbsArray;
	std::array<ID3D11ShaderResourceView*, numSrvs> srvsArray;
	for (uint i = 0; i < numCBs; ++i)
		cbsArray[i] = static_cast<ID3D11Buffer*>(cbs[i].buffer.Get());
	for (uint i = 0; i < numSrvs; ++i)
		srvsArray[i] = srvs[i].srv;
	std::array<ID3D11UnorderedAccessView*, numUavs> uavssArray;
	std::array<UINT, numUavs> initialCounts;
	for (uint i = 0; i < numUavs; ++i)
	{
		uavssArray[i] = uavs[i].uav;
		initialCounts[i] = 0;
	}

	context->CSSetShader(shader->GetComputeShader(), nullptr, 0);
	context->CSSetConstantBuffers(0, numCBs, cbsArray.data());
	context->CSSetShaderResources(0, numSrvs, srvsArray.data());
	context->CSSetUnorderedAccessViews(0, numUavs, uavssArray.data(), initialCounts.data());
	context->Dispatch(x, y, z);

	context->ClearState();
}



}



void Voxelizer3()
{
#if 0
	std::cout << "Loading Model . . . ";
	auto model = LoadObjModuleFromFile("data/sponza/sponza.obj");
	// auto model = LoadObjModuleFromFile("data/leather_chair/leather_chair.obj");
	// auto model = LoadObjModuleFromFile("data/primitives/box.obj");
	std::cout << "DONE" << std::endl;

	//

	VoxelMap voxelMap;
	RasterizeModel(&voxelMap, *model, Scale(1.0f / 10.0f));

	const uint vxPerBrick = brickSize * brickSize * brickSize;
	const uint vxMemory = voxelMap.voxelBricks.size() * vxPerBrick + voxelMap.sparceBricks.size() * vxPerBrick * sizeof(uint32);
	std::cout << "Voxel Mem: " << vxMemory / 1024 / 1024 << "." << vxMemory / 1024 % 1024 << "Mb" << std::endl;
#endif

	///

	using namespace vox;

	ifstream ifs{ "data/voxels/platformer_test05.vox" , ios::binary };
	VoxData voxData;
	ReadFox(ifs, voxData);

	VoxelMap voxelMap;
	
	struct Transform
	{
		Vector3i position{ zero };
		uint l = 0;
	};
	VoxTransverse(voxData, voxData.nodes[0].get(), Transform(),
		[&voxelMap](const VoxData& voxData, VoxNode* node, Transform& context)
	{
		if (auto transform = VoxToTransform(node))
		{
			assert(transform->numFrames == 1);
			auto it = transform->frameAttibutes[0].find("_t");
			if (it != transform->frameAttibutes[0].end())
			{
				int x, y, z;
				sscanf_s(it->second.c_str(), "%d %d %d", &x, &y, &z);
				context.position.x += x;
				context.position.y += y;
				context.position.z += z;
			}
			it = transform->frameAttibutes[0].find("_r");
			if (it != transform->frameAttibutes[0].end())
			{
				it = it;
			}
		}
		if (auto shape = VoxToShape(node))
		{
			for (uint i = 0; i < shape->models.size(); ++i)
			{
				for (const auto& voxel : voxData.models[shape->models[i].modelId].voxels)
				{
					VoxVec3i size = voxData.models[shape->models[i].modelId].size;
					Vector3i position = context.position - Vector3i(size.x / 2, size.y / 2, size.z / 2);
					InsertVoxel(
						&voxelMap.sparceBricks,
						&voxelMap.voxelBricks,
						position + Vector3i(voxel.x, voxel.y, voxel.z),
						voxel.index);
				}
			}
		}
		++context.l;
	});
	
	/*
	VoxelMap voxelMap;
	for (const auto& voxel : voxData.models[0].voxels)
	{
		InsertVoxel(
			&voxelMap.sparceBricks,
			&voxelMap.voxelBricks,
			Vector3i(voxel.x, voxel.y, voxel.z),
			uint8(1));
	}
	*/

	///

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

	//

	VoxelMapGPU voxelMapGPU;
	voxelMapGPU.Build(renderDevice, renderDevice->GetContext(), voxelMap);

	GPUBuffer viewCB;
	GPUBuffer rayDir;
	GPUBuffer rayDirPrev;
	GPUBuffer normals;
	GPUBuffer hitT;
	GPUBuffer hitTPrev;
	GPUBuffer light;

	GPUBuffer aoHitT;
	GPUBuffer aoRayDir;
	GPUBuffer aoRayMask;
	GPUBuffer aoSunDiffuse;

	GPUBuffer sunDiffuse;

	GPUBuffer output;
	GPUBuffer outputPrev;

	GPUBuffer haltonTable;

	Vector2i rayRate{1, 1};

	struct ViewConsts
	{
		Matrix44f projViewTM;
		Matrix44f invProjViewTM;
		Matrix44f prevProjViewTM;
		Matrix44f prevInvProjViewTM;
		Vector2f  viewportSize;
		Vector2f  invViewportSize;
		Vector3f  viewPosition;
		uint      _;
		Vector3f  prevViewPosition;
		uint      frameId;
		Vector2i  rayRate;
		Vector2i  __;
	};
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.name = "viewCB";
		bufferDesc.type = GPUBufferType::ConstantBuffer;
		bufferDesc.usageFlags = GPUUsage_Dynamic;
		bufferDesc.elementStride = sizeof(ViewConsts);
		bufferDesc.elementCount = 1;
		viewCB = GPUBuffer::Create(renderDevice, bufferDesc);
	}
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.format = DXGI_FORMAT_R8G8B8A8_SNORM;
		bufferDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_UnorderedAccess;
		bufferDesc.width = canvasDesc.width;
		bufferDesc.height = canvasDesc.height;
		bufferDesc.name = "normals";
		normals = GPUBuffer::Create(renderDevice, bufferDesc);

		bufferDesc.name = "rayDir";
		bufferDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		rayDir = GPUBuffer::Create(renderDevice, bufferDesc);

		bufferDesc.name = "rayDirPrev";
		bufferDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		rayDirPrev = GPUBuffer::Create(renderDevice, bufferDesc);
	}
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.format = DXGI_FORMAT_R32_FLOAT;
		bufferDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_UnorderedAccess;
		bufferDesc.width = canvasDesc.width;
		bufferDesc.height = canvasDesc.height;
		bufferDesc.name = "hitT";
		hitT = GPUBuffer::Create(renderDevice, bufferDesc);

		bufferDesc.name = "hitTPrev";
		hitTPrev = GPUBuffer::Create(renderDevice, bufferDesc);
	}
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_UnorderedAccess;
		bufferDesc.width = canvasDesc.width / rayRate.x;
		bufferDesc.height = canvasDesc.height / rayRate.y;

		bufferDesc.format = DXGI_FORMAT_R32_FLOAT;
		bufferDesc.name = "aoHitT";
		aoHitT = GPUBuffer::Create(renderDevice, bufferDesc);

		bufferDesc.format = DXGI_FORMAT_R8G8B8A8_SNORM;
		bufferDesc.name = "aoRayDir";
		aoRayDir = GPUBuffer::Create(renderDevice, bufferDesc);

		bufferDesc.format = DXGI_FORMAT_R8_UNORM;
		bufferDesc.name = "aoRayMask";
		aoRayMask = GPUBuffer::Create(renderDevice, bufferDesc);

		bufferDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		bufferDesc.name = "aoSunDiffuse";
		aoSunDiffuse = GPUBuffer::Create(renderDevice, bufferDesc);
	}
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_UnorderedAccess;
		bufferDesc.width = canvasDesc.width / rayRate.x;
		bufferDesc.height = canvasDesc.height / rayRate.y;
		bufferDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		bufferDesc.name = "sunDiffuse";
		sunDiffuse = GPUBuffer::Create(renderDevice, bufferDesc);
	}
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		bufferDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_UnorderedAccess;
		bufferDesc.width = canvasDesc.width;
		bufferDesc.height = canvasDesc.height;
		bufferDesc.name = "light";
		light = GPUBuffer::Create(renderDevice, bufferDesc);

		bufferDesc.name = "output";
		output = GPUBuffer::Create(renderDevice, bufferDesc);

		bufferDesc.name = "outputPrev";
		outputPrev = GPUBuffer::Create(renderDevice, bufferDesc);
	}
	{
		const uint numSamples = 64;
		Recti bounds;
		bounds.max = Vector2i(64, 64);
		vipt::HaltonSampler sampler(numSamples, bounds);

		vector<Vector4<uint16>> data;
		data.resize(bounds.max.x * bounds.max.y * numSamples);
		for (uint y = 0; y < bounds.max.y; ++y)
		{
			for (uint x = 0; x < bounds.max.x; ++x)
			{
				sampler.StartPixel(Vector2i(x, y));
				for (uint z = 0; z < numSamples; ++z)
				{
					const float mult = float(numeric_limits<uint16>::max());
					sampler.SetSample(z);
					Vector4<uint16> sample;
					sample.x = uint16(sampler.Get1D() * mult);
					sample.y = uint16(sampler.Get1D() * mult);
					sample.z = uint16(sampler.Get1D() * mult);
					sample.w = uint16(sampler.Get1D() * mult);
					data[x + y * bounds.max.x + z * bounds.max.x * bounds.max.y] = sample;
				}
			}
		}

		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::Texture3D;
		bufferDesc.format = DXGI_FORMAT_R16G16B16A16_UNORM;
		bufferDesc.usageFlags = GPUUsage_ShaderResource;
		bufferDesc.width = bounds.max.x;
		bufferDesc.height = bounds.max.y;
		bufferDesc.depth = numSamples;		
		bufferDesc.dataPtr = data.data();
		bufferDesc.dataSize = bufferDesc.width * sizeof(Vector4<uint16>);
		bufferDesc.dataSlice = bufferDesc.width * bufferDesc.height * sizeof(Vector4<uint16>);
		bufferDesc.name = "haltonTable";
		haltonTable = GPUBuffer::Create(renderDevice, bufferDesc);
	}

	///

	ComputePass primaryRaysGBufferPass{ L"primaryRaysGBuffer" };
	primaryRaysGBufferPass.SetShader(shaderManager.CompileShaderFile("data/shaders/voxels3/csPrimaryRays.hlsl", "csMain", ShaderType::ComputeShader));
	primaryRaysGBufferPass.SetCB(0, viewCB);
	primaryRaysGBufferPass.SetSRV(0, voxelMapGPU.sparceBricks);
	primaryRaysGBufferPass.SetSRV(1, voxelMapGPU.voxelBricks);
	primaryRaysGBufferPass.SetUAV(0, hitT);
	primaryRaysGBufferPass.SetUAV(1, normals);
	primaryRaysGBufferPass.SetUAV(2, rayDir);
	///
	ComputePass aoRayCast{ L"aoRayCast" };
	aoRayCast.SetShader(shaderManager.CompileShaderFile("data/shaders/voxels3/csAORayCast.hlsl", "csMain", ShaderType::ComputeShader));
	aoRayCast.SetCB(0, viewCB);
	aoRayCast.SetSRV(0, voxelMapGPU.sparceBricks);
	aoRayCast.SetSRV(1, voxelMapGPU.voxelBricks);
	aoRayCast.SetSRV(2, hitT);
	aoRayCast.SetSRV(3, normals);
	aoRayCast.SetSRV(4, rayDir);
	aoRayCast.SetSRV(5, haltonTable);
	aoRayCast.SetUAV(0, aoHitT);
	aoRayCast.SetUAV(1, aoRayDir);
	aoRayCast.SetUAV(2, aoRayMask);
	aoRayCast.SetUAV(3, aoSunDiffuse);
	///
	ComputePass aoRayReuse{ L"aoRayReuse" };
	aoRayReuse.SetShader(shaderManager.CompileShaderFile("data/shaders/voxels3/csAORayReuse.hlsl", "csMain", ShaderType::ComputeShader));
	aoRayReuse.SetCB(0, viewCB);
	aoRayReuse.SetSRV(0, hitT);
	aoRayReuse.SetSRV(1, normals);
	aoRayReuse.SetSRV(2, rayDir);
	aoRayReuse.SetSRV(3, aoHitT);
	aoRayReuse.SetSRV(4, aoRayDir);
	aoRayReuse.SetSRV(5, aoRayMask);
	aoRayReuse.SetSRV(6, aoSunDiffuse);
	aoRayReuse.SetUAV(0, light);
	///
	ComputePass sunDiffusePass{ L"sunDiffuse" };
	sunDiffusePass.SetShader(shaderManager.CompileShaderFile("data/shaders/voxels3/csSunDiffuse.hlsl", "csMain", ShaderType::ComputeShader));
	sunDiffusePass.SetCB(0, viewCB);
	sunDiffusePass.SetSRV(0, voxelMapGPU.sparceBricks);
	sunDiffusePass.SetSRV(1, voxelMapGPU.voxelBricks);
	sunDiffusePass.SetSRV(2, hitT);
	sunDiffusePass.SetSRV(3, normals);
	sunDiffusePass.SetSRV(4, rayDir);
	sunDiffusePass.SetUAV(0, sunDiffuse);
	///
	ComputePass resolveDiffusePass{ L"sunResolve" };
	resolveDiffusePass.SetShader(shaderManager.CompileShaderFile("data/shaders/voxels3/csSunResolve.hlsl", "csMain", ShaderType::ComputeShader));
	resolveDiffusePass.SetCB(0, viewCB);
	resolveDiffusePass.SetSRV(0, sunDiffuse);
	resolveDiffusePass.SetSRV(1, hitT);
	resolveDiffusePass.SetSRV(2, normals);
	resolveDiffusePass.SetSRV(3, rayDir);
	resolveDiffusePass.SetUAV(0, light);
	///
	ComputePass temporalDenoisePass{ L"temporalDenoise" };
	temporalDenoisePass.SetShader(shaderManager.CompileShaderFile("data/shaders/voxels3/csTemporalDenoise.hlsl", "csMain", ShaderType::ComputeShader));
	temporalDenoisePass.SetCB(0, viewCB);
	temporalDenoisePass.SetSRV(0, hitT);
	temporalDenoisePass.SetSRV(1, normals);
	temporalDenoisePass.SetSRV(2, rayDir);
	temporalDenoisePass.SetSRV(3, light);
	temporalDenoisePass.SetSRV(4, outputPrev);
	temporalDenoisePass.SetSRV(5, rayDirPrev);
	temporalDenoisePass.SetSRV(6, hitTPrev);
	temporalDenoisePass.SetUAV(0, output);
	///
	ComputePass finalizePass{ L"finalize" };
	finalizePass.SetShader(shaderManager.CompileShaderFile("data/shaders/voxels3/csFinalize.hlsl", "csMain", ShaderType::ComputeShader));
	finalizePass.SetSRV(0, output);
	finalizePass.SetUAV(0, swapChain->GetBackBuffer());
	///

	OrbitalCamera camera(canvas);
//	camera.SetPerspective(1.4f, 1.0f, 1000.0f);
	camera.SetPerspective(0.9f, 1.0f, 1024.0f * 8.0f);
	/*
	camera.SetCamera(
		Vector3f(388.397919f, -5.89890289f, 122.766502f),
		Quaternionf(-0.683579028f, 0.142710492f, -0.159888670f, 0.697763085f),
		310.844086f);
	*/
	camera.SetCamera(
		Vector3f(217.062759f, 242.156372f, 21.3335361f),
		Quaternionf(-0.00298559666f, 0.0131207705f, 0.513732135f, 0.857935369f),
		310.844086f);

	ViewConsts viewConsts;
	Matrix44f projTM = camera.PerspectiveMatrix();
	Matrix44f viewTM = camera.ViewMatrix();
	viewConsts.projViewTM = Mul(viewTM, projTM);
	viewConsts.invProjViewTM = Inverse(viewConsts.projViewTM);
	viewConsts.viewportSize = Vector2f(canvasDesc.width, canvasDesc.height);
	viewConsts.invViewportSize = Vector2f(1.0f / canvasDesc.width, 1.0f / canvasDesc.height);
	viewConsts.viewPosition = camera.Position();
	viewConsts.frameId = 0;
	viewConsts.rayRate = rayRate;

	TimeCounter counter;
	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		Time deltaTime = counter.GetElapsed();

		camera.Update(deltaTime);
		wikiDraw.PushProjViewTM(Mul(camera.ViewMatrix(), camera.PerspectiveMatrix()));
		wikiDraw.PushWorldTM(Matrix44f(zero));

		Matrix44f projTM = camera.PerspectiveMatrix();
		Matrix44f viewTM = camera.ViewMatrix();
		Matrix44f projViewTM = Mul(viewTM, projTM);
		viewConsts.prevProjViewTM = viewConsts.projViewTM;
		viewConsts.prevInvProjViewTM = viewConsts.invProjViewTM;
		viewConsts.prevViewPosition = viewConsts.viewPosition;
		viewConsts.projViewTM = Mul(viewTM, projTM);
		viewConsts.invProjViewTM = Inverse(viewConsts.projViewTM);
		viewConsts.viewportSize = Vector2f(canvasDesc.width, canvasDesc.height);
		viewConsts.invViewportSize = Vector2f(1.0f / canvasDesc.width, 1.0f / canvasDesc.height);
		viewConsts.viewPosition = camera.Position();
		viewConsts.frameId++;
		viewCB.Update(renderDevice->GetContext(), viewConsts);

		commandBuffer.GetContext()->CopyResource(hitTPrev.buffer, hitT.buffer);
		commandBuffer.GetContext()->CopyResource(outputPrev.buffer, output.buffer);
		commandBuffer.GetContext()->CopyResource(rayDirPrev.buffer, rayDir.buffer);
		
		primaryRaysGBufferPass.Dispatch(
			commandBuffer,
			canvasDesc.width / 8,
			canvasDesc.height / 8,
			1);

		aoRayCast.Dispatch(
			commandBuffer,
			(canvasDesc.width / rayRate.x) / 8,
			(canvasDesc.height / rayRate.y) / 8,
			1);

		aoRayReuse.Dispatch(
			commandBuffer,
			canvasDesc.width / 8,
			canvasDesc.height / 8,
			1);

		sunDiffusePass.Dispatch(
			commandBuffer,
			(canvasDesc.width / rayRate.x) / 8,
			(canvasDesc.height / rayRate.y) / 8,
			1);

		resolveDiffusePass.Dispatch(
			commandBuffer,
			canvasDesc.width / 8,
			canvasDesc.height / 8,
			1);

		temporalDenoisePass.Dispatch(
			commandBuffer,
			canvasDesc.width / 8,
			canvasDesc.height / 8,
			1);

		finalizePass.Dispatch(
			commandBuffer,
			canvasDesc.width / 128,
			canvasDesc.height / 1,
			1);

		swapChain->Present();

		Time time = GetTime();
	}
}
