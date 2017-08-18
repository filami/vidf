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

using namespace vidf;
using namespace proto;
using namespace dx11;

//////////////////////////////////////////////////////////////////////////

// const float voxelSize = 50.0f;

const float voxelSize = 2.0f;
typedef uint8 SparceMapData;
const uint sparceMapSize = 8;
const uint sparceMapLevels = 3;

/*
const float voxelSize = 2.0f;
typedef uint8 SparceMapData;
const uint sparceMapSize = 2;
const uint sparceMapLevels = 9;
*/
const float sparceMapDimension = std::pow(sparceMapSize, sparceMapLevels) * voxelSize;
const Boxf sparceMapBox = Boxf(
	Vector3f(-sparceMapDimension, -sparceMapDimension, -sparceMapDimension) * 0.5f,
	Vector3f(sparceMapDimension, sparceMapDimension, sparceMapDimension) * 0.5f);

//////////////////////////////////////////////////////////////////////////

void DrawModelGeometry(WikiDraw* wikiDraw, const Module& model, uint geomIdx)
{
	const Module::Geometry& geometry = model.GetGeometry(geomIdx);

	wikiDraw->Begin(WikiDraw::Lines);
	const uint lastPolygon = geometry.firstPolygon + geometry.numPolygons;
	for (uint polyIdx = geometry.firstPolygon; polyIdx != lastPolygon; ++polyIdx)
	{
		const uint numVertices = model.GetPolygonNumVertices(polyIdx);

		const Vector3f first = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, 0));
		Vector3f last = first;
		for (uint vertIdx = 1; vertIdx < numVertices; ++vertIdx)
		{
			const Vector3f cur = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, vertIdx));
			wikiDraw->PushVertex(last);
			wikiDraw->PushVertex(cur);
			last = cur;
		}
		wikiDraw->PushVertex(last);
		wikiDraw->PushVertex(first);
	}
	wikiDraw->End();
}

void DrawModel(WikiDraw* wikiDraw, const Module& model)
{
	wikiDraw->SetColor(0, 0, 0, 255);
	const uint numGeoms = model.GetNumGeometries();
	for (uint geomIdx = 0; geomIdx < numGeoms; ++geomIdx)
		DrawModelGeometry(wikiDraw, model, geomIdx);
}

struct Fragment
{
	Vector3f position;
};

struct RasterData
{
	std::vector<Fragment> fragments;
	float voxelSize;
};

template<typename T, uint size>
struct SparceMap
{
	SparceMap()
	{
		memset(dataBrick.data(), 0, sizeof(dataBrick));
	}

	typedef std::unique_ptr<SparceMap<T, size>> SubMapPtr;
	std::array<std::array<std::array<SubMapPtr, size>, size>, size> subMapBrick;
	std::array<std::array<std::array<T, size>, size>, size> dataBrick;
};

void RasterizeTriangle(RasterData* raster, Trianglef triangle)
{
	const float voxelSize = raster->voxelSize;
	const Vector3f voxelSize3 = Vector3f(voxelSize, voxelSize, voxelSize);

	const auto Floor = [](Vector3f v)
	{
		return Vector3f(std::floor(v.x), std::floor(v.y), std::floor(v.z));
	};
	const auto Ceil = [](Vector3f v)
	{
		return Vector3f(std::ceil(v.x), std::ceil(v.y), std::ceil(v.z));
	};

	Boxf box = Boxf(
		Floor(Min(Min(triangle.v0, triangle.v1), triangle.v2)/voxelSize)*voxelSize,
		Ceil(Max(Max(triangle.v0+Vector3f(voxelSize, voxelSize, voxelSize), triangle.v1), triangle.v2)/voxelSize)*voxelSize);
	/*
	box.min = Min(Min(triangle.v0, triangle.v1), triangle.v2);
	box.max = Max(Max(triangle.v0, triangle.v1), triangle.v2);
	box.min.x = std::floor(box.min.x);
	box.min.y = std::floor(box.min.y);
	box.min.z = std::floor(box.min.z);
	*/

	for (float z = box.min.z; z < box.max.z; z += voxelSize)
	{
		for (float y = box.min.y; y < box.max.y; y += voxelSize)
		{
			for (float x = box.min.x; x < box.max.x; x += voxelSize)
			{
				Boxf voxel;
				voxel.min = Vector3f(x, y, z);
				voxel.max = voxel.min + voxelSize3;
				if (!BoxTriangleIntersect(voxel, triangle))
					continue;
				Fragment fragment;
				fragment.position = Vector3f((voxel.max + voxel.min) * 0.5f);
				raster->fragments.push_back(fragment);
			}
		}
	}
}

void RasterizeModelGeometry(RasterData* raster, const Module& model, const Matrix44f& objectTM, uint geomIdx)
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
			RasterizeTriangle(raster, transTriangle);
			triangle.v1 = triangle.v2;
		}
	}
}

void RasterizeModel(RasterData* raster, const Module& model, const Matrix44f& objectTM)
{
	const uint numGeoms = model.GetNumGeometries();
	for (uint geomIdx = 0; geomIdx < numGeoms; ++geomIdx)
		RasterizeModelGeometry(raster, model, objectTM, geomIdx);
}

//////////////////////////////////////////////////////////////////////////

void SparceTreeInsertPoint(SparceMap<SparceMapData, sparceMapSize>* map, Boxf box, Vector3f point, uint level)
{
	const Vector3i mappedPoint = Vector3i((point - box.min) / (box.max - box.min) * sparceMapSize);

	const Vector3f voxelSize = (box.max - box.min) / sparceMapSize;
	if (level <= 1)
	{
		map->dataBrick[mappedPoint.x][mappedPoint.y][mappedPoint.z] = 1;
	}
	else
	{
		Boxf subBox;
		subBox.min = box.min + Vector3f(mappedPoint) * voxelSize;
		subBox.max = subBox.min + voxelSize;
		auto& subMap = map->subMapBrick[mappedPoint.x][mappedPoint.y][mappedPoint.z];
		if (!subMap)
			subMap = std::make_unique<SparceMap<SparceMapData, sparceMapSize>>();
		map->dataBrick[mappedPoint.x][mappedPoint.y][mappedPoint.z] = 2;
		SparceTreeInsertPoint(subMap.get(), subBox, point, level - 1);
	}
}

void SparceTreeClearData(SparceMap<SparceMapData, sparceMapSize>* map)
{
	std::memset(map->dataBrick.data(), 0, sizeof(map->dataBrick));
	for (uint z = 0; z < sparceMapSize; ++z)
	{
		for (uint y = 0; y < sparceMapSize; ++y)
		{
			for (uint x = 0; x < sparceMapSize; ++x)
			{
				if (map->subMapBrick[x][y][z])
					SparceTreeClearData(map->subMapBrick[x][y][z].get());
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void DrawBoxWireframe(WikiDraw* wikiDraw, Boxf box)
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

void DrawBoxFilled(WikiDraw* wikiDraw, Boxf box)
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

void DrawSparceMap(WikiDraw* wikiDraw, const SparceMap<SparceMapData, sparceMapSize>& map, Boxf box)
{
	wikiDraw->SetColor(255, 0, 0, 255);
	DrawBoxWireframe(wikiDraw, box);

	const Vector3f voxelSize = (box.max - box.min) / sparceMapSize;
	for (uint z = 0; z < sparceMapSize; ++z)
	{
		for (uint y = 0; y < sparceMapSize; ++y)
		{
			for (uint x = 0; x < sparceMapSize; ++x)
			{
				Boxf subBox;
				subBox.min = box.min + Vector3f(x, y, z) * voxelSize;
				subBox.max = subBox.min + voxelSize;

				if (map.dataBrick[x][y][z] == 2)
				{
					DrawSparceMap(wikiDraw, *map.subMapBrick[x][y][z], subBox);
				}
				else if (map.dataBrick[x][y][z] == 1)
				{
					Vector3f center = (subBox.min + subBox.max) * 0.5f;
					subBox.min = center - voxelSize * 0.45f;
					subBox.max = center + voxelSize * 0.45f;
					DrawBoxFilled(wikiDraw, subBox);
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

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

void Voxelizer()
{
	std::cout << "Loading Model . . . ";
	// auto model = LoadObjModuleFromFile("data/sponza/sponza.obj");
	auto model = LoadObjModuleFromFile("data/leather_chair/leather_chair.obj");
	// auto model = LoadObjModuleFromFile("data/primitives/box.obj");
	std::cout << "DONE" << std::endl;

	std::cout << "Rasterizing . . . ";
	RasterData raster;
	raster.voxelSize = voxelSize;
	RasterizeModel(&raster, *model, Matrix44f(zero));
	std::cout << "DONE" << std::endl;

	std::cout << "Generating Sparce Map . . . ";
	SparceMap<SparceMapData, sparceMapSize> sparceMap;
	for (auto fragment : raster.fragments)
		SparceTreeInsertPoint(&sparceMap, sparceMapBox, fragment.position, sparceMapLevels);
	std::cout << "DONE" << std::endl;
	
	Vector3f center = Vector3f(zero);
	float dist = 10.0f;
	if (!raster.fragments.empty())
	{
		for (auto fragment : raster.fragments)
			center = center + fragment.position;
		center = center * (1.0f / raster.fragments.size());
		dist = 0.0f;
		for (auto fragment : raster.fragments)
			dist = Max(dist, Distance(center, fragment.position));
	}

	RenderDevicePtr renderDevice = RenderDevice::Create(RenderDeviceDesc());
	ShaderManager shaderManager(renderDevice);
	CommandBuffer commandBuffer(renderDevice);
	WikiDraw wikiDraw(renderDevice, &shaderManager);

	Dx11CanvasListener canvasListener;
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

	OrbitalCamera camera(canvas);
	camera.SetPerspective(1.4f, 1.0f, sparceMapDimension * 2.0f);
	camera.SetCamera(center, Quaternionf(zero), dist * 2.0f);

	TimeCounter counter;
	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		Time deltaTime = counter.GetElapsed();

		camera.Update(deltaTime);
		wikiDraw.PushProjViewTM(Mul(camera.ViewMatrix(), camera.PerspectiveMatrix()));
		wikiDraw.PushWorldTM(Matrix44f(zero));

		Time time = GetTime();
		Matrix44f objectTM = ToMatrix44(QuaternionAxisAngle(Vector3f(0.0f, 1.0f, 0.0f), time.AsFloat()*0.15f));
		
		raster.fragments.clear();
		SparceTreeClearData(&sparceMap);
		RasterizeModel(&raster, *model, objectTM);
		for (auto fragment : raster.fragments)
			SparceTreeInsertPoint(&sparceMap, sparceMapBox, fragment.position, sparceMapLevels);

		FLOAT white[] = {1.0f, 1.0f, 1.0f, 1.0f};
		renderDevice->GetContext()->ClearRenderTargetView(swapChain->GetBackBufferRTV(), white);
		renderDevice->GetContext()->ClearDepthStencilView(depthStencil.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

		commandBuffer.BeginRenderPass(renderPass);

		DrawSparceMap(&wikiDraw, sparceMap, sparceMapBox);
		wikiDraw.PushWorldTM(objectTM);
		DrawModel(&wikiDraw, *model);
		wikiDraw.Flush(&commandBuffer);

		commandBuffer.EndRenderPass();

		swapChain->Present();
	}
}
