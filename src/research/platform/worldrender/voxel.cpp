#include "pch.h"
#include "voxel.h"

namespace vi::retro::render
{


	vector<D3D11_INPUT_ELEMENT_DESC> VoxelModel::geometryDesc;
	vector<D3D11_INPUT_ELEMENT_DESC> VoxelModel::outlineGeometryDesc;



	void VoxelModel::InsertVoxel(Vector3i point, Color color)
	{
		voxels[point] = color.AsRGBA8();
	}



	void VoxelModel::AppendModel(const VoxelModel& model, Vector3i offset, const Matrix33<int>& rot)
	{
		for (auto& voxel : model.voxels)
			voxels[Mul(rot, voxel.first) + offset] = voxel.second;
	}



	void VoxelModel::Triangulate()
	{
		if (triangulated)
			return;

		struct VertexEq
		{
			bool operator()(const Vertex& a, const Vertex& b) const noexcept
			{
				if (a.position != b.position)
					return false;
				if (a.normal != b.normal)
					return false;
				if (a.color != b.color)
					return false;
				return true;
			}
		};
		struct VertexHash
		{
			size_t operator()(const Vertex& s) const noexcept
			{
				size_t const h1(hash<Vector3f>{}(s.position));
				size_t const h2(hash<Vector3f>{}(s.normal));
				size_t const h3(hash<uint32>{}(s.color));
				return hash_combine(h1, hash_combine(h2, h3));
			}
		};

		unordered_map<Vertex, Index, VertexHash, VertexEq> vertexToIndex;
		unordered_map<Vector3f, Index> outlineVertexToIndex;

		auto InsertVertex = [&](Vertex vertex)
		{
			Index index = 0;
			auto it = vertexToIndex.find(vertex);
			if (it == vertexToIndex.end())
			{
				index = Index(vertices.size());
				vertexToIndex[vertex] = index;
				vertices.push_back(vertex);
			}
			else
				index = it->second;
			indices.push_back(index);
		};

		auto InsertOutlineVertex = [&](Vector3f vertex)
		{
			Index index = 0;
			auto it = outlineVertexToIndex.find(vertex);
			if (it == outlineVertexToIndex.end())
			{
				index = Index(outlineVertices.size());
				outlineVertexToIndex[vertex] = index;
				outlineVertices.push_back(vertex);
			}
			else
				index = it->second;
			outlineIndices.push_back(index);
		};

		auto HasVoxel = [&](Vector3i point)
		{
			return voxels.find(point) != voxels.end();
		};

		auto HasOutline = [&](Vector3i point, int i0, int d0, int i1, int d1)
		{
			Vector3i aDir{ zero };
			Vector3i bDir{ zero };
			Vector3i cDir{ zero };
			aDir[i0] = d0;
			aDir[i1] = d1;
			bDir[i0] = d0;
			cDir[i1] = d1;
			const bool a = voxels.find(point + aDir) != voxels.end();
			const bool b = voxels.find(point + bDir) != voxels.end();
			const bool c = voxels.find(point + cDir) != voxels.end();
			if (a && b && c)
				return false;
			if (!a && !b && !c)
				return true;
			if (a && b)
				return true;
			if (a && c)
				return true;
			if (b && c && !a)
				return true;
			if (a && !b && !c)
				return true;
			return false;
		};

		for (const auto& voxel : voxels)
		{
		//	if (vertices.size() >= ((1 << 16) - 8))
		//		break;

			Vertex vertex;
			Vector3i point = voxel.first;
			vertex.color = voxel.second;

			const Vector3f positions[]
			{
				{ point.x + 0.0f, point.y + 0.0f, point.z + 0.0f }, // 0
				{ point.x + 1.0f, point.y + 0.0f, point.z + 0.0f }, // 1
				{ point.x + 0.0f, point.y + 1.0f, point.z + 0.0f }, // 2
				{ point.x + 1.0f, point.y + 1.0f, point.z + 0.0f }, // 3
				{ point.x + 0.0f, point.y + 0.0f, point.z + 1.0f }, // 4
				{ point.x + 1.0f, point.y + 0.0f, point.z + 1.0f }, // 5
				{ point.x + 0.0f, point.y + 1.0f, point.z + 1.0f }, // 6
				{ point.x + 1.0f, point.y + 1.0f, point.z + 1.0f }, // 7
			};
			
			// Voxel
			// -X
			if (!HasVoxel(point + Vector3i(-1, 0, 0)))
			{
				vertex.normal = Vector3f(-1.0f, 0.0f, 0.0f);
				vertex.position = positions[0]; InsertVertex(vertex);
				vertex.position = positions[4]; InsertVertex(vertex);
				vertex.position = positions[6]; InsertVertex(vertex);
				vertex.position = positions[0]; InsertVertex(vertex);
				vertex.position = positions[6]; InsertVertex(vertex);
				vertex.position = positions[2]; InsertVertex(vertex);
			}
			// +X
			if (!HasVoxel(point + Vector3i(1, 0, 0)))
			{
				vertex.normal = Vector3f(1.0f, 0.0f, 0.0f);
				vertex.position = positions[1]; InsertVertex(vertex);
				vertex.position = positions[3]; InsertVertex(vertex);
				vertex.position = positions[7]; InsertVertex(vertex);

				vertex.position = positions[1]; InsertVertex(vertex);
				vertex.position = positions[7]; InsertVertex(vertex);
				vertex.position = positions[5]; InsertVertex(vertex);
			}			
			// -Y
			if (!HasVoxel(point + Vector3i(0, -1, 0)))
			{
				vertex.normal = Vector3f(0.0f, -1.0f, 0.0f);
				vertex.position = positions[0]; InsertVertex(vertex);
				vertex.position = positions[1]; InsertVertex(vertex);
				vertex.position = positions[5]; InsertVertex(vertex);
				vertex.position = positions[0]; InsertVertex(vertex);
				vertex.position = positions[5]; InsertVertex(vertex);
				vertex.position = positions[4]; InsertVertex(vertex);
			}			
			// +Y
			if (!HasVoxel(point + Vector3i(0, 1, 0)))
			{
				vertex.normal = Vector3f(0.0f, 1.0f, 0.0f);
				vertex.position = positions[2]; InsertVertex(vertex);
				vertex.position = positions[6]; InsertVertex(vertex);
				vertex.position = positions[7]; InsertVertex(vertex);
				vertex.position = positions[2]; InsertVertex(vertex);
				vertex.position = positions[7]; InsertVertex(vertex);
				vertex.position = positions[3]; InsertVertex(vertex);
			}			
			// -Z
			if (!HasVoxel(point + Vector3i(0, 0, -1)))
			{
				vertex.normal = Vector3f(0.0f, 0.0f, -1.0f);
				vertex.position = positions[0]; InsertVertex(vertex);
				vertex.position = positions[2]; InsertVertex(vertex);
				vertex.position = positions[3]; InsertVertex(vertex);

				vertex.position = positions[0]; InsertVertex(vertex);
				vertex.position = positions[3]; InsertVertex(vertex);
				vertex.position = positions[1]; InsertVertex(vertex);
			}
			// +Z
			if (!HasVoxel(point + Vector3i(0, 0, 1)))
			{
				vertex.normal = Vector3f(0.0f, 0.0f, 1.0f);
				vertex.position = positions[4]; InsertVertex(vertex);
				vertex.position = positions[5]; InsertVertex(vertex);
				vertex.position = positions[7]; InsertVertex(vertex);
				vertex.position = positions[4]; InsertVertex(vertex);
				vertex.position = positions[7]; InsertVertex(vertex);
				vertex.position = positions[6]; InsertVertex(vertex);
			}
			/*
			// Outlines
			// -X -Y
			if (HasOutline(point, 0, -1, 1, -1))
			{
				InsertOutlineVertex(positions[0]);
				InsertOutlineVertex(positions[4]);
			}
			// +X -Y
			if (HasOutline(point, 0, 1, 1, -1))
			{
				InsertOutlineVertex(positions[5]);
				InsertOutlineVertex(positions[1]);
			}
			// -X +Y
			if (HasOutline(point, 0, -1, 1, 1))
			{
				InsertOutlineVertex(positions[6]);
				InsertOutlineVertex(positions[2]);
			}
			// +X +Y
			if (HasOutline(point, 0, 1, 1, 1))
			{
				InsertOutlineVertex(positions[7]);
				InsertOutlineVertex(positions[3]);
			}
			// -X -Z
			if (HasOutline(point, 0, -1, 2, -1))
			{
				InsertOutlineVertex(positions[0]);
				InsertOutlineVertex(positions[2]);
			}
			// +X -Z
			if (HasOutline(point, 0, 1, 2, -1))
			{
				InsertOutlineVertex(positions[1]);
				InsertOutlineVertex(positions[3]);
			}
			// -X +Z
			if (HasOutline(point, 0, -1, 2, 1))
			{
				InsertOutlineVertex(positions[6]);
				InsertOutlineVertex(positions[4]);
			}
			// +X +Z
			if (HasOutline(point, 0, 1, 2, 1))
			{
				InsertOutlineVertex(positions[7]);
				InsertOutlineVertex(positions[5]);
			}
			// -Y -Z
			if (HasOutline(point, 1, -1, 2, -1))
			{
				InsertOutlineVertex(positions[0]);
				InsertOutlineVertex(positions[1]);
			}
			// +Y -Z
			if (HasOutline(point, 1, 1, 2, -1))
			{
				InsertOutlineVertex(positions[2]);
				InsertOutlineVertex(positions[3]);
			}
			// -Y +Z
			if (HasOutline(point, 1, -1, 2, 1))
			{
				InsertOutlineVertex(positions[4]);
				InsertOutlineVertex(positions[5]);
			}
			// +Y +Z
			if (HasOutline(point, 1, 1, 2, 1))
			{
				InsertOutlineVertex(positions[6]);
				InsertOutlineVertex(positions[7]);
			}
			*/
		}

		flippedIndices.resize(indices.size());
		for (uint i = 0; i < indices.size(); i += 3)
		{
			flippedIndices[i + 0] = indices[i + 0];
			flippedIndices[i + 1] = indices[i + 2];
			flippedIndices[i + 2] = indices[i + 1];
		}

		triangulated = true;
	}


	vector<D3D11_INPUT_ELEMENT_DESC> VoxelModel::GetGeometryDesc()
	{
		static bool init = false;
		if (!init)
		{
			D3D11_INPUT_ELEMENT_DESC desc{};
			desc.SemanticName = "POSITION";
			desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			desc.AlignedByteOffset = offsetof(Vertex, position);
			geometryDesc.push_back(desc);

			desc.SemanticName = "NORMAL";
			desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			desc.AlignedByteOffset = offsetof(Vertex, normal);
			geometryDesc.push_back(desc);

			desc.SemanticName = "COLOR";
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.AlignedByteOffset = offsetof(Vertex, color);
			geometryDesc.push_back(desc);

			init = true;
		}
		return geometryDesc;
	}


	vector<D3D11_INPUT_ELEMENT_DESC> VoxelModel::GetOutlineGeometryDesc()
	{
		static bool init = false;
		if (!init)
		{
			D3D11_INPUT_ELEMENT_DESC desc{};
			desc.SemanticName = "POSITION";
			desc.Format = DXGI_FORMAT_R32G32B32_FLOAT;
			outlineGeometryDesc.push_back(desc);

			init = true;
		}
		return outlineGeometryDesc;
	}



	void VoxelModel::CreateResources(RenderDevicePtr renderDevice)
	{
		if (gpuResources)
			return;

		GPUBufferDesc vbDesc;
		vbDesc.type = GPUBufferType::VertexBuffer;
		vbDesc.elementStride = sizeof(Vertex);
		vbDesc.elementCount = vertices.size();
		vbDesc.dataPtr = vertices.data();
		vbDesc.dataSize = vertices.size() * sizeof(Vertex);
		vbDesc.name = "voxelModelVB";
		vertexBuffer = GPUBuffer::Create(renderDevice, vbDesc);

		GPUBufferDesc ibDesc;
		ibDesc.type = GPUBufferType::IndexBuffer;
		ibDesc.format = DXGI_FORMAT_R16_UINT;
		ibDesc.elementStride = sizeof(Index);
		ibDesc.elementCount = indices.size();
		ibDesc.dataPtr = indices.data();
		ibDesc.dataSize = indices.size() * sizeof(Index);
		ibDesc.name = "voxelModelIB";
		indexBuffer = GPUBuffer::Create(renderDevice, ibDesc);

		ibDesc.dataPtr = flippedIndices.data();
		ibDesc.dataSize = flippedIndices.size() * sizeof(Index);
		flippedIndexBuffer = GPUBuffer::Create(renderDevice, ibDesc);

		/*
		GPUBufferDesc vbOutlineDesc;
		vbOutlineDesc.type = GPUBufferType::VertexBuffer;
		vbOutlineDesc.elementStride = sizeof(Vector3f);
		vbOutlineDesc.elementCount = outlineVertices.size();
		vbOutlineDesc.dataPtr = outlineVertices.data();
		vbOutlineDesc.dataSize = outlineVertices.size() * sizeof(Vertex);
		vbOutlineDesc.name = "voxelModelOutlineVB";
		outlineVertexBuffer = GPUBuffer::Create(renderDevice, vbOutlineDesc);

		GPUBufferDesc ibOutlineDesc;
		ibOutlineDesc.type = GPUBufferType::IndexBuffer;
		ibOutlineDesc.format = DXGI_FORMAT_R32_UINT;
		ibOutlineDesc.elementStride = sizeof(Index);
		ibOutlineDesc.elementCount = outlineIndices.size();
		ibOutlineDesc.dataPtr = outlineIndices.data();
		ibOutlineDesc.dataSize = outlineIndices.size() * sizeof(Index);
		ibOutlineDesc.name = "voxelModelOutlineIB";
		outlineIndexBuffer = GPUBuffer::Create(renderDevice, ibOutlineDesc);
		*/
		gpuResources = true;
	}



	void VoxelMultiModel::AddModel(VoxelModelPtr model)
	{
		models.push_back(model);
	}


}
