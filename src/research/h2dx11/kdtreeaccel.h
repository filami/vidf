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

#if defined(_MSC_VER)
#define NOMINMAX
#pragma once
#endif

#ifndef PBRT_ACCELERATORS_KDTREEACCEL_H
#define PBRT_ACCELERATORS_KDTREEACCEL_H

// accelerators/kdtreeaccel.h*
#include "vidf/common/ray.h"
#include "vidf/common/plane.h"

namespace pbrt {


using namespace vidf;


struct Primitive
{
	Vector3f vertices[3];
};



// KdTreeAccel Declarations
// KdTreeAccel Local Declarations
struct KdAccelNode {
	// KdAccelNode Methods
	void InitLeaf(int *primNums, int np, std::vector<int> *primitiveIndices);
	void InitInterior(int axis, int ac, float s) {
		split = s;
		flags = axis;
		aboveChild |= (ac << 2);
	}
	float SplitPos() const { return split; }
	int nPrimitives() const { return nPrims >> 2; }
	int SplitAxis() const { return flags & 3; }
	bool IsLeaf() const { return (flags & 3) == 3; }
	int AboveChild() const { return aboveChild >> 2; }
	union {
		float split;                 // Interior
		int onePrimitive;            // Leaf
		int primitiveIndicesOffset;  // Leaf
	};

private:
	union {
		int flags;       // Both
		int nPrims;      // Leaf
		int aboveChild;  // Interior
	};
};

struct SurfaceInteraction {
	Vector3f point;
	Vector3f normal;
	float dist;
	uint primId;
};


bool IntersectS(const Rayf &ray, const Primitive& prim, SurfaceInteraction* isect);


struct BoundEdge;
class KdTreeAccel {
  public:
    // KdTreeAccel Public Methods
    KdTreeAccel(const std::vector<Primitive>& p,
                int isectCost = 80, int traversalCost = 1,
                float emptyBonus = 0.5, int maxPrims = 1, int maxDepth = -1);
    Boxf WorldBound() const { return bounds; }
    ~KdTreeAccel();
    bool Intersect(const Rayf &ray, SurfaceInteraction *isect, std::vector<uint>* testIds = nullptr) const;
    // bool IntersectP(const Rayf &ray) const;

//  private:
    // KdTreeAccel Private Methods
    void buildTree(int nodeNum, const Boxf &bounds,
                   const std::vector<Boxf> &primBounds, int *primNums,
                   int nprims, int depth,
                   const std::unique_ptr<BoundEdge[]> edges[3], int *prims0,
                   int *prims1, int badRefines = 0);

    // KdTreeAccel Private Data
    const int isectCost, traversalCost, maxPrims;
    const float emptyBonus;
    std::vector<Primitive> primitives;
    std::vector<int> primitiveIndices;
	std::vector<KdAccelNode> nodes;
    int nAllocedNodes, nextFreeNode;
	Boxf bounds;
};

struct KdToDo {
    const KdAccelNode *node;
    float tMin, tMax;
};

/*
std::shared_ptr<KdTreeAccel> CreateKdTreeAccelerator(
    std::vector<std::shared_ptr<Primitive>> prims, const ParamSet &ps);
*/

}  // namespace pbrt

#endif  // PBRT_ACCELERATORS_KDTREEACCEL_H