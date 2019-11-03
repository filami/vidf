#pragma once

#include "vidf/common/matrix33.h"
#include "vidf/rendererdx11/resources.h"

namespace std
{


	constexpr inline std::size_t hash_combine(std::size_t hash1, std::size_t hash2)
	{
		return hash1 ^ (hash2 * 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
	}


	template<typename T> struct hash<vidf::Vector3<T>>
	{
		typedef vidf::Vector3<T> argument_type;
		typedef std::size_t      result_type;
		result_type operator()(argument_type const& s) const noexcept
		{
			result_type const h1(std::hash<T>{}(s.x));
			result_type const h2(std::hash<T>{}(s.y));
			result_type const h3(std::hash<T>{}(s.y));
			return hash_combine(h1, hash_combine(h2, h3));
		}
	};


}

namespace vi::retro::render
{

	using namespace std;
	using namespace vidf;
	using namespace dx11;


	class VoxelModel
	{
	public:
		struct Vertex
		{
			Vector3f position;
			Vector3f normal;
			uint32   color;
		};

	private:
		typedef uint16 Index;

	public:
		void InsertVoxel(Vector3i point, Color color);
		void AppendModel(const VoxelModel& model, Vector3i offset, const Matrix33<int>& rot = Matrix33<int>{zero});
		void Triangulate();
		void CreateResources(RenderDevicePtr renderDevice);

		const unordered_map<Vector3i, uint32>& GetVoxels() const { return voxels; }
		const vector<Vertex>   GetVertices() const { return vertices; }
		const vector<Index>    GetIndices() const { return indices; }
		const vector<Vector3f> GetOutlineVertices() const { return outlineVertices; }
		const vector<Index>    GetOutlineIndices() const { return outlineIndices; }

		static vector<D3D11_INPUT_ELEMENT_DESC> GetGeometryDesc();
		GPUBuffer GetVertexBuffer() const { return vertexBuffer; }
		GPUBuffer GetIndexBuffer() const { return indexBuffer; }
		GPUBuffer GetFlippedIndexBuffer() const { return flippedIndexBuffer; }

		static vector<D3D11_INPUT_ELEMENT_DESC> GetOutlineGeometryDesc();
		GPUBuffer GetOutlineVertexBuffer() const { return outlineVertexBuffer; }
		GPUBuffer GetOutlineIndexBuffer() const { return outlineIndexBuffer; }

	private:
		unordered_map<Vector3i, uint32> voxels;
		vector<Vertex> vertices;
		vector<Index>  indices;
		vector<Index>  flippedIndices;
		vector<Vector3f> outlineVertices;
		vector<Index>    outlineIndices;
		bool triangulated = false;

		static vector<D3D11_INPUT_ELEMENT_DESC> geometryDesc;
		static vector<D3D11_INPUT_ELEMENT_DESC> outlineGeometryDesc;
		GPUBuffer vertexBuffer;
		GPUBuffer indexBuffer;
		GPUBuffer flippedIndexBuffer;
		GPUBuffer outlineVertexBuffer;
		GPUBuffer outlineIndexBuffer;
		bool gpuResources = false;
	};

	typedef shared_ptr<VoxelModel> VoxelModelPtr;



	class VoxelMultiModel
	{
	public:
		void AddModel(VoxelModelPtr model);

	private:
		deque<VoxelModelPtr> models;
	};


}
