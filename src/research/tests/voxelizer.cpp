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

//////////////////////////////////////////////////////////////////////////
#if 0
/********************************************************/
/* AABB-triangle overlap test code                      */
/* by Tomas Akenine-Möller                              */
/* Function: int triBoxOverlap(float boxcenter[3],      */
/*          float boxhalfsize[3],float triverts[3][3]); */
/* History:                                             */
/*   2001-03-05: released the code in its first version */
/*   2001-06-18: changed the order of the tests, faster */
/*                                                      */
/* Acknowledgement: Many thanks to Pierre Terdiman for  */
/* suggestions and discussions on how to optimize code. */
/* Thanks to David Hunt for finding a ">="-bug!         */
/********************************************************/

#include <math.h>
#include <stdio.h>
/*
#define X 0
#define Y 1
#define Z 2

#define CROSS(dest,v1,v2) \
dest[0] = v1[1] * v2[2] - v1[2] * v2[1]; \
dest[1] = v1[2] * v2[0] - v1[0] * v2[2]; \
dest[2] = v1[0] * v2[1] - v1[1] * v2[0];

#define DOT(v1,v2) (v1[0]*v2[0]+v1[1]*v2[1]+v1[2]*v2[2])



#define SUB(dest,v1,v2) \

dest[0] = v1[0] - v2[0]; \

dest[1] = v1[1] - v2[1]; \

dest[2] = v1[2] - v2[2];
*/

/*
#define FINDMINMAX(x0,x1,x2,min,max) \

min = max = x0;   \

if (x1<min) min = x1; \

if (x1>max) max = x1; \

if (x2<min) min = x2; \

if (x2>max) max = x2;*/


template<typename T>
T FindMin(T x0, T x1, T x2)
{
	return Min(Min(x0, x1), x2);
}



template<typename T>
T FindMax(T x0, T x1, T x2)
{
	return Max(Max(x0, x1), x2);
}



bool PlaneBoxOverlap(Vector3f normal, Vector3f vert, Vector3f maxbox)
{
	Vector3f vmin, vmax;

	for (uint i = 0; i < 3; ++i)
	{
		if (normal[i] > 0.0f)
		{
			vmin[i] = -maxbox[i] - vert[i];
			vmax[i] = maxbox[i] - vert[i];
		}
		else
		{
			vmin[i] = maxbox[i] - vert[i];
			vmax[i] = -maxbox[i] - vert[i];
		}
	}

	if (Dot(normal, vmin) > 0.0f)
		return false;

	if (Dot(normal, vmax) >= 0.0f)
		return true;

	return false;
}



#if 0

/*======================== X-tests ========================*/

#define AXISTEST_X01(a, b, fa, fb)			   \

p0 = a*v0[Y] - b*v0[Z];			       	   \

p2 = a*v2[Y] - b*v2[Z];			       	   \

if (p0 < p2) { min = p0; max = p2; }
else { min = p2; max = p0; } \

rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \

if (min > rad || max < -rad) return 0;



#define AXISTEST_X2(a, b, fa, fb)			   \

p0 = a*v0[Y] - b*v0[Z];			           \

p1 = a*v1[Y] - b*v1[Z];			       	   \

if (p0 < p1) { min = p0; max = p1; }
else { min = p1; max = p0; } \

rad = fa * boxhalfsize[Y] + fb * boxhalfsize[Z];   \

if (min > rad || max < -rad) return 0;



/*======================== Y-tests ========================*/

#define AXISTEST_Y02(a, b, fa, fb)			   \

p0 = -a*v0[X] + b*v0[Z];		      	   \

p2 = -a*v2[X] + b*v2[Z];	       	       	   \

if (p0 < p2) { min = p0; max = p2; }
else { min = p2; max = p0; } \

rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \

if (min > rad || max < -rad) return 0;



#define AXISTEST_Y1(a, b, fa, fb)			   \

p0 = -a*v0[X] + b*v0[Z];		      	   \

p1 = -a*v1[X] + b*v1[Z];	     	       	   \

if (p0 < p1) { min = p0; max = p1; }
else { min = p1; max = p0; } \

rad = fa * boxhalfsize[X] + fb * boxhalfsize[Z];   \

if (min > rad || max < -rad) return 0;



/*======================== Z-tests ========================*/



#define AXISTEST_Z12(a, b, fa, fb)			   \

p1 = a*v1[X] - b*v1[Y];			           \

p2 = a*v2[X] - b*v2[Y];			       	   \

if (p2 < p1) { min = p2; max = p1; }
else { min = p1; max = p2; } \

rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \

if (min > rad || max < -rad) return 0;



#define AXISTEST_Z0(a, b, fa, fb)			   \

p0 = a*v0[X] - b*v0[Y];				   \

p1 = a*v1[X] - b*v1[Y];			           \

if (p0 < p1) { min = p0; max = p1; }
else { min = p1; max = p0; } \

rad = fa * boxhalfsize[X] + fb * boxhalfsize[Y];   \

if (min > rad || max < -rad) return 0;

#endif


template<uint axis0, uint axis1>
bool AxisTest(Vector3f v0, Vector3f v1)
{
	const float p0 = a*v0[axis0] - b*v0[axis1];
	const float p1 = a*v1[axis0] - b*v1[axis1];

	if (p0 < p1)
	{
		min = p0;
		max = p1;
	}
	else
	{
		min = p1;
		max = p0;
	}

	const float rad = fa * boxhalfsize[axis0] + fb * boxhalfsize[axis1];

	return min <= rad && max >= -rad;
}



#if 0

int triBoxOverlap(float boxcenter[3], float boxhalfsize[3], float triverts[3][3])

{



	/*    use separating axis theorem to test overlap between triangle and box */

	/*    need to test for overlap in these directions: */

	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */

	/*       we do not even need to test these) */

	/*    2) normal of the triangle */

	/*    3) crossproduct(edge from tri, {x,y,z}-directin) */

	/*       this gives 3x3=9 more tests */

	float v0[3], v1[3], v2[3];

	//   float axis[3];

	float min, max, p0, p1, p2, rad, fex, fey, fez;		// -NJMP- "d" local variable removed

	float normal[3], e0[3], e1[3], e2[3];



	/* This is the fastest branch on Sun */

	/* move everything so that the boxcenter is in (0,0,0) */

	SUB(v0, triverts[0], boxcenter);

	SUB(v1, triverts[1], boxcenter);

	SUB(v2, triverts[2], boxcenter);



	/* compute triangle edges */

	SUB(e0, v1, v0);      /* tri edge 0 */

	SUB(e1, v2, v1);      /* tri edge 1 */

	SUB(e2, v0, v2);      /* tri edge 2 */



												/* Bullet 3:  */

												/*  test the 9 tests first (this was faster) */

	fex = fabsf(e0[X]);

	fey = fabsf(e0[Y]);

	fez = fabsf(e0[Z]);

	AXISTEST_X01(e0[Z], e0[Y], fez, fey);

	AXISTEST_Y02(e0[Z], e0[X], fez, fex);

	AXISTEST_Z12(e0[Y], e0[X], fey, fex);



	fex = fabsf(e1[X]);

	fey = fabsf(e1[Y]);

	fez = fabsf(e1[Z]);

	AXISTEST_X01(e1[Z], e1[Y], fez, fey);

	AXISTEST_Y02(e1[Z], e1[X], fez, fex);

	AXISTEST_Z0(e1[Y], e1[X], fey, fex);



	fex = fabsf(e2[X]);

	fey = fabsf(e2[Y]);

	fez = fabsf(e2[Z]);

	AXISTEST_X2(e2[Z], e2[Y], fez, fey);

	AXISTEST_Y1(e2[Z], e2[X], fez, fex);

	AXISTEST_Z12(e2[Y], e2[X], fey, fex);



	/* Bullet 1: */

	/*  first test overlap in the {x,y,z}-directions */

	/*  find min, max of the triangle each direction, and test for overlap in */

	/*  that direction -- this is equivalent to testing a minimal AABB around */

	/*  the triangle against the AABB */



	/* test in X-direction */

	FINDMINMAX(v0[X], v1[X], v2[X], min, max);

	if (min > boxhalfsize[X] || max < -boxhalfsize[X]) return 0;



	/* test in Y-direction */

	FINDMINMAX(v0[Y], v1[Y], v2[Y], min, max);

	if (min > boxhalfsize[Y] || max < -boxhalfsize[Y]) return 0;



	/* test in Z-direction */

	FINDMINMAX(v0[Z], v1[Z], v2[Z], min, max);

	if (min > boxhalfsize[Z] || max < -boxhalfsize[Z]) return 0;



	/* Bullet 2: */

	/*  test if the box intersects the plane of the triangle */

	/*  compute plane equation of triangle: normal*x+d=0 */

	CROSS(normal, e0, e1);

	// -NJMP- (line removed here)

	if (!planeBoxOverlap(normal, v0, boxhalfsize)) return 0;	// -NJMP-



	return 1;   /* box and triangle overlaps */

}

#endif

#endif

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
