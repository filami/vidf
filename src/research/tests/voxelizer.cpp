#include "pch.h"
#include "gmMachine.h"
#include "gmArrayLib.h"
#include "gmCall.h"
#include "gmGCRoot.h"
#include "gmGCRootUtil.h"
#include "gmHelpers.h"
#include "gmMathLib.h"
#include "gmStringLib.h"
#include "gmSystemLib.h"
#include "gmVector3Lib.h"
#include "proto/mesh.h"
#include "common/plane.h"

using namespace vidf;
using namespace proto;

struct Triangle
{
	Vector3f v0, v1, v2;
	Vector3f operator[] (uint idx) const { return static_cast<const Vector3f*>(&v0)[idx]; }
	Vector3f& operator[] (uint idx) { return static_cast<Vector3f*>(&v0)[idx]; }
};

bool BoxTriangleIntersect(const Boxf& box, const Triangle& triangle)
{
	const Planef planes[6] =
	{
		Planef(Vector3f(1.0f, 0.0f, 0.0f),  box.min.x),
		Planef(Vector3f(-1.0f, 0.0f, 0.0f), -box.max.x),
		Planef(Vector3f(0.0f, 1.0f, 0.0f),  box.min.y),
		Planef(Vector3f(0.0f,-1.0f, 0.0f), -box.max.y),
		Planef(Vector3f(0.0f, 0.0f, 1.0f),  box.min.z),
		Planef(Vector3f(0.0f, 0.0f,-1.0f), -box.max.z),
	};
	uint flags[3] = {};
	for (uint i = 0; i < 6; ++i)
	{
		const Planef plane = planes[i];
		uint planeFlags = 1 << i;
		for (uint j = 0; j < 3; ++j)
		{
			const Vector3f vertex = triangle[j];
			if (Distance(plane, vertex) >= 0.0f)
				flags[j] |= planeFlags;
		}
	}
	return (flags[0] | flags[1] | flags[2]) == 0x3f;
}

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
	Vector3i position;
};

struct RasterData
{
	std::vector<Fragment> fragments;
	float voxelSize;
};

void RasterizeTriangle(RasterData* raster, Triangle triangle)
{
	const float voxelSize = raster->voxelSize;
	const float invVoxelSize = 1.0f / voxelSize;
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
				fragment.position = Vector3i(voxel.min * invVoxelSize);
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

		Triangle triangle;
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

void DrawFragments(const RasterData& raster)
{
	const float voxelSz = raster.voxelSize;
	const Vector3f voxelSizeHalf = Vector3f(voxelSz, voxelSz, voxelSz) * 0.5f;

	/*/
	glColor4ub(255, 128, 64, 255);
	glBegin(GL_LINES);
	for (const Fragment frag : raster.fragments)
	{
	Vector3f vert = Vector3f(frag.position) * voxelSz + voxelSizeHalf;

	glVertex3f(vert.x           , vert.y           , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z           );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z + voxelSz );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z + voxelSz );
	glVertex3f(vert.x           , vert.y           , vert.z + voxelSz );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z + voxelSz );

	glVertex3f(vert.x           , vert.y           , vert.z           );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z + voxelSz );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z + voxelSz );
	glVertex3f(vert.x           , vert.y           , vert.z + voxelSz );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z + voxelSz );

	glVertex3f(vert.x           , vert.y           , vert.z           );
	glVertex3f(vert.x           , vert.y           , vert.z + voxelSz );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y           , vert.z + voxelSz );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x + voxelSz , vert.y + voxelSz , vert.z + voxelSz );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z           );
	glVertex3f(vert.x           , vert.y + voxelSz , vert.z + voxelSz );
	}
	glEnd();
	/**/

	glBegin(GL_QUADS);
	for (const Fragment frag : raster.fragments)
	{
		Vector3f vert = Vector3f(frag.position) * voxelSz;

		glColor4ub(255, 128, 0, 255);
		glVertex3f(vert.x, vert.y, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z);

		glColor4ub(0, 128, 255, 255);
		glVertex3f(vert.x, vert.y, vert.z + voxelSz);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z + voxelSz);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z + voxelSz);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z + voxelSz);

		glColor4ub(128, 255, 0, 255);
		glVertex3f(vert.x, vert.y, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z + voxelSz);
		glVertex3f(vert.x, vert.y, vert.z + voxelSz);

		glColor4ub(128, 0, 255, 255);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z + voxelSz);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z + voxelSz);

		glColor4ub(0, 255, 128, 255);
		glVertex3f(vert.x, vert.y, vert.z);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z);
		glVertex3f(vert.x, vert.y + voxelSz, vert.z + voxelSz);
		glVertex3f(vert.x, vert.y, vert.z + voxelSz);

		glColor4ub(255, 0, 128, 255);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z);
		glVertex3f(vert.x + voxelSz, vert.y + voxelSz, vert.z + voxelSz);
		glVertex3f(vert.x + voxelSz, vert.y, vert.z + voxelSz);
	}
	glEnd();
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
	// raster.voxelSize = 20.0f;
	raster.voxelSize = 2.0f;
	// raster.voxelSize = 0.5f;
	RasterizeModel(&raster, *model);
	std::cout << "DONE" << std::endl;

	Vector3f center = Vector3f(raster.fragments[0].position);
	float dist = 0.0f;
	for (auto fragment : raster.fragments)
		center = center + Vector3f(fragment.position);
	center = center * (1.0f / raster.fragments.size()) * raster.voxelSize;
	for (auto fragment : raster.fragments)
		dist = Max(dist, Distance(center, Vector3f(fragment.position) * raster.voxelSize));

	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc());

	OrbitalCamera camera(protoGL.GetCanvas());
	camera.SetPerspective(1.4f, 1.0f, dist * 10.0f);
	camera.SetCamera(center, Quaternionf(zero), dist * 2.0f);

	glEnable(GL_DEPTH_TEST);

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

		protoGL.Swap();
	}
}
