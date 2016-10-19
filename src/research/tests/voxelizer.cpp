#include "pch.h"
#include "proto/mesh.h"
#include "common/plane.h"
#include "common/intersect.h"

using namespace vidf;
using namespace proto;

//////////////////////////////////////////////////////////////////////////

void DrawModelGeometry(const Module& model, uint geomIdx)
{
	const Module::Geometry& geometry = model.GetGeometry(geomIdx);

	glBegin(GL_LINES);
	const uint lastPolygon = geometry.firstPolygon + geometry.numPolygons;
	for (uint polyIdx = geometry.firstPolygon; polyIdx != lastPolygon; ++polyIdx)
	{
		const uint numVertices = model.GetPolygonNumVertices(polyIdx);

		const Vector3f first = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, 0));
		Vector3f last = first;
		for (uint vertIdx = 1; vertIdx < numVertices; ++vertIdx)
		{
			const Vector3f cur = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, vertIdx));
			glVertex3fv(&last.x);
			glVertex3fv(&cur.x);
			last = cur;
		}
		glVertex3fv(&last.x);
		glVertex3fv(&first.x);
	}
	glEnd();
}

void DrawModel(const Module& model)
{
	glColor4ub(0, 0, 0, 255);
	const uint numGeoms = model.GetNumGeometries();
	for (uint geomIdx = 0; geomIdx < numGeoms; ++geomIdx)
		DrawModelGeometry(model, geomIdx);
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

const float voxelSize = 2.0f;
typedef uint8 SparceMapData;
const uint sparceMapSize = 8;
const uint sparceMapLevels = 3;
const float sparceMapDimension = voxelSize * float(std::pow(sparceMapLevels, sparceMapSize));
const Boxf sparceMapBox = Boxf(
	Vector3f(-sparceMapDimension, -sparceMapDimension, -sparceMapDimension) * 0.5f,
	Vector3f( sparceMapDimension,  sparceMapDimension,  sparceMapDimension) * 0.5f);

void RasterizeTriangle(RasterData* raster, Trianglef triangle)
{
	const float voxelSize = raster->voxelSize;
	const Vector3f voxelSize3 = Vector3f(voxelSize, voxelSize, voxelSize);

	Boxf box;
	box.min = Min(Min(triangle.v0, triangle.v1), triangle.v2);
	box.max = Max(Max(triangle.v0, triangle.v1), triangle.v2);
	box.min.x = std::floor(box.min.x);
	box.min.y = std::floor(box.min.y);
	box.min.z = std::floor(box.min.z);

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

void RasterizeModelGeometry(RasterData* raster, const Module& model, uint geomIdx)
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
			RasterizeTriangle(raster, triangle);
			triangle.v1 = triangle.v2;
		}
	}
}

void RasterizeModel(RasterData* raster, const Module& model)
{
	const uint numGeoms = model.GetNumGeometries();
	for (uint geomIdx = 0; geomIdx < numGeoms; ++geomIdx)
		RasterizeModelGeometry(raster, model, geomIdx);
}

//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////

void DrawBoxWireframe(Boxf box)
{
	glBegin(GL_LINES);

	glVertex3f(box.min.x, box.min.y, box.min.z);
	glVertex3f(box.max.x, box.min.y, box.min.z);
	glVertex3f(box.min.x, box.max.y, box.min.z);
	glVertex3f(box.max.x, box.max.y, box.min.z);
	glVertex3f(box.min.x, box.max.y, box.max.z);
	glVertex3f(box.max.x, box.max.y, box.max.z);
	glVertex3f(box.min.x, box.min.y, box.max.z);
	glVertex3f(box.max.x, box.min.y, box.max.z);

	glVertex3f(box.min.x, box.min.y, box.min.z);
	glVertex3f(box.min.x, box.max.y, box.min.z);
	glVertex3f(box.max.x, box.min.y, box.min.z);
	glVertex3f(box.max.x, box.max.y, box.min.z);
	glVertex3f(box.max.x, box.min.y, box.max.z);
	glVertex3f(box.max.x, box.max.y, box.max.z);
	glVertex3f(box.min.x, box.min.y, box.max.z);
	glVertex3f(box.min.x, box.max.y, box.max.z);

	glVertex3f(box.min.x, box.min.y, box.min.z);
	glVertex3f(box.min.x, box.min.y, box.max.z);
	glVertex3f(box.max.x, box.min.y, box.min.z);
	glVertex3f(box.max.x, box.min.y, box.max.z);
	glVertex3f(box.max.x, box.max.y, box.min.z);
	glVertex3f(box.max.x, box.max.y, box.max.z);
	glVertex3f(box.min.x, box.max.y, box.min.z);
	glVertex3f(box.min.x, box.max.y, box.max.z);

	glEnd();
}

void DrawBoxFilled(Boxf box)
{
	const auto glVertex = [](Vector3f v)
	{
		glVertex3f(v.x, v.y, v.z);
	};

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

	glBegin(GL_QUADS);

	glColor4ub(255, 128, 0, 255);
	glVertex(vertices[0]);
	glVertex(vertices[2]);
	glVertex(vertices[3]);
	glVertex(vertices[1]);

	glColor4ub(0, 128, 255, 255);
	glVertex(vertices[4]);
	glVertex(vertices[5]);
	glVertex(vertices[7]);
	glVertex(vertices[6]);

	glColor4ub(128, 255, 0, 255);
	glVertex(vertices[0]);
	glVertex(vertices[4]);
	glVertex(vertices[6]);
	glVertex(vertices[2]);

	glColor4ub(128, 0, 255, 255);
	glVertex(vertices[1]);
	glVertex(vertices[3]);
	glVertex(vertices[7]);
	glVertex(vertices[5]);

	glColor4ub(0, 255, 128, 255);
	glVertex(vertices[0]);
	glVertex(vertices[1]);
	glVertex(vertices[5]);
	glVertex(vertices[4]);

	glColor4ub(255, 0, 128, 255);
	glVertex(vertices[2]);
	glVertex(vertices[6]);
	glVertex(vertices[7]);
	glVertex(vertices[3]);

	glEnd();
}

void DrawFragments(const RasterData& raster)
{
	const float voxelSz = raster.voxelSize;
	const Vector3f voxelSize = Vector3f(voxelSz, voxelSz, voxelSz);

	glColor4ub(255, 128, 64, 255);
	for (const Fragment frag : raster.fragments)
	{
		Vector3f vertex = Vector3f(frag.position);
		Boxf box = Boxf(vertex, vertex + voxelSize);
		// DrawBoxWireframe(box);
		DrawBoxFilled(box);
	}
}

void DrawSparceMap(const SparceMap<SparceMapData, sparceMapSize>& map, Boxf box)
{
	glColor4ub(255, 0, 0, 255);
	DrawBoxWireframe(box);

	const float invBoxSize = 1.0f / sparceMapSize;
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

				if (map.subMapBrick[x][y][z])
					DrawSparceMap(*map.subMapBrick[x][y][z], subBox);
				else if (map.dataBrick[x][y][z] != 0)
					DrawBoxFilled(box);
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////

void Voxelizer()
{
	std::cout << "Loading Model . . . ";
	// auto model = LoadObjModuleFromFile("sponza/sponza.obj");
	auto model = LoadObjModuleFromFile("data/leather_chair/leather_chair.obj");
	std::cout << "DONE" << std::endl;

	std::cout << "Voxelizing . . . ";
	RasterData raster;
	raster.voxelSize = voxelSize;
	RasterizeModel(&raster, *model);

	SparceMap<SparceMapData, sparceMapSize> sparceMap;

	std::cout << "DONE" << std::endl;

	Vector3f center = Vector3f(raster.fragments[0].position);
	float dist = 0.0f;
	for (auto fragment : raster.fragments)
		center = center + fragment.position;
	center = center * (1.0f / raster.fragments.size());
	for (auto fragment : raster.fragments)
		dist = Max(dist, Distance(center, fragment.position));

	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc());

	OrbitalCamera camera(protoGL.GetCanvas());
	camera.SetPerspective(1.4f, 1.0f, sparceMapDimension * 2.0f);
	camera.SetCamera(center, Quaternionf(zero), dist * 2.0f);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	TimeCounter counter;
	while (protoGL.Update())
	{
		Time deltaTime = counter.GetElapsed();

		camera.Update(deltaTime);
		camera.CommitToGL();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		DrawModel(*model);
		DrawFragments(raster);
		DrawSparceMap(sparceMap, sparceMapBox);

		protoGL.Swap();
	}
}
