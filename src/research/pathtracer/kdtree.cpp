
/*
	pbrt source code is Copyright(c) 1998-2016
						Matt Pharr, Greg Humphreys, and Wenzel Jakob.

	This file is part of pbrt.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are
	met:

	- Redistributions of source code must retain the above copyright
	  notice, this list of conditions and the following disclaimer.

	- Redistributions in binary form must reproduce the above copyright
	  notice, this list of conditions and the following disclaimer in the
	  documentation and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
	IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
	TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
	PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
	HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
	LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */


// accelerators/kdtreeaccel.cpp*
#include "pch.h"
#include "kdtree.h"
#include <algorithm>

namespace vipt
{



inline int Log2Int(uint32_t v) {
	unsigned long lz = 0;
	if (_BitScanReverse(&lz, v)) return lz;
	return 0;
}



Boxf PrimitiveBounds(const Primitive& prim)
{
	Boxf bounds;
	bounds.min = bounds.max = prim.vertices[0];
	bounds = Union(bounds, prim.vertices[1]);
	bounds = Union(bounds, prim.vertices[2]);
	return bounds;
}



#define MachineEpsilon (std::numeric_limits<float>::epsilon() * 0.5)



inline float gamma(int n)
{
	return (n * MachineEpsilon) / (1 - n * MachineEpsilon);
}



bool IntersectBoxP(const Rayf &ray, const Boxf& bounds, float* hitt0, float* hitt1)
{
	float t0 = 0, t1 = ray.maximum;
	for (int i = 0; i < 3; ++i) {
		// Update interval for _i_th bounding box slab
		float invRayDir = 1 / ray.direction[i];
		float tNear = (bounds.min[i] - ray.origin[i]) * invRayDir;
		float tFar = (bounds.max[i] - ray.origin[i]) * invRayDir;

		// Update parametric interval from slab intersection $t$ values
		if (tNear > tFar) std::swap(tNear, tFar);

		// Update _tFar_ to ensure robust ray--bounds intersection
		tFar *= 1 + 2 * gamma(3);
		t0 = tNear > t0 ? tNear : t0;
		t1 = tFar < t1 ? tFar : t1;
		if (t0 > t1) return false;
	}
	if (hitt0) *hitt0 = t0;
	if (hitt1) *hitt1 = t1;
	return true;
}



bool IntersectPrimS(const Rayf &ray, const Primitive& prim, SurfaceInteraction* isect)
{
	const float EPSILON = 0.0000001;
	Vector3f vertex0 = prim.vertices[0];
	Vector3f vertex1 = prim.vertices[1];
	Vector3f vertex2 = prim.vertices[2];
	Vector3f edge1, edge2, h, s, q;
	float a, f, u, v;
	edge1 = vertex1 - vertex0;
	edge2 = vertex2 - vertex0;
	Vector3f n = Cross(edge1, edge2);
	if (Dot(ray.direction, n) >= 0.0f)
		return false;
	// h = rayVector.crossProduct(edge2);
	h = Cross(ray.direction, edge2);
	// a = edge1.dotProduct(h);
	a = Dot(edge1, h);
	if (a > -EPSILON && a < EPSILON)
		return false;
	f = 1 / a;
	s = ray.origin - vertex0;
	// u = f * (s.dotProduct(h));
	u = f * Dot(s, h);
	if (u < 0.0 || u > 1.0)
		return false;
	q = Cross(s, edge1);
	v = f * Dot(ray.direction, q);
	if (v < 0.0 || u + v > 1.0)
		return false;
	// At this stage we can compute t to find out where the intersection point is on the line.
	float t = f * Dot(edge2, q);
	if (t > isect->dist)
		return false;
	if (t > EPSILON) // ray intersection
	{
	//	outIntersectionPoint = rayOrigin + rayVector * t;
		isect->point = ray.origin + ray.direction * t;
		isect->normal = Normalize(n);
		isect->dist = t;
		isect->baryCoord = Vector2f(u, v);
		return true;
	}
	else // This means that there is a line intersection but not a ray intersection.
		return false;
}


bool IntersectPrimP(const Rayf &ray, const Primitive& prim)
{
	const float EPSILON = 0.0000001;
	Vector3f vertex0 = prim.vertices[0];
	Vector3f vertex1 = prim.vertices[1];
	Vector3f vertex2 = prim.vertices[2];
	Vector3f edge1, edge2, h, s, q;
	float a, f, u, v;
	edge1 = vertex1 - vertex0;
	edge2 = vertex2 - vertex0;
	Vector3f n = Cross(edge1, edge2);
	if (Dot(ray.direction, n) >= 0.0f)
		return false;
	// h = rayVector.crossProduct(edge2);
	h = Cross(ray.direction, edge2);
	// a = edge1.dotProduct(h);
	a = Dot(edge1, h);
	if (a > -EPSILON && a < EPSILON)
		return false;
	f = 1 / a;
	s = ray.origin - vertex0;
	// u = f * (s.dotProduct(h));
	u = f * Dot(s, h);
	if (u < 0.0 || u > 1.0)
		return false;
	q = Cross(s, edge1);
	v = f * Dot(ray.direction, q);
	if (v < 0.0 || u + v > 1.0)
		return false;
	float t = f * Dot(edge2, q);
	if (t > EPSILON && t < ray.maximum) // ray intersection
		return true;
	return false;
}



float SurfaceArea(Boxf bounds)
{
	Vector3f a = bounds.max - bounds.min;
	return (a.x * a.y + a.x * a.z + a.y * a.z) * 2.0f;
}



int MaximumExtent(Boxf bounds)
{
	Vector3f a = bounds.max - bounds.min;
	if (a.x > a.y && a.x > a.z)
		return 0;
	if (a.y > a.z)
		return 1;
	return 2;
}



enum class EdgeType { Start, End };
struct BoundEdge {
	// BoundEdge Public Methods
	BoundEdge() {}
	BoundEdge(float t, int primNum, bool starting) : t(t), primNum(primNum) {
		type = starting ? EdgeType::Start : EdgeType::End;
	}
	float t;
	int primNum;
	EdgeType type;
};



KdTreeAccel::KdTreeAccel()
{
}


// KdTreeAccel Method Definitions
void KdTreeAccel::Build(
	const std::vector<Primitive>& p,
	int _isectCost, int _traversalCost, float _emptyBonus,
	int _maxPrims, int maxDepth)
{
	isectCost = _isectCost;
	traversalCost = _traversalCost;
	maxPrims = _maxPrims;
	emptyBonus = _emptyBonus;
	primitives = p;

	// Build kd-tree for accelerator
	// ProfilePhase _(Prof::AccelConstruction);
	nextFreeNode = nAllocedNodes = 0;
	if (maxDepth <= 0)
		maxDepth = std::round(8 + 1.3f * Log2Int(int64_t(primitives.size())));

	// Compute bounds for kd-tree construction
	std::vector<Boxf> primBounds;
	primBounds.reserve(primitives.size());
	bounds = PrimitiveBounds(primitives[0]);
	for (const Primitive &prim : primitives) {
		Boxf b = PrimitiveBounds(prim);
		bounds = Union(bounds, b);
		primBounds.push_back(b);
	}

	// Allocate working memory for kd-tree construction
	std::unique_ptr<BoundEdge[]> edges[3];
	for (int i = 0; i < 3; ++i)
		edges[i].reset(new BoundEdge[2 * primitives.size()]);
	std::unique_ptr<int[]> prims0(new int[primitives.size()]);
	std::unique_ptr<int[]> prims1(new int[(maxDepth + 1) * primitives.size()]);

	// Initialize _primNums_ for kd-tree construction
	std::unique_ptr<int[]> primNums(new int[primitives.size()]);
	for (size_t i = 0; i < primitives.size(); ++i) primNums[i] = i;

	// Start recursive construction of kd-tree
	buildTree(0, bounds, primBounds, primNums.get(), primitives.size(),
			  maxDepth, edges, prims0.get(), prims1.get());
}



void KdAccelNode::InitLeaf(int *primNums, int np,
						   std::vector<int> *primitiveIndices) {
	flags = 3;
	nPrims |= (np << 2);
	// Store primitive ids for leaf node
	if (np == 0)
		onePrimitive = 0;
	else if (np == 1)
		onePrimitive = primNums[0];
	else {
		primitiveIndicesOffset = primitiveIndices->size();
		for (int i = 0; i < np; ++i) primitiveIndices->push_back(primNums[i]);
	}
}



KdTreeAccel::~KdTreeAccel() { }



void KdTreeAccel::buildTree(
	int nodeNum, const Boxf &nodeBounds,
	const std::vector<Boxf> &allPrimBounds,
	int *primNums, int nPrimitives, int depth,
	const std::unique_ptr<BoundEdge[]> edges[3],
	int *prims0, int *prims1, int badRefines)
{
	nodes.resize(nodes.size() + 1);
	++nextFreeNode;

	// Initialize leaf node if termination criteria met
	if (nPrimitives <= maxPrims || depth == 0) {
		nodes[nodeNum].InitLeaf(primNums, nPrimitives, &primitiveIndices);
		return;
	}

	// Initialize interior node and continue recursion

	// Choose split axis position for interior node
	int bestAxis = -1, bestOffset = -1;
	float bestCost = std::numeric_limits<float>::max();
	float oldCost = isectCost * float(nPrimitives);
	float totalSA = SurfaceArea(nodeBounds);
	float invTotalSA = 1 / totalSA;
	Vector3f d = nodeBounds.max - nodeBounds.min;

	// Choose which axis to split along
	int axis = MaximumExtent(nodeBounds);
	int retries = 0;
retrySplit:

	// Initialize edges for _axis_
	for (int i = 0; i < nPrimitives; ++i) {
		int pn = primNums[i];
		const Boxf &bounds = allPrimBounds[pn];
		edges[axis][2 * i] = BoundEdge(bounds.min[axis], pn, true);
		edges[axis][2 * i + 1] = BoundEdge(bounds.max[axis], pn, false);
	}

	// Sort _edges_ for _axis_
	std::sort(&edges[axis][0], &edges[axis][2 * nPrimitives],
			  [](const BoundEdge &e0, const BoundEdge &e1) -> bool {
				  if (e0.t == e1.t)
					  return (int)e0.type < (int)e1.type;
				  else
					  return e0.t < e1.t;
			  });

	// Compute cost of all splits for _axis_ to find best
	int nBelow = 0, nAbove = nPrimitives;
	for (int i = 0; i < 2 * nPrimitives; ++i) {
		if (edges[axis][i].type == EdgeType::End) --nAbove;
		float edgeT = edges[axis][i].t;
		if (edgeT > nodeBounds.min[axis] && edgeT < nodeBounds.max[axis]) {
			// Compute cost for split at _i_th edge

			// Compute child surface areas for split at _edgeT_
			int otherAxis0 = (axis + 1) % 3, otherAxis1 = (axis + 2) % 3;
			float belowSA = 2 * (d[otherAxis0] * d[otherAxis1] +
								 (edgeT - nodeBounds.min[axis]) *
									 (d[otherAxis0] + d[otherAxis1]));
			float aboveSA = 2 * (d[otherAxis0] * d[otherAxis1] +
								 (nodeBounds.max[axis] - edgeT) *
									 (d[otherAxis0] + d[otherAxis1]));
			float pBelow = belowSA * invTotalSA;
			float pAbove = aboveSA * invTotalSA;
			float eb = (nAbove == 0 || nBelow == 0) ? emptyBonus : 0;
			float cost =
				traversalCost +
				isectCost * (1 - eb) * (pBelow * nBelow + pAbove * nAbove);

			// Update best split if this is lowest cost so far
			if (cost < bestCost) {
				bestCost = cost;
				bestAxis = axis;
				bestOffset = i;
			}
		}
		if (edges[axis][i].type == EdgeType::Start) ++nBelow;
	}
	// CHECK(nBelow == nPrimitives && nAbove == 0);

	// Create leaf if no good splits were found
	if (bestAxis == -1 && retries < 2) {
		++retries;
		axis = (axis + 1) % 3;
		goto retrySplit;
	}
	if (bestCost > oldCost) ++badRefines;
	if ((bestCost > 4 * oldCost && nPrimitives < 16) || bestAxis == -1 ||
		badRefines == 3) {
		nodes[nodeNum].InitLeaf(primNums, nPrimitives, &primitiveIndices);
		return;
	}

	// Classify primitives with respect to split
	int n0 = 0, n1 = 0;
	for (int i = 0; i < bestOffset; ++i)
		if (edges[bestAxis][i].type == EdgeType::Start)
			prims0[n0++] = edges[bestAxis][i].primNum;
	for (int i = bestOffset + 1; i < 2 * nPrimitives; ++i)
		if (edges[bestAxis][i].type == EdgeType::End)
			prims1[n1++] = edges[bestAxis][i].primNum;

	// Recursively initialize children nodes
	float tSplit = edges[bestAxis][bestOffset].t;
	Boxf bounds0 = nodeBounds, bounds1 = nodeBounds;
	bounds0.max[bestAxis] = bounds1.min[bestAxis] = tSplit;
	buildTree(nodeNum + 1, bounds0, allPrimBounds, prims0, n0, depth - 1, edges,
			  prims0, prims1 + nPrimitives, badRefines);
	int aboveChild = nextFreeNode;
	nodes[nodeNum].InitInterior(bestAxis, aboveChild, tSplit);
	buildTree(aboveChild, bounds1, allPrimBounds, prims1, n1, depth - 1, edges,
			  prims0, prims1 + nPrimitives, badRefines);
}



bool KdTreeAccel::Intersect(const Rayf &ray, SurfaceInteraction *isect, std::vector<uint>* testIds) const
{
	// ProfilePhase p(Prof::AccelIntersect);
	// Compute initial parametric range of ray inside kd-tree extent
	float tMin, tMax;
	if (!IntersectBoxP(ray, bounds, &tMin, &tMax)) {
		return false;
	}

	isect->dist = std::numeric_limits<float>::max();

	// Prepare to traverse kd-tree for ray
	Vector3f invDir(1 / ray.direction.x, 1 / ray.direction.y, 1 / ray.direction.z);
	const int maxTodo = 64;
	KdToDo todo[maxTodo];
	int todoPos = 0;

	// Traverse kd-tree nodes in order for ray
	bool hit = false;
	const KdAccelNode *node = &nodes[0];
	while (node != nullptr) {
		// Bail out if we found a hit closer than the current node
		if (tMax < tMin) break;
		if (!node->IsLeaf()) {
			// Process kd-tree interior node

			// Compute parametric distance along ray to split plane
			int axis = node->SplitAxis();
			float tPlane = (node->SplitPos() - ray.origin[axis]) * invDir[axis];

			// Get node children pointers for ray
			const KdAccelNode *firstChild, *secondChild;
			int belowFirst =
				(ray.origin[axis] < node->SplitPos()) ||
				(ray.origin[axis] == node->SplitPos() && ray.direction[axis] <= 0);
			if (belowFirst) {
				firstChild = node + 1;
				secondChild = &nodes[node->AboveChild()];
			} else {
				firstChild = &nodes[node->AboveChild()];
				secondChild = node + 1;
			}

			// Advance to next child node, possibly enqueue other child
			if (tPlane > tMax || tPlane <= 0)
				node = firstChild;
			else if (tPlane < tMin)
				node = secondChild;
			else {
				// Enqueue _secondChild_ in todo list
				todo[todoPos].node = secondChild;
				todo[todoPos].tMin = tPlane;
				todo[todoPos].tMax = tMax;
				++todoPos;
				node = firstChild;
				tMax = tPlane;
			}
		} else {
			// Check for intersections inside leaf node
			int nPrimitives = node->nPrimitives();
			if (nPrimitives == 1) {
				const Primitive &p =
					primitives[node->onePrimitive];
				// Check one primitive inside leaf node
				if (testIds)
					testIds->push_back(node->onePrimitive);
				if (IntersectPrimS(ray, p, isect))
				{
					isect->primId = node->onePrimitive;
					hit = true;
				}
			} else {
				for (int i = 0; i < nPrimitives; ++i) {
					int index =
						primitiveIndices[node->primitiveIndicesOffset + i];
					const Primitive &p = primitives[index];
					if (testIds)
						testIds->push_back(index);
					// Check one primitive inside leaf node
					if (IntersectPrimS(ray, p, isect))
					{
						isect->primId = index;
						hit = true;
					}
				}
			}

			// Grab next node to process from todo list
			if (todoPos > 0) {
				--todoPos;
				node = todo[todoPos].node;
				tMin = todo[todoPos].tMin;
				tMax = todo[todoPos].tMax;
			} else
				break;
		}
	}
	return hit;
}



bool KdTreeAccel::IntersectP(const Rayf& ray) const
{
	// ProfilePhase p(Prof::AccelIntersectP);
	// Compute initial parametric range of ray inside kd-tree extent
	float tMin, tMax;
	if (!IntersectBoxP(ray, bounds, &tMin, &tMax)) {
		return false;
	}

	// Prepare to traverse kd-tree for ray
	Vector3f invDir(1 / ray.direction.x, 1 / ray.direction.y, 1 / ray.direction.z);
	const int maxTodo = 64;
	KdToDo todo[maxTodo];
	int todoPos = 0;
	const KdAccelNode *node = &nodes[0];
	while (node != nullptr) {
		if (node->IsLeaf()) {
			// Check for shadow ray intersections inside leaf node
			int nPrimitives = node->nPrimitives();
			if (nPrimitives == 1) {
				const Primitive p =
					primitives[node->onePrimitive];
				if (IntersectPrimP(ray, p)) return true;
			} else {
				for (int i = 0; i < nPrimitives; ++i) {
					int primitiveIndex =
						primitiveIndices[node->primitiveIndicesOffset + i];
					const Primitive prim =
						primitives[primitiveIndex];
					if (IntersectPrimP(ray, prim))
						return true;
				}
			}

			// Grab next node to process from todo list
			if (todoPos > 0) {
				--todoPos;
				node = todo[todoPos].node;
				tMin = todo[todoPos].tMin;
				tMax = todo[todoPos].tMax;
			} else
				break;
		} else {
			// Process kd-tree interior node

			// Compute parametric distance along ray to split plane
			int axis = node->SplitAxis();
			float tPlane = (node->SplitPos() - ray.origin[axis]) * invDir[axis];

			// Get node children pointers for ray
			const KdAccelNode *firstChild, *secondChild;
			int belowFirst =
				(ray.origin[axis] < node->SplitPos()) ||
				(ray.origin[axis] == node->SplitPos() && ray.direction[axis] <= 0);
			if (belowFirst) {
				firstChild = node + 1;
				secondChild = &nodes[node->AboveChild()];
			} else {
				firstChild = &nodes[node->AboveChild()];
				secondChild = node + 1;
			}

			// Advance to next child node, possibly enqueue other child
			if (tPlane > tMax || tPlane <= 0)
				node = firstChild;
			else if (tPlane < tMin)
				node = secondChild;
			else {
				// Enqueue _secondChild_ in todo list
				todo[todoPos].node = secondChild;
				todo[todoPos].tMin = tPlane;
				todo[todoPos].tMax = tMax;
				++todoPos;
				node = firstChild;
				tMax = tPlane;
			}
		}
	}
	return false;
}

void KdTreeAccel::QueryPrimitives(Vector3f point, std::vector<uint>* testIds) const
{
	if (point.x < bounds.min.x || point.y < bounds.min.y || point.z < bounds.min.z)
		return;
	if (point.x > bounds.max.x || point.y > bounds.max.y || point.z > bounds.max.z)
		return;

	uint nodeIdx = 0;
	while (!nodes[nodeIdx].IsLeaf())
	{
		const int axis = nodes[nodeIdx].SplitAxis();
		const float plane = nodes[nodeIdx].SplitPos();
		if (point[axis] < plane)
			nodeIdx++;
		else
			nodeIdx = nodes[nodeIdx].AboveChild();
	}

	int nPrimitives = nodes[nodeIdx].nPrimitives();
	if (nPrimitives == 1)
	{
		testIds->push_back(nodes[nodeIdx].onePrimitive);
	}
	else
	{
		for (int i = 0; i < nPrimitives; ++i)
		{
			int primitiveIndex = primitiveIndices[nodes[nodeIdx].primitiveIndicesOffset + i];
			testIds->push_back(primitiveIndex);
		}
	}
}

}  // namespace pbrt