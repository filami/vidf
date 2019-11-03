#include "pch.h"
#include "bih.h"

namespace h2
{


	void BIH::Build(const std::vector<BIH::Triangle>& _triangles)
	{
		triangles = _triangles;
		nodes.clear();
		leafs.clear();
		indices.clear();
	}



	Boxf BIH::TriangleToBounds(BIH::Triangle triangle)
	{
		Boxf bounds;
		bounds.min = bounds.max = triangle.vertices[0];
		bounds = Union(bounds, triangle.vertices[1]);
		bounds = Union(bounds, triangle.vertices[2]);
		return bounds;
	}



	void BIH::BuildNode()
	{
	}


}
