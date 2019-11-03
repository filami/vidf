#pragma once

namespace h2
{

	using namespace vidf;



	class BIH
	{
	public:
		struct Triangle
		{
			Vector3f vertices[3];
		};

	public:
		void Build(const std::vector<Triangle>& _triangles);

	private:
		struct Node
		{
			void  InitLeaf(int *primNums, int np, std::vector<int>* primitiveIndices);
			void  InitInterior(int axis, int ac, float s);
			float SplitPos() const { return split; }
			int   GetNumPrimitives() const { return nPrims >> 2; }
			int   SplitAxis() const { return flags & 3; }
			bool  IsLeaf() const { return (flags & 3) == 3; }
			int   AboveChild() const { return aboveChild >> 2; }

			union
			{
				float split;                   // Interior
				int   primitiveIndicesOffset;  // Leaf
			};
			union
			{
				int flags;       // Both
				int nPrims;      // Leaf
				int aboveChild;  // Interior
			};
		};

		struct Leaf
		{
			uint firstTriangleIndex;
			uint numTriangles;
		};

		enum class EdgeType
		{
			Start,
			End
		};

		struct BoundEdge
		{
			BoundEdge();
			BoundEdge(float t, int primNum, bool starting);
			float    t;
			int      primNum;
			EdgeType type;
		};

	private:
		Boxf TriangleToBounds(Triangle triangle);
		void BuildNode();

	private:
		std::vector<Triangle> triangles;
		std::vector<Node>     nodes;
		std::vector<Leaf>     leafs;
		std::vector<uint>     indices;
	};


}
