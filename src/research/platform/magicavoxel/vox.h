#pragma once

namespace vox
{

	using namespace std;
	using namespace vidf;


	struct VoxVec3i
	{
		uint32 x, y, z;
	};


	struct VoxVoxel
	{
		uint8 x, y, z;
		uint8 index;
	};



	struct VoxModel
	{
		VoxVec3i size;
		vector<VoxVoxel> voxels;
	};


	struct VoxColor
	{
		uint8 r, g, b, a;
	};


	typedef unordered_map<string, string> VoxDict;


	enum class VoxNodeType
	{
		Transform,
		Group,
		Shape,
	};


	struct VoxNode
	{
		int32 nodeId;
		VoxNodeType type;
		VoxDict attributes;
		vector<uint> childrenIds;
	};


	struct VoxNodeTransform : public VoxNode
	{
		int32 childNodeId;
		int32 reserved;
		int32 layerId;
		int32 numFrames;
		vector<VoxDict> frameAttibutes;
	};


	struct VoxNodeGroup : public VoxNode
	{
		vector<int32> childrenIds;
	};


	struct VoxNodeShape : public VoxNode
	{
		struct VoxModelRef
		{
			int32 modelId;
			VoxDict attributes;
		};
		vector<VoxModelRef> models;
	};



	inline VoxNodeTransform* VoxToTransform(VoxNode* node)
	{
		if (!node || node->type != VoxNodeType::Transform)
			return nullptr;
		return static_cast<VoxNodeTransform*>(node);
	}

	inline VoxNodeGroup* VoxToGroup(VoxNode* node)
	{
		if (!node || node->type != VoxNodeType::Group)
			return nullptr;
		return static_cast<VoxNodeGroup*>(node);
	}

	inline VoxNodeShape* VoxToShape(VoxNode* node)
	{
		if (!node || node->type != VoxNodeType::Shape)
			return nullptr;
		return static_cast<VoxNodeShape*>(node);
	}



	struct VoxData
	{
		deque<VoxModel> models;
		deque<shared_ptr<VoxNode>> nodes;
		array<VoxColor, 256> palette;
	};



	bool ReadFox(istream& is, VoxData& voxData);


	template<typename Context, typename Fn>
	void VoxTransverse(const VoxData& voxData, VoxNode* node, Context context, Fn fn)
	{
		fn(voxData, node, context);
		if (VoxNodeTransform* transform = VoxToTransform(node))
			VoxTransverse(voxData, voxData.nodes[transform->childNodeId].get(), context, fn);
		else if (VoxNodeGroup* group = VoxToGroup(node))
		{
			for (uint i = 0; i < group->childrenIds.size(); ++i)
				VoxTransverse(voxData, voxData.nodes[group->childrenIds[i]].get(), context, fn);
		}
	}



}
