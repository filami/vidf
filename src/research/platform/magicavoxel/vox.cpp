#include "pch.h"
#include "vox.h"

namespace vox
{


	struct VoxHeader
	{
		uint32 magic;
		uint32 version;
	};


	struct VoxChunk
	{
		union
		{
			uint32 id;
			char type[4];
		};
		uint32 contentSize;
		uint32 childrenSize;
	};



	void ReadVox(istream& is, int32& output)
	{
		is.read((char*)&output, sizeof(output));
	}



	void ReadVox(istream& is, string& output)
	{
		int32 n;
		ReadVox(is, n);
		vector<uint8> data;
		data.resize(n);
		is.read((char*)data.data(), n);
		output = string(data.begin(), data.end());
	}


	void ReadVox(istream& is, VoxDict& output)
	{
		int32 n;
		ReadVox(is, n);
		for (uint i = 0; i < n; ++i)
		{
			string key;
			string value;
			ReadVox(is, key);
			ReadVox(is, value);
			output[key] = value;
		}
	}


	void ReadVox(istream& is, VoxNodeTransform& output)
	{
		output.type = VoxNodeType::Transform;
		ReadVox(is, output.nodeId);
		ReadVox(is, output.attributes);
		ReadVox(is, output.childNodeId);
		ReadVox(is, output.reserved);
		ReadVox(is, output.layerId);
		ReadVox(is, output.numFrames);
		output.frameAttibutes.resize(output.numFrames);
		for (uint i = 0; i < output.numFrames; ++i)
			ReadVox(is, output.frameAttibutes[i]);
	}



	void ReadVox(istream& is, VoxNodeGroup& output)
	{
		output.type = VoxNodeType::Group;
		ReadVox(is, output.nodeId);
		ReadVox(is, output.attributes);
		int32 numChildren;
		ReadVox(is, numChildren);
		output.childrenIds.resize(numChildren);
		for (uint i = 0; i < numChildren; ++i)
			ReadVox(is, output.childrenIds[i]);
	}



	void ReadVox(istream& is, VoxNodeShape& output)
	{
		output.type = VoxNodeType::Shape;
		ReadVox(is, output.nodeId);
		ReadVox(is, output.attributes);
		int32 numModels;
		ReadVox(is, numModels);
		output.models.resize(numModels);
		for (uint i = 0; i < numModels; ++i)
		{
			ReadVox(is, output.models[i].modelId);
			ReadVox(is, output.models[i].attributes);
		}
	}
	


	bool ReadFox(istream& is, VoxData& voxData)
	{
		VoxHeader header;
		is.read((char*)&header, sizeof(header));
		if (header.magic != 0x20584f56 || header.version != 150)
			return false;

		VoxChunk chunk;
		is.read((char*)&chunk, sizeof(chunk));
		if (chunk.id == 0x4e49414d) // MAIN
		{
			uint end = uint(is.tellg()) + chunk.contentSize + chunk.childrenSize;

			uint32 numVoxels;

			while (uint(is.tellg()) < end)
			{
				is.read((char*)&chunk, sizeof(chunk));
				switch (chunk.id)
				{
				default:
					is.seekg(chunk.contentSize + chunk.childrenSize, is.cur);
					break;
				case 0x455a4953: // SIZE
					voxData.models.emplace_back();
					is.read((char*)&voxData.models.back().size, sizeof(VoxVec3i));
					break;
				case 0x495a5958: // XYZI
					is.read((char*)&numVoxels, sizeof(numVoxels));
					voxData.models.back().voxels.resize(numVoxels);
					is.read((char*)voxData.models.back().voxels.data(), sizeof(VoxVoxel) * numVoxels);
					break;
				case 0x41424752: // RGBA
					is.read((char*)voxData.palette.data(), sizeof(voxData.palette));
					break;
				case 0x4e52546e: // nTRN
				{
					VoxNodeTransform nTRN;
					ReadVox(is, nTRN);
					voxData.nodes.emplace_back(make_shared<VoxNodeTransform>(nTRN));
				}
				break;
				case 0x5052476e: // nGRP
				{
					VoxNodeGroup nGRP;
					ReadVox(is, nGRP);
					voxData.nodes.emplace_back(make_shared<VoxNodeGroup>(nGRP));
				}
				break;
				case 0x5048536e: // nSHP
				{
					VoxNodeShape nSHP;
					ReadVox(is, nSHP);
					voxData.nodes.emplace_back(make_shared<VoxNodeShape>(nSHP));
				}
				break;
				}
			}
		}
		return true;
	}


}
