#include "pch.h"

#include <stdlib.h>
#include <sstream>
#include <iomanip>

#include <list>
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

#include "rendererdx12/common.h"
#include "rendererdx12/renderdevice.h"
#include "rendererdx12/rendercontext.h"
#include "rendererdx12/shadermanager.h"
#include "rendererdx12/resources.h"

#include "stream.h"
#include "pak.h"
#include "bsptexture.h"

using namespace vidf;
using namespace proto;



namespace vidf
{


	uint SDBMHashFn(const void* data, uint length)
	{
		uint hash = 0;
		uint i = 0;
		const char* ptr = (const char*)data;
		for (i = 0; i < length; ++ptr, ++i)
			hash = (*ptr) + (hash << 6) + (hash << 16) - hash;
		return hash;
	}


}



namespace std
{


	template<typename T> struct SDBMHash
	{
		typedef T argument_type;
		typedef size_t result_type;
		result_type operator()(argument_type const& s) const noexcept
		{
			return SDBMHashFn(&s, sizeof(argument_type));
		}
	};



	template<typename T> struct SDBMHashEqualTo
	{
		typedef T argument_type;
		constexpr bool operator()(const argument_type& left, const argument_type& right) const
		{
			return memcmp(&left, &right, sizeof(argument_type)) == 0;
		}
	};


}



template<typename K, typename V>
using SDBMHashMap = unordered_map<K, V, SDBMHash<K>, SDBMHashEqualTo<K>>;



namespace vidf::dx12
{



class Dx12CanvasListener : public CanvasListener
{
public:
	Dx12CanvasListener() {}

	virtual void Close()
	{
		PostQuitMessage();
	}
	virtual void KeyDown(KeyCode keyCode)
	{
		if (keyCode == KeyCode::Escape)
			PostQuitMessage();
	}
};



}



namespace h2
{

	using namespace dx12;


	float bspUnitsToMeters = 1.0f / 64.0f;


	enum BspLumpType
	{
		BspLump_Entities,
		BspLump_Planes,
		BspLump_Vertices,
		BspLump_Visibility,
		BspLump_Nodes,
		BspLump_TextureInformation,
		BspLump_Faces,
		BspLump_Lightmaps,
		BspLump_Leaves,
		BspLump_LeafFaceTable,
		BspLump_LeafBrushTable,
		BspLump_Edges,
		BspLump_FaceEdgeTable,
		BspLump_Models,
		BspLump_Brushes,
		BspLump_BrushSides,
		BspLump_Pop,
		BspLump_Areas,
		BspLump_AreaPortals,

		BspLump_Count,
	};


	enum BspSurface
	{
		BspSurf_LIGHT = 0x01,
		BspSurf_SLICK = 0x02,
		BspSurf_SKY = 0x04,
		BspSurf_WARP = 0x08,
		BspSurf_TRANS33 = 0x10,
		BspSurf_TRANS66 = 0x20,
		BspSurf_FLOWING = 0x40,
		BspSurf_NODRAW = 0x80,
	};


	struct BspHeader
	{
		uint32 signature;
		uint32 version;
	};

	struct BspLump
	{
		uint32 offset;
		uint   length;
	};

	struct BspEdge
	{
		int16 vId[2];
	};

	struct BspPlane
	{
		Vector3f normal;
		float    distance;
		uint32   type;
	};

	struct BspNode
	{
		uint32         plane;
		int32          frontChild;
		int32          backChild;
		Vector3<int16> bboxMin;
		Vector3<int16> bboxMax;
		uint16         firstFace;
		uint16         numFaces;
	};

	struct BspFace
	{
		uint16 plane;
		uint16 planeSide;
		uint32 firstEdge;
		uint16 numEdges;
		uint16 textureInfo;
		uint8  lightmapStyles[4];
		uint32 lightmapOffset;
	};

	struct BspTexInfo
	{
		Vector3f uAxis;
		float    uOffset;
		Vector3f vAxis;
		float    vOffset;
		uint32   flags;
		uint32   value;
		char     textureName[32];
		uint32   nextTexInfo;
	};

	typedef std::unordered_map<std::string, std::string> BspEntity;

	struct BspData
	{
		vector<Vector3f>   vertices;
		vector<BspEdge>    edges;
		vector<BspNode>    nodes;
		vector<int32>      faceEdges;
		vector<BspFace>    faces;
		vector<BspPlane>   planes;
		vector<BspTexInfo> texInfo;

		vector<uint32>       texInfoToTexture;
		vector<BspTexture>   textures;
		vector<GPUBufferPtr> gpuTextures;
		map<std::string, uint> textureMap;
		vector<BspEntity>    entities;
	};

	class Tokenizer
	{
	public:
		Tokenizer(const char* _begin, const char* _final)
			: begin(_begin), end(_begin), final(_final) {}

		void Next()
		{
			begin = end;
			while (!EndOfStream() && std::isspace(*begin))
				begin++;
			if (EndOfStream())
				return;
			if (*begin == '\"')
			{
				end = begin + 1;
				while (!EndOfStream() && *end != '\"') end++;
				if (EndOfStream())
					__debugbreak();
				end++;
			}
			else if (*begin == '}' || *begin == '{')
				end = begin + 1;
			else
				__debugbreak();
		}

		bool EndOfStream() const { return *begin == 0 || begin == final; }
		const char* Top() const { return begin; }
		const char* End() const { return end; }

	private:
		const char* begin;
		const char* end;
		const char* final;
	};

	void ParseEntities(BspData& bspData, const char* begin, const char* end)
	{
		Tokenizer tokenizer{ begin, end };
		BspEntity entity;
		std::string key;
		std::string value;

		for (;;)
		{
			tokenizer.Next();
			if (tokenizer.EndOfStream())
				break;
			if (*tokenizer.Top() != '{')
				__debugbreak();

			entity.clear();
			for (;;)
			{
				tokenizer.Next();
				if (*tokenizer.Top() == '}')
					break;
				if (*tokenizer.Top() != '\"')
					__debugbreak();
				key = std::string(tokenizer.Top() + 1, tokenizer.End() - 1);
				tokenizer.Next();
				if (*tokenizer.Top() != '\"')
					__debugbreak();
				value = std::string(tokenizer.Top() + 1, tokenizer.End() - 1);

				entity[key] = value;
			}
			bspData.entities.emplace_back(entity);
		}
	}

	template<typename TStream>
	StreamResult Stream(TStream& stream, BspHeader& header)
	{
		Stream(stream, header.signature);
		Stream(stream, header.version);
		if (header.signature != 0x50534249 || header.version != 38)
			return StreamResult::Fail;
		return StreamResult::Ok;
	}

	template<typename TStream>
	StreamResult Stream(TStream& stream, BspLump& lump)
	{
		Stream(stream, lump.offset);
		Stream(stream, lump.length);
		return StreamResult::Ok;
	}

	template<typename TStream>
	StreamResult Stream(TStream& stream, BspEdge& egde)
	{
		Stream(stream, egde.vId[0]);
		Stream(stream, egde.vId[1]);
		return StreamResult::Ok;
	}

	template<typename TStream>
	StreamResult Stream(TStream& stream, BspPlane& plane)
	{
		Stream(stream, plane.normal);
		Stream(stream, plane.distance);
		Stream(stream, plane.type);
		return StreamResult::Ok;
	}

	template<typename TStream>
	StreamResult Stream(TStream& stream, BspNode& node)
	{
		Stream(stream, node.plane);
		Stream(stream, node.frontChild);
		Stream(stream, node.backChild);
		Stream(stream, node.bboxMin);
		Stream(stream, node.bboxMax);
		Stream(stream, node.firstFace);
		Stream(stream, node.numFaces);
		return StreamResult::Ok;
	}

	template<typename TStream>
	StreamResult Stream(TStream& stream, BspFace& face)
	{
		Stream(stream, face.plane);
		Stream(stream, face.planeSide);
		Stream(stream, face.firstEdge);
		Stream(stream, face.numEdges);
		Stream(stream, face.textureInfo);
		Stream(stream, face.lightmapStyles);
		Stream(stream, face.lightmapOffset);
		return StreamResult::Ok;
	}

	template<typename TStream>
	StreamResult Stream(TStream& stream, BspTexInfo& texInfo)
	{
		Stream(stream, texInfo.uAxis);
		Stream(stream, texInfo.uOffset);
		Stream(stream, texInfo.vAxis);
		Stream(stream, texInfo.vOffset);
		Stream(stream, texInfo.flags);
		Stream(stream, texInfo.value);
		Stream(stream, texInfo.textureName);
		Stream(stream, texInfo.nextTexInfo);
		return StreamResult::Ok;
	}

	template<typename TStream, typename T>
	StreamResult Stream(TStream& stream, size_t fileOffset, std::vector<T>& data, const BspLump& lump)
	{
		stream.seekg(fileOffset + lump.offset);
		data.resize(lump.length / sizeof(T));
		Stream(stream, data.begin(), data.end());
		return StreamResult::Ok;
	}

	template<typename TStream>
	StreamResult Stream(TStream& stream, size_t fileOffset, std::vector<char>& data, const BspLump& lump)
	{
		stream.seekg(fileOffset + lump.offset);
		data.resize(lump.length);
		Stream(stream, data);
		return StreamResult::Ok;
	}

	template<typename TStream>
	StreamResult Stream(TStream& stream, BspData& bspData)
	{
		static_assert(StreamTraits<TStream>::IsInput(), "Only supports Input streams");
		static_assert(StreamTraits<TStream>::IsBinary(), "Only supports Binary streams");

		size_t fileOffset = stream.tellg();
		BspHeader header;
		if (Stream(stream, header) != StreamResult::Ok)
			return StreamResult::Fail;

		std::array<BspLump, BspLump_Count> lumps;
		Stream(stream, lumps.begin(), lumps.end());
		Stream(stream, fileOffset, bspData.vertices, lumps[BspLump_Vertices]);
		Stream(stream, fileOffset, bspData.edges, lumps[BspLump_Edges]);
		Stream(stream, fileOffset, bspData.nodes, lumps[BspLump_Nodes]);
		Stream(stream, fileOffset, bspData.faceEdges, lumps[BspLump_FaceEdgeTable]);
		Stream(stream, fileOffset, bspData.faces, lumps[BspLump_Faces]);
		Stream(stream, fileOffset, bspData.planes, lumps[BspLump_Planes]);
		Stream(stream, fileOffset, bspData.texInfo, lumps[BspLump_TextureInformation]);

		std::vector<char> entityScript;
		Stream(stream, fileOffset, entityScript, lumps[BspLump_Entities]);
		ParseEntities(bspData, entityScript.data(), entityScript.data() + entityScript.size());

		for (auto& vertex : bspData.vertices)
			vertex = vertex * bspUnitsToMeters;
		for (auto& texInfo : bspData.texInfo)
		{
			texInfo.uAxis = texInfo.uAxis / bspUnitsToMeters;
			texInfo.vAxis = texInfo.vAxis / bspUnitsToMeters;
			texInfo.uOffset = texInfo.uOffset / bspUnitsToMeters;
			texInfo.vOffset = texInfo.vOffset / bspUnitsToMeters;
		}

		return StreamResult::Ok;
	}



	template<typename TStream>
	GPUBufferPtr LoadTextureM8(TStream& stream, RenderDevicePtr renderDevice, const char* textureName)
	{
		static_assert(StreamTraits<TStream>::IsInput(), "Only supports Input streams");
		static_assert(StreamTraits<TStream>::IsBinary(), "Only supports Binary streams");

		const size_t fileOffset = stream.tellg();

		uint32 signature;
		Stream(stream, signature);
		assert(signature == 0x02);

		char name[32];
		Stream(stream, name);

		const uint maxNumMips = 16;
		std::array<uint32, maxNumMips> widths;
		std::array<uint32, maxNumMips> heigths;
		std::array<uint32, maxNumMips> offsets;
		Stream(stream, widths.begin(), widths.end());
		Stream(stream, heigths.begin(), heigths.end());
		Stream(stream, offsets.begin(), offsets.end());

		uint numMips = 0;
		uint bufferSize = 0;
		for (uint i = 0; i < 16; ++i)
		{
			if (widths[i] == 0 || heigths[i] == 0)
				break;
			bufferSize += widths[i] * heigths[i];
			++numMips;
		}

		char nextFrame[32];
		Stream(stream, nextFrame);

		const uint paletteSize = 256 * 3;
		uint8 palette[paletteSize];
		Stream(stream, palette);

		std::vector<uint32> buffer;
		buffer.resize(bufferSize);
		GPUBufferDesc desc;
		desc.type = GPUBufferType::Texture2D;
		desc.usage = GPUUsage_ShaderResource;
		desc.width = widths[0];
		desc.height = heigths[0];
		desc.mipLevels = numMips;
		desc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		desc.name = textureName;

		std::vector<uint32>::iterator dstIt = buffer.begin();
		std::vector<uint8> inputBuffer;
		inputBuffer.resize(widths[0] * heigths[0]);
		for (uint i = 0; i < numMips; ++i)
		{
			uint bufferSz = widths[i] * heigths[i];
			stream.seekg(fileOffset + offsets[i]);
			// Stream(stream, inputBuffer.data(), bufferSz);
			stream.read((char*)inputBuffer.data(), bufferSz);
			for (uint j = 0; j < bufferSz; ++j)
			{
				const uint8 r = palette[inputBuffer[j] * 3 + 0];
				const uint8 g = palette[inputBuffer[j] * 3 + 1];
				const uint8 b = palette[inputBuffer[j] * 3 + 2];
				const uint32 color = (0xff << 24) | (b << 16) | (g << 8) | r;
				*dstIt = color;
				++dstIt;
			}
		}

		desc.dataPtr = buffer.data();
		desc.dataSize = bufferSize * sizeof(uint32);

		return renderDevice->CreateBuffer(desc);
	}
	


	struct LightKdNode
	{
		// first 2 bits:
		//	0 = leaf
		//	1 = x plane
		//	2 = y plane
		//	3 = z plane
		// For Nodes:
		//  30 bits for above child index
		// For Leafs:
		//	30 bits for light data index
		uint  data;
		float plane;
	};


	struct LightKdLeaf
	{
		//	Vector3f boundsMin;
		//	Vector3f boundsMax;
		uint     firstLightIdx;
		uint     numLightIdx;
	};


	struct LightCull
	{
		Vector3f position;
		float    cullRadiusSqr;
		int32    children[2] = { 0, 0 };	// negative: child is a leaf, otherwise, child is index-1
	};


	struct LightShade
	{
		Vector3f position;
		float radius;
		float  brightness;
		uint32 color;
	};



	class LightManager
	{
	public:
		void BuildFromBSP(const std::vector<BspEntity>& entities);
		void CreateBuffers(RenderDevicePtr renderDevice);
		GPUBufferPtr GetCullsBuffer() const { return lightCullsBuffer; }
		GPUBufferPtr GetShadesBuffer() const { return lightShadesBuffer; }
		GPUBufferPtr GetKdNodesBuffer() const { return lightKdNodesBuffer; }
		GPUBufferPtr GetKdLeafsBuffer() const { return lightKdLeafsBuffer; }
		GPUBufferPtr GetLightIdsBuffer() const { return lightIdsBuffer; }
		uint GetNumLightCulls() const { return lightCulls.size(); }

	private:
		void InsertLight(Vector3f position, float radius, LightShade shade);
		void BuildKdTree();
		void BuildKdNode(uint nodeIdx, Boxf bounds, const std::vector<uint>& shadeIds, uint depth, bool forceLeaf = false);
		void MakeNode(uint nodeIdx, float plane, uint axis, uint aboveChild);
		void MakeLeaf(Boxf bounds, uint nodeIdx, const std::vector<uint>& shadeIds);

	private:
		std::vector<LightKdNode> kdNodes;
		std::vector<LightKdLeaf> kdLeafs;
		std::vector<uint>        lightIds;
		std::vector<LightCull>   lightCulls;
		std::vector<LightShade>  lightShades;
		GPUBufferPtr             lightCullsBuffer;
		GPUBufferPtr             lightShadesBuffer;
		GPUBufferPtr             lightKdNodesBuffer;
		GPUBufferPtr             lightKdLeafsBuffer;
		GPUBufferPtr             lightIdsBuffer;
	};



	void LightManager::BuildFromBSP(const std::vector<BspEntity>& entities)
	{
		lightCulls.clear();
		lightShades.clear();
		for (const auto& entity : entities)
		{
			auto it = entity.find("classname");
			if (it == entity.end() || it->second != "light")
				continue;
			LightCull lightCull;
			LightShade lightShade;

			it = entity.find("origin");
			if (it == entity.end())
				continue;
			sscanf_s(
				it->second.c_str(), "%f %f %f",
				&lightCull.position.x, &lightCull.position.y, &lightCull.position.z);
			lightCull.position = lightCull.position * bspUnitsToMeters;
			lightShade.position = lightCull.position;

			it = entity.find("_color");
			float r = 1.0f, g = 1.0f, b = 1.0f;
			if (it != entity.end())
				sscanf_s(it->second.c_str(), "%f %f %f", &r, &g, &b);
			lightShade.color = (uint(r * 255.0f) << 16) | (uint(g * 255.0f) << 8) | uint(b * 255.0f);

			it = entity.find("light");
			lightShade.brightness = 300.0f;
			if (it != entity.end())
				sscanf_s(it->second.c_str(), "%f", &lightShade.brightness);
			lightShade.brightness = lightShade.brightness * bspUnitsToMeters;
			lightCull.cullRadiusSqr = Square(lightShade.brightness * 4.0f);
			// lightShade.radius = lightShade.brightness * 4.0f;
			lightShade.radius = lightShade.brightness * 2.0f;

			//	lightCulls.push_back(lightCull);
			lightShades.push_back(lightShade);

			//	InsertLight(lightCull.position, lightShade.brightness * 4.0f, lightShade);
		}
		BuildKdTree();
	}



	void LightManager::InsertLight(Vector3f position, float radius, LightShade shade)
	{
		uint shadeLightId = lightShades.size();
		lightShades.push_back(shade);

		if (lightCulls.empty())
		{
			LightCull newCull;
			newCull.position = position;
			newCull.cullRadiusSqr = Square(radius);
			newCull.children[0] = -shadeLightId - 1;
			lightCulls.push_back(newCull);
		}
		else
		{
			uint curCullId = 0;
			while (lightCulls[curCullId].children[0] >= 0)
			{
				const uint child0 = lightCulls[curCullId].children[0];
				const uint child1 = lightCulls[curCullId].children[1];
				const Vector3f pos0 = lightCulls[child0].position;
				const Vector3f pos1 = lightCulls[child1].position;
				const float dist0 = Distance(position, pos0);
				const float dist1 = Distance(position, pos1);
				if (dist0 < dist1)
					curCullId = child0;
				else
					curCullId = child1;
			}
			LightCull newCull = lightCulls[curCullId];
			uint newCullId = lightCulls.size();
			lightCulls.push_back(newCull);
			lightCulls[curCullId].children[0] = newCullId;

			newCull.position = position;
			newCull.cullRadiusSqr = Square(radius);
			newCull.children[0] = -shadeLightId;
			newCullId = lightCulls.size();
			lightCulls.push_back(newCull);
			lightCulls[curCullId].children[1] = newCullId;
		}
	}



	Boxf SphereToBox(Vector3f center, float radius)
	{
		Boxf bounds;
		bounds.min = center - Vector3f(radius, radius, radius);
		bounds.max = center + Vector3f(radius, radius, radius);
		return bounds;
	}



	bool IsFullyContained(Boxf box, Boxf testBox)
	{
		if (testBox.min.x >= box.min.x && testBox.max.x <= box.max.x &&
			testBox.min.y >= box.min.y && testBox.max.y <= box.max.y &&
			testBox.min.z >= box.min.z && testBox.max.z <= box.max.z)
			return true;
		return false;
	}


	bool Intersects(Boxf box0, Boxf box1)
	{
		if (box0.max.x < box1.min.x) return false;
		if (box0.min.x > box1.max.x) return false;
		if (box0.max.y < box1.min.y) return false;
		if (box0.min.y > box1.max.y) return false;
		if (box0.max.z < box1.min.z) return false;
		if (box0.min.z > box1.max.z) return false;
		return true;
	}



	uint MaximumExtent(Boxf box)
	{
		const Vector3f v = box.max - box.min;
		if (v.x > v.y && v.x > v.z)
			return 0;
		if (v.y > v.z)
			return 1;
		return 2;
	}


	// based on: https://github.com/mmp/pbrt-v3/blob/master/src/accelerators/kdtreeaccel.cpp
	// It does utilize a much simpler and faster heuristic however
	void LightManager::BuildKdTree()
	{
		kdNodes.clear();
		Boxf bounds = SphereToBox(lightShades[0].position, lightShades[0].radius);
		std::vector<uint> shadeIds;
		shadeIds.reserve(lightShades.size());
		for (const auto shade : lightShades)
		{
			bounds = Union(bounds, SphereToBox(shade.position, shade.radius));
			shadeIds.push_back(shadeIds.size());
		}
		BuildKdNode(0, bounds, shadeIds, 0);
	}



	void LightManager::MakeNode(uint nodeIdx, float plane, uint axis, uint aboveChild)
	{
		kdNodes[nodeIdx].plane = plane;
		kdNodes[nodeIdx].data = axis + 1;
		kdNodes[nodeIdx].data |= aboveChild << 2;
	}


	void LightManager::MakeLeaf(Boxf bounds, uint nodeIdx, const std::vector<uint>& shadeIds)
	{
		kdNodes[nodeIdx].plane = 0;
		kdNodes[nodeIdx].data = kdLeafs.size() << 2;

		LightKdLeaf leaf;
		leaf.firstLightIdx = lightIds.size();
		leaf.numLightIdx = shadeIds.size();
		lightIds.insert(lightIds.end(), shadeIds.begin(), shadeIds.end());
		kdLeafs.push_back(leaf);

		for (uint i = 0; i < shadeIds.size(); ++i)
		{
			const uint shadeId = shadeIds[i];
			const Boxf shadeBounds = SphereToBox(lightShades[shadeId].position, lightShades[shadeId].radius);
			if (!Intersects(bounds, shadeBounds))
				__debugbreak();
		}
	}



	void LightManager::BuildKdNode(uint nodeIdx, Boxf bounds, const std::vector<uint>& shadeIds, uint depth, bool forceLeaf)
	{
		kdNodes.emplace_back();

		//	const uint maxDepth = 10;
		const uint maxDepth = 20;
		const uint maxShades = 1;
		if (forceLeaf || shadeIds.size() <= maxShades || depth >= maxDepth)
		{
			MakeLeaf(bounds, nodeIdx, shadeIds);
			return;
		}

		int axis = MaximumExtent(bounds);
		Vector3f d = bounds.max - bounds.min;
	//	float bestOffset = bounds.min[axis] + d[axis] * 0.5f;
		float bestOffset = bounds.min[axis];
		float bestHeuristic = d[axis];

		for (uint i = 0; i < shadeIds.size() * 2; ++i)
		{
			const bool testMax = i >= shadeIds.size();
			const uint j = testMax ? i - shadeIds.size() : i;
			const uint shadeId = shadeIds[j];
			const Boxf shadeBounds = SphereToBox(lightShades[shadeId].position, lightShades[shadeId].radius);
			float testSplit = testMax ? shadeBounds.max[axis] : shadeBounds.min[axis];
			if (testSplit < bounds.min[axis] || testSplit > bounds.max[axis])
				continue;
			// using volume heuristic, not SAH. We are looking up lights, not tracing them.
			// the closer the split is to the middle, the better
			const float heuristic = abs(testSplit - (bounds.min[axis] + d[axis] * 0.5f));
			if (heuristic < bestHeuristic)
			{
				bestOffset = testSplit;
				bestHeuristic = heuristic;
			}
		}

		Boxf leftBounds = bounds;
		Boxf rightBounds = bounds;
		leftBounds.max[axis] = bestOffset;
		rightBounds.min[axis] = bestOffset;

		std::vector<uint> leftPartitionShadeIds;
		std::vector<uint> rightPartitionShadeIds;
		leftPartitionShadeIds.reserve(shadeIds.size());
		rightPartitionShadeIds.reserve(shadeIds.size());
		for (uint i = 0; i < shadeIds.size(); ++i)
		{
			const float epsilon = 0.95f;
			const uint shadeId = shadeIds[i];
			const Boxf shadeBounds = SphereToBox(lightShades[shadeId].position, lightShades[shadeId].radius * epsilon);
			if (Intersects(shadeBounds, leftBounds))
				leftPartitionShadeIds.push_back(shadeId);
			if (Intersects(shadeBounds, rightBounds))
				rightPartitionShadeIds.push_back(shadeId);
		}
		if (leftPartitionShadeIds.size() == rightPartitionShadeIds.size() == shadeIds.size())
			__debugbreak();

		BuildKdNode(
			nodeIdx + 1, leftBounds, leftPartitionShadeIds, depth + 1,
			leftPartitionShadeIds.size() == shadeIds.size());
		MakeNode(nodeIdx, bestOffset, axis, kdNodes.size());
		BuildKdNode(
			kdNodes.size(), rightBounds, rightPartitionShadeIds, depth + 1,
			rightPartitionShadeIds.size() == shadeIds.size());
	}



	void LightManager::CreateBuffers(RenderDevicePtr renderDevice)
	{
		{
			GPUBufferDesc bufferDesc;
			bufferDesc.type = GPUBufferType::Buffer;
			bufferDesc.usage = GPUUsage_ShaderResource;
			bufferDesc.dataSize = lightShades.size() * sizeof(LightShade);
			bufferDesc.dataStride = sizeof(LightShade);
			bufferDesc.dataPtr = lightShades.data();
			bufferDesc.name = "lightShades";
			lightShadesBuffer = renderDevice->CreateBuffer(bufferDesc);
		}
		{
			GPUBufferDesc bufferDesc;
			bufferDesc.type = GPUBufferType::Buffer;
			bufferDesc.usage = GPUUsage_ShaderResource;
			bufferDesc.dataSize = kdNodes.size() * sizeof(LightKdNode);
			bufferDesc.dataStride = sizeof(LightKdNode);
			bufferDesc.dataPtr = kdNodes.data();
			bufferDesc.name = "lightKdNodes";
			lightKdNodesBuffer = renderDevice->CreateBuffer(bufferDesc);
		}
		{
			GPUBufferDesc bufferDesc;
			bufferDesc.type = GPUBufferType::Buffer;
			bufferDesc.usage = GPUUsage_ShaderResource;
			bufferDesc.dataSize = kdLeafs.size() * sizeof(LightKdLeaf);
			bufferDesc.dataStride = sizeof(LightKdLeaf);
			bufferDesc.dataPtr = kdLeafs.data();
			bufferDesc.name = "lightkdLeafs";
			lightKdLeafsBuffer = renderDevice->CreateBuffer(bufferDesc);
		}
		{
			GPUBufferDesc bufferDesc;
			bufferDesc.type = GPUBufferType::Buffer;
			bufferDesc.usage = GPUUsage_ShaderResource;
			bufferDesc.dataSize = lightIds.size() * sizeof(uint32);
			bufferDesc.dataStride = sizeof(uint32);
			bufferDesc.dataPtr = lightIds.data();
			bufferDesc.name = "lightIds";
			lightIdsBuffer = renderDevice->CreateBuffer(bufferDesc);
		}
	}

}



void H2Dx12()
{
	using namespace h2;
	using namespace dx12;

	FileManager fileManager;
	fileManager.AddPak("data/h2dx11/Htic2-0.pak");
	fileManager.AddPak("data/h2dx11/Htic2-1.pak");

	bool hasCamera = false;
	Vector3f camTarget;
	Quaternionf camRot;
	FileManager::PakFileHandle map;
	float camDist = 10.0f;
	
	{
		map = fileManager.OpenFile("maps/ssdocks.bsp");
		camTarget = Vector3f(
			31.3377571f,
			-33.3413124f,
			-3.76843309f);
		camRot = Normalize(Quaternionf(
			-0.0863006115f,
			-0.0475851297f,
			0.868246019f,
			-0.486247569f));
		camDist = 31.8663692f;
		hasCamera = true;
	}
	
	// map = fileManager.OpenFile("maps/sstown.bsp");
	// map = fileManager.OpenFile("maps/sspalace.bsp");
	// map = fileManager.OpenFile("maps/andplaza.bsp");
	// map = fileManager.OpenFile("maps/andslums.bsp");
	// map = fileManager.OpenFile("maps/hive1.bsp");
	// map = fileManager.OpenFile("maps/kellcaves.bsp");
	// map = fileManager.OpenFile("maps/canyon.bsp");
	// map = fileManager.OpenFile("maps/oglemine1.bsp");
	// map = fileManager.OpenFile("maps/oglemine2.bsp");
	// map = fileManager.OpenFile("maps/tutorial.bsp");
	// map = fileManager.OpenFile("maps/tutorial2.bsp");
	// map = fileManager.OpenFile("maps/dmandoria.bsp");

	if (!map)
		return;

	BspData bspData;
	if (Stream(*map, bspData) != h2::StreamResult::Ok)
		return;

	bspData.texInfoToTexture.resize(bspData.texInfo.size());
	for (uint i = 0; i < bspData.texInfo.size(); ++i)
	{
		BspTexInfo& texInfo = bspData.texInfo[i];

		auto it = bspData.textureMap.find(texInfo.textureName);
		uint textureId;
		if (it == bspData.textureMap.end())
		{
			BspTexture m8Data;
			m8Data.name = std::string("textures/") + texInfo.textureName + ".m8";
			FileManager::PakFileHandle m8 = fileManager.OpenFile(m8Data.name.c_str());
			if (m8)
				StreamM8(*m8, m8Data);
			else
				__debugbreak();
			textureId = bspData.textures.size();
			bspData.textures.push_back(m8Data);
			bspData.textureMap[texInfo.textureName] = textureId;
		}
		else
		{
			textureId = it->second;
		}
		const BspTexture& texture = bspData.textures[textureId];
		texInfo.uAxis = texInfo.uAxis / texture.width;
		texInfo.vAxis = texInfo.vAxis / texture.heigth;
		texInfo.uOffset = texInfo.uOffset / texture.width;
		texInfo.vOffset = texInfo.vOffset / texture.heigth;
		bspData.texInfoToTexture[i] = textureId;
	}

	struct Vertex
	{
		Vector3f position;
		Vector3f normal;
		Vector2f texCoord;
	};

	Boxf bBox;
	bBox.min = Vector3f(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	bBox.max = Vector3f(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	struct Batch
	{
		std::vector<Vertex> vertices;
		uint first = 0;
		uint count = 0;
		uint textureId = 0;
	};
	std::vector<Batch> batches;
	batches.resize(bspData.textures.size());
	for (const BspFace& face : bspData.faces)
	{
		const BspTexInfo& texInfo = bspData.texInfo[face.textureInfo];
		const uint noDraw = BspSurf_NODRAW | BspSurf_SKY;
		if (texInfo.flags & noDraw)
			continue;

		Batch& batch = batches[bspData.texInfoToTexture[face.textureInfo]];
		std::vector<Vertex>& vertices = batch.vertices;
		const Vector3f normal = Normalize(bspData.planes[face.plane].normal) * (face.planeSide ? -1.0f : 1.0f);

		auto GetVertexId = [&bspData, face](uint i)
		{
			int edgeId = bspData.faceEdges[face.firstEdge + i];
			return edgeId > 0 ? bspData.edges[edgeId].vId[1] : bspData.edges[-edgeId].vId[0];
		};
		auto GetTexCoord = [&texInfo](Vector3f vertex)
		{
			return Vector2f(
				vertex.x * texInfo.uAxis.x + vertex.y * texInfo.uAxis.y + vertex.z * texInfo.uAxis.z + texInfo.uOffset,
				vertex.x * texInfo.vAxis.x + vertex.y * texInfo.vAxis.y + vertex.z * texInfo.vAxis.z + texInfo.vOffset);
		};

		Vertex v0;
		v0.position = bspData.vertices[GetVertexId(0)];
		v0.normal = normal;
		v0.texCoord = GetTexCoord(v0.position);

		Vertex v1;
		v1.position = bspData.vertices[GetVertexId(1)];
		v1.normal = normal;
		v1.texCoord = GetTexCoord(v1.position);

		for (uint vertIdx = 2; vertIdx < face.numEdges; ++vertIdx)
		{
			Vertex v2;
			v2.position = bspData.vertices[GetVertexId(vertIdx)];
			v2.normal = normal;
			v2.texCoord = GetTexCoord(v2.position);

			bBox = Union(bBox, v0.position);
			bBox = Union(bBox, v1.position);
			bBox = Union(bBox, v2.position);

			vertices.push_back(v0);
			vertices.push_back(v1);
			vertices.push_back(v2);
			v1 = v2;
		}
	}
	SDBMHashMap<Vertex, uint32> vertexMap;
	vector<Vertex> vertices;
	vector<uint32> indices;
	vector<Batch> solidBatches;
	vector<Batch> oitBatches;
	for (uint i = 0; i < batches.size(); ++i)
	{
		Batch& batch = batches[i];
		batch.first = indices.size();
		batch.count = batch.vertices.size();
		batch.textureId = i;
		for (const Vertex& vertex : batch.vertices)
		{
			auto it = vertexMap.find(vertex);
			if (it == vertexMap.end())
			{
				uint idx = vertices.size();
				vertexMap[vertex] = idx;
				vertices.push_back(vertex);
				indices.push_back(idx);
			}
			else
				indices.push_back(it->second);
		}
		batch.vertices.clear();
		batch.vertices.shrink_to_fit();

		if (batch.count != 0)
		{
			const BspTexture& texture = bspData.textures[i];
			const uint transparent = BspSurf_TRANS33 | BspSurf_TRANS66;
			if (texture.flags & transparent)
				oitBatches.push_back(batch);
			else
				solidBatches.push_back(batch);
		}
	}
	vertexMap.clear();

	//////////////////////////////////////////////////////////////////////////

	const uint width = 1280;
	const uint height = 720;

	Dx12CanvasListener canvasListener;

	CanvasDesc canvasDesc{};
	canvasDesc.width = width;
	canvasDesc.height = height;
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	canvas->AddListener(&canvasListener);
	Assert(canvas != nullptr);

	// init Dx12
	RenderDeviceDesc renderDeviceDesc;
	renderDeviceDesc.enableValidation = true;

	SwapChainDesc swapChainDesc;
	swapChainDesc.windowHandle = canvas->GetHandle();
	swapChainDesc.width = width;
	swapChainDesc.height = height;

	RenderDevicePtr renderDevice = RenderDevice::Create(renderDeviceDesc);
	SwapChainPtr swapChain =  renderDevice->CreateSwapChain(swapChainDesc);

	// create some resources
	ShaderManager shaderManager;
	GPUBufferPtr vertexBuffer;
	GPUBufferPtr indexBuffer;
	GPUBufferPtr viewCB;
	GraphicsPSOPtr pso;
	vector<DrawBatch> drawBatches;
	vector<ResourceSetPtr> batchesRSs;
	DescriptorHandle sampler;
	GPUBufferPtr depthBuffer;
	GPUBufferPtr gBuffer0;      // rgb-Diffuse  a-
	GPUBufferPtr gBuffer1;      // rgb-Normal
	GPUBufferPtr finalBuffer;
	ResourceSetPtr viewRS;
	PD3D12RootSignature finalizeRS;
	PD3D12PipelineState finalizePSO;
	GPUBufferPtr momentsBuffer;
	LightManager lightManager;
	struct ViewConsts
	{
		Matrix44f projViewTM;
		Matrix44f invProjViewTM;
		Vector2f  viewportSize;
		Vector2f  invViewportSize;
		Vector3f  viewPosition;
		uint      frameIdx;
	} viewConsts;
	RenderPassPtr renderPass;
	// Create GBuffer
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.usage = GPUUsage_ShaderResource | GPUUsage_DepthStencil;
		bufferDesc.width = width;
		bufferDesc.height = height;
		bufferDesc.format = DXGI_FORMAT_D32_FLOAT;
		bufferDesc.name = "DepthBuffer";
		depthBuffer = renderDevice->CreateBuffer(bufferDesc);
						
		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.usage = GPUUsage_ShaderResource | GPUUsage_RenderTarget | GPUUsage_UnorderedAccess;
		bufferDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		bufferDesc.name = "gBuffer0";
		gBuffer0 = renderDevice->CreateBuffer(bufferDesc);

		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.usage = GPUUsage_ShaderResource | GPUUsage_RenderTarget | GPUUsage_UnorderedAccess;
		bufferDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		bufferDesc.name = "gBuffer1";
		gBuffer1 = renderDevice->CreateBuffer(bufferDesc);
	}

	// create final buffer
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.usage = GPUUsage_UnorderedAccess | GPUUsage_RenderTarget;
		bufferDesc.width = width;
		bufferDesc.height = height;
		bufferDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		bufferDesc.name = "FinalBuffer";
		finalBuffer = renderDevice->CreateBuffer(bufferDesc);
	}

	// render pass
	{
		RenderPassDesc desc;
		desc.rtvs.push_back(gBuffer0);
		desc.rtvs.push_back(gBuffer1);
		desc.dsv = depthBuffer;
		desc.viewport.Width = width;
		desc.viewport.Height = height;
		renderPass = renderDevice->CreateRenderPass(desc, "final");
	}

	// Describe and create the graphics pipeline state object (PSO).
	{
		GraphicsPSODesc psoDesc;

		psoDesc.geometryDesc.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, position), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		psoDesc.geometryDesc.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, offsetof(Vertex, normal), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		psoDesc.geometryDesc.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, offsetof(Vertex, texCoord), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
		psoDesc.vertexShader = shaderManager.CompileShaderFile("data/h2dx12/gbuffer.hlsl", "VSMain", ShaderType::VertexShader);
		psoDesc.pixelShader = shaderManager.CompileShaderFile("data/h2dx12/gbuffer.hlsl", "PSMain", ShaderType::PixelShader);
		psoDesc.renderPass = renderPass;
		psoDesc.depthStencil.DepthEnable = true;
		psoDesc.depthStencil.StencilEnable = false;
		psoDesc.depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
		psoDesc.depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		pso = renderDevice->CreateGraphicsPSO(psoDesc);
	}

	// create vertex buffer
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::VertexBuffer;
		bufferDesc.dataSize = vertices.size() * sizeof(Vertex);
		bufferDesc.dataStride = sizeof(Vertex);
		bufferDesc.dataPtr = vertices.data();
		bufferDesc.name = "VertexBuffer";
		vertexBuffer = renderDevice->CreateBuffer(bufferDesc);
	}

	// create index buffer
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::IndexBuffer;
		bufferDesc.dataSize = indices.size() * sizeof(uint32);
		bufferDesc.format = DXGI_FORMAT_R32_UINT;
		bufferDesc.dataStride = sizeof(uint32);
		bufferDesc.dataPtr = indices.data();
		bufferDesc.name = "IndexBuffer";
		indexBuffer = renderDevice->CreateBuffer(bufferDesc);
	}

	// create cbuffer
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::ConstantBuffer;
		bufferDesc.usage = GPUUsage_Dynamic;
		bufferDesc.dataSize = sizeof(ViewConsts);
		bufferDesc.name = "viewCB";
		viewCB = renderDevice->CreateBuffer(bufferDesc);
	}

	// create M8 textures
	{
		bspData.gpuTextures.resize(bspData.textures.size());
		batchesRSs.resize(bspData.textures.size());
		for (uint i = 0; i < bspData.textures.size(); ++i)
		{
			batchesRSs[i] = renderDevice->CreateResourceSet();
			FileManager::PakFileHandle m8 = fileManager.OpenFile(bspData.textures[i].name.c_str());
			if (m8)
			{
				bspData.gpuTextures[i] = LoadTextureM8(*m8, renderDevice, bspData.textures[i].name.c_str());
				batchesRSs[i]->AddShaderResource(0, bspData.gpuTextures[i]);
				renderDevice->PrepareResourceSet(batchesRSs[i]);
			}
		}
	}

	// create a sampler
	{
		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.Filter = D3D12_FILTER_ANISOTROPIC;
		samplerDesc.MaxAnisotropy = 8;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = std::numeric_limits<float>::max();
		sampler = renderDevice->CreateSampler(samplerDesc);
	}

	// draw batches
	{
		drawBatches.reserve(batches.size());

		DrawBatch drawBatch;
		drawBatch.mode = DrawBatchMode::DrawIndexed;
		drawBatch.topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		drawBatch.pso = pso;
		drawBatch.AddVertexBuffer(vertexBuffer);
		drawBatch.indexBuffer = indexBuffer;
		for (const auto& batch : solidBatches)
		{
			drawBatch.startVertexLocation = batch.first;
			drawBatch.vertexCountPerInstance = batch.count;
			drawBatches.push_back(drawBatch);
		}

		viewRS = renderDevice->CreateResourceSet();
		viewRS->AddConstants(0, viewCB);
		renderDevice->PrepareResourceSet(viewRS);
	}

	// Create raytrace output Buffer
	GPUBufferPtr dxrOutput;
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.usage = GPUUsage_ShaderResource | GPUUsage_UnorderedAccess;
		bufferDesc.width = width;
		bufferDesc.height = height;
		bufferDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		bufferDesc.name = "dxrOutput";
		dxrOutput = renderDevice->CreateBuffer(bufferDesc);

		bufferDesc.name = "momentsBuffer";
		bufferDesc.usage |= GPUUsage_RenderTarget;	// so can be cleared
		momentsBuffer = renderDevice->CreateBuffer(bufferDesc);
	}

	// temporal
	GPUBufferPtr temporalOut;
	GPUBufferPtr temporalCB;
	ComputePtr svgfTemporalPass;
	{
		GPUBufferDesc bufferDesc;
		bufferDesc.type = GPUBufferType::Texture2D;
		bufferDesc.usage = GPUUsage_ShaderResource | GPUUsage_UnorderedAccess;
		bufferDesc.usage |= GPUUsage_RenderTarget;	// so can be cleared
		bufferDesc.width = width;
		bufferDesc.height = height;
		bufferDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		bufferDesc.name = "temporalOut";
		temporalOut = renderDevice->CreateBuffer(bufferDesc);
	}
	{
		GPUBufferDesc cbDesc;
		cbDesc.type = GPUBufferType::ConstantBuffer;
		cbDesc.usage = GPUUsage_Dynamic;
		cbDesc.dataSize = sizeof(uint);
		cbDesc.name = "temporalCB";
		temporalCB = renderDevice->CreateBuffer(cbDesc);

		ResourceSetPtr rs = renderDevice->CreateResourceSet();
		rs->AddShaderResource(0, dxrOutput);
		rs->AddShaderResource(1, gBuffer0);
		rs->AddShaderResource(2, gBuffer1);
		rs->AddShaderResource(3, depthBuffer);
		rs->AddConstants(0, temporalCB);
		rs->AddUnorderedAccess(0, temporalOut);
		rs->AddUnorderedAccess(1, momentsBuffer);

		ResourceLayoutPtr rl = renderDevice->CreateResourceLayout();
		rl->AddResourceSet(0, rs);

		ComputeDesc desc;
		desc.resourceLayout = rl;
		desc.shader = shaderManager.CompileShaderFile("data/h2dx12/csSvgfTemporal.hlsl", "csMain", ShaderType::ComputeShader);
		svgfTemporalPass = renderDevice->CreateCompute(desc);
	}

	// atrous
	vector<ComputePtr> svgfAtrousPasses;
//	vector<GPUBufferPtr> svgfAtrousCBs;	// TODO = implement inline CBs, more efficient
	GPUBufferPtr atrousOut;
	GPUBufferPtr temporalCpySrc;
	array<GPUBufferPtr, 2> atrousTemp;
	{
		GPUBufferDesc bufferDesc = dxrOutput->desc;
		bufferDesc.name = "atrousOut";
		atrousOut = renderDevice->CreateBuffer(bufferDesc);
		bufferDesc.name = "atrousTemp0";
		atrousTemp[0] = renderDevice->CreateBuffer(bufferDesc);
		bufferDesc.name = "atrousTemp1";
		atrousTemp[1] = renderDevice->CreateBuffer(bufferDesc);

		struct CB
		{
			uint     step;
			Vector2f depthFactors;
			float    _;
			uint width;
			uint height;
			uint __[2];
			Vector3f gauss;
			float ___;
		};
		GPUBufferDesc cbDesc;
		cbDesc.type = GPUBufferType::ConstantBuffer;
		cbDesc.usage = GPUUsage_Dynamic;
		cbDesc.dataSize = sizeof(CB);
		cbDesc.name = "svgfAtrousCBs";

		ComputeDesc desc;
		desc.shader = shaderManager.CompileShaderFile("data/h2dx12/csSvgfAtrous.hlsl", "csMain", ShaderType::ComputeShader);

		auto Gaussian = [](float sigma, float x)
		{
			const float sigma2 = sigma * sigma;
			const float x2 = x * x;
			return 1.0f / sqrt(2 * PI * sigma2) * exp(-x2 / (2 * sigma2));
		};

		const uint numSamples = 4;
		uint tempSrc = 0;
		uint tempDst = 1;
		uint step = 1;
		for (uint i = 0; i < numSamples; ++i)
		{
			// TODO = implement inline CBs, more efficient
			CB cbData;
			cbData.step = step;
			const float n = 1.0f / 64.0f;
			const float f = 10000.0f;
			cbData.depthFactors = Vector2f(1 - (f / n), f / n);
			cbData.width = width;
			cbData.height = height;
			// cbData.gauss = Vector3f(Gaussian(1.0f / step, -1.0f), Gaussian(1.0f / step, 0.0f), Gaussian(1.0f / step, 1.0f));
			// cbData.gauss = Vector3f(Gaussian(step, -1.0f), Gaussian(step, 0.0f), Gaussian(step, 1.0f));
			// cbData.gauss = Vector3f(Gaussian(1.0f, -1.0f * step), Gaussian(1.0f, 0.0f), Gaussian(1.0f, 1.0f * step));
			// cbData.gauss = Vector3f(Gaussian(step, -1.0f * step), Gaussian(step, 0.0f), Gaussian(step, 1.0f * step));
			cbData.gauss = Vector3f(1.0f / 4.0f, 1.0f / 2.0f, 1.0f / 4.0f);
			// cbData.gauss = Vector3f(1.0f, 1.0f, 1.0f);
			cbDesc.dataPtr = &cbData;
			GPUBufferPtr cb = renderDevice->CreateBuffer(cbDesc);

			ResourceSetPtr rs = renderDevice->CreateResourceSet();
			if (i == 0)
				rs->AddShaderResource(0, temporalOut);
			else
				rs->AddShaderResource(0, atrousTemp[tempSrc]);
						
			GPUBufferPtr dst;
			if (i >= numSamples - 1)
				dst = atrousOut;
			else
				dst = atrousTemp[tempDst];
			rs->AddUnorderedAccess(0, dst);
			if (i == 0)
				temporalCpySrc = dst;

			rs->AddShaderResource(1, gBuffer0);
			rs->AddShaderResource(2, gBuffer1);
			rs->AddShaderResource(3, depthBuffer);	// TODO  add linear depth buffer pass
			rs->AddConstants(0, cb);

			ResourceLayoutPtr rl = renderDevice->CreateResourceLayout();
			rl->AddResourceSet(0, rs);

			desc.resourceLayout = rl;
			svgfAtrousPasses.push_back(renderDevice->CreateCompute(desc));

			swap(tempSrc, tempDst);
			step *= 2;
		}
	}

	// finalize compute
	ComputePtr finalizePass;
	{
		ResourceSetPtr rs = renderDevice->CreateResourceSet();
		rs->AddShaderResource(0, gBuffer0);
		rs->AddShaderResource(1, atrousOut);
		rs->AddUnorderedAccess(0, finalBuffer);

		ResourceLayoutPtr rl = renderDevice->CreateResourceLayout();
		rl->AddResourceSet(0, rs);

		ComputeDesc desc;
		desc.shader = shaderManager.CompileShaderFile("data/h2dx12/csFinalize.hlsl", "csMain", ShaderType::ComputeShader);
		desc.resourceLayout = rl;
		finalizePass = renderDevice->CreateCompute(desc);
	}

	// create lights
	{
		lightManager.BuildFromBSP(bspData.entities);
		lightManager.CreateBuffers(renderDevice);
	}

	// Prepare raytracing resources
	enum class GlobalRootSignatureParams
	{
		OutputViewSlot = 0,
		AccelerationStructureSlot,
		Count,
	};
	enum class LocalRootSignatureParams
	{
		ViewportConstantSlot = 0,
		Count,
	};
	PD3D12Device5 device5 = renderDevice->GetDevice5();
	PD3D12Resource bottomLevelAccelerationStructure;
	PD3D12Resource topLevelAccelerationStructure;
	PD3D12Resource instanceDescs;
	PD3D12Resource rayGenShaderTable;
	PD3D12Resource missShaderTable;
	PD3D12Resource hitGroupShaderTable;
	ShaderPtr dxrShader;
	PD3D12RootSignature raytracingGlobalRootSignature;
	PD3D12RootSignature raytracingLocalRootSignature;
	Pointer<ID3D12StateObject> stateObject;
	Pointer<ID3D12StateObjectProperties> stateObjectProperties;
	vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDescs;
	D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
	ResourceSetPtr globalRS;
	ResourceSetPtr rayGenRS;
	{
		// prepare resources
		{
			globalRS = renderDevice->CreateResourceSet();
			globalRS->AddUnorderedAccess(0, dxrOutput);
			renderDevice->PrepareResourceSet(globalRS);

			rayGenRS = renderDevice->CreateResourceSet();
			rayGenRS->AddConstants(0, viewCB);
			rayGenRS->AddShaderResource(1, gBuffer0);
			rayGenRS->AddShaderResource(2, gBuffer1);
			rayGenRS->AddShaderResource(3, depthBuffer);
			rayGenRS->AddShaderResource(4, lightManager.GetKdNodesBuffer());
			rayGenRS->AddShaderResource(5, lightManager.GetKdLeafsBuffer());
			rayGenRS->AddShaderResource(6, lightManager.GetLightIdsBuffer());
			rayGenRS->AddShaderResource(7, lightManager.GetShadesBuffer());
			rayGenRS->AddUnorderedAccess(1, momentsBuffer);
			renderDevice->PrepareResourceSet(rayGenRS);
		}

		// root signatures
		{
			CD3DX12_ROOT_PARAMETER rootParameters[2];
			rootParameters[0].InitAsDescriptorTable(1, &CD3DX12_DESCRIPTOR_RANGE{ D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0 });
			rootParameters[1].InitAsShaderResourceView(0);
			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters);

			PD3D12RootSignature rootSig;
			Pointer<ID3DBlob> blob;
			Pointer<ID3DBlob> error;
			AssertHr(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob.Get(), &error.Get()));
			AssertHr(device5->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&raytracingGlobalRootSignature.Get())));
		}
		{
			vector<D3D12_DESCRIPTOR_RANGE> ranges;
			ranges.reserve(rayGenRS->ranges.size());
			for (uint i = 0; i < rayGenRS->ranges.size(); ++i)
			{
				if (rayGenRS->ranges[i].BaseShaderRegister != -1)
					ranges.push_back(rayGenRS->ranges[i]);
			}
			CD3DX12_ROOT_PARAMETER rootParameters[1];
			rootParameters[0].InitAsDescriptorTable(ranges.size(), ranges.data());

			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters);
			rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE;

			PD3D12RootSignature rootSig;
			Pointer<ID3DBlob> blob;
			Pointer<ID3DBlob> error;
			AssertHr(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob.Get(), &error.Get()));
			AssertHr(device5->CreateRootSignature(1, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&raytracingLocalRootSignature.Get())));
		}
		
		// shader identifier
		{
			dxrShader = shaderManager.CompileShaderFile("data/shaders/dxrshader.hlsl", nullptr, ShaderType::Library);

			D3D12_EXPORT_DESC exports[] =
			{
				{ L"rgMain", nullptr, D3D12_EXPORT_FLAG_NONE },
				{ L"msMain", nullptr, D3D12_EXPORT_FLAG_NONE },
				{ L"chMain", nullptr, D3D12_EXPORT_FLAG_NONE },
			};

			D3D12_DXIL_LIBRARY_DESC dxilLib{};
			dxilLib.DXILLibrary.pShaderBytecode = dxrShader->GetBufferPointer();
			dxilLib.DXILLibrary.BytecodeLength = dxrShader->GetBufferSize();
			dxilLib.pExports = exports;
			dxilLib.NumExports = _countof(exports);

			D3D12_RAYTRACING_SHADER_CONFIG shaderConfig{};
			shaderConfig.MaxAttributeSizeInBytes = 2 * sizeof(float);
			shaderConfig.MaxPayloadSizeInBytes = 4 * sizeof(float);

			D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig{};
			pipelineConfig.MaxTraceRecursionDepth = 1;

			D3D12_HIT_GROUP_DESC hitGroupDesc{};
			hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
			hitGroupDesc.ClosestHitShaderImport = L"chMain";
			hitGroupDesc.HitGroupExport = L"HitGroup";

			D3D12_GLOBAL_ROOT_SIGNATURE global{};
			global.pGlobalRootSignature = raytracingGlobalRootSignature;

			D3D12_LOCAL_ROOT_SIGNATURE local{};
			local.pLocalRootSignature = raytracingLocalRootSignature;

			D3D12_STATE_SUBOBJECT more;
			more.Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			more.pDesc = &local;
			LPCWSTR chExports[] = { L"rgMain" };
			D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION exportAssociation{};
			exportAssociation.pExports = chExports;
			exportAssociation.NumExports = 1;
			exportAssociation.pSubobjectToAssociate = &more;

			vector<D3D12_STATE_SUBOBJECT> states;
			states.emplace_back();
			states.back().Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
			states.back().pDesc = &dxilLib;
			states.emplace_back();
			states.back().Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
			states.back().pDesc = &hitGroupDesc;
			states.emplace_back();
			states.back().Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
			states.back().pDesc = &shaderConfig;
			states.emplace_back();
			states.back().Type = D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE;
			states.back().pDesc = &local;
			states.emplace_back();
			states.back().Type = D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION;
			states.back().pDesc = &exportAssociation;
			states.emplace_back();
			states.back().Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
			states.back().pDesc = &global;
			states.emplace_back();
			states.back().Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
			states.back().pDesc = &pipelineConfig;

			D3D12_STATE_OBJECT_DESC psoDesc{};
			psoDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
			psoDesc.pSubobjects = states.data();
			psoDesc.NumSubobjects = states.size();

			AssertHr(device5->CreateStateObject(&psoDesc, IID_PPV_ARGS(&stateObject.Get())));
			stateObject->QueryInterface(IID_PPV_ARGS(&stateObjectProperties.Get()));
		}
		
		// shader table
		if (stateObjectProperties)
		{
			auto CreateShaderTable = [&](LPCWSTR exportName, void* data, uint dataSz)
			{
				PD3D12Resource table;

				uint shaderRecordSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES + dataSz;
				uint bufferSize = DivUp(shaderRecordSize, D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT) * D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;

				AssertHr(device5->CreateCommittedResource(
					&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
					D3D12_HEAP_FLAG_NONE,
					&CD3DX12_RESOURCE_DESC::Buffer(shaderRecordSize),
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&table.Get())));
				struct ShaderRef
				{
					void* shaderIdentifier;
					uint  shaderIdentifierSize;
					void* localRootArguments;
					uint  localRootArgumentsSize;
				} shaderRef;
				static_assert(sizeof(ShaderRef) == D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
				shaderRef.shaderIdentifier = stateObjectProperties->GetShaderIdentifier(exportName);
				shaderRef.shaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
				shaderRef.localRootArguments = data;
				shaderRef.localRootArgumentsSize = dataSz;

				void* mappedData;
				CD3DX12_RANGE readRange(0, 0);
				AssertHr(table->Map(0, &readRange, &mappedData));
				uint8_t* byteDest = static_cast<uint8_t*>(mappedData);
				memcpy(byteDest, shaderRef.shaderIdentifier, shaderRef.shaderIdentifierSize);
				if (dataSz)
					memcpy(byteDest + shaderRef.shaderIdentifierSize, shaderRef.localRootArguments, shaderRef.localRootArgumentsSize);

				return table;
			};

			rayGenShaderTable = CreateShaderTable(L"rgMain", &rayGenRS->descTable.gpu, sizeof(rayGenRS->descTable.gpu));
			missShaderTable = CreateShaderTable(L"msMain", nullptr, 0);
			hitGroupShaderTable = CreateShaderTable(L"HitGroup", nullptr, 0);
		}
	}

	// build raytrace AS
	{
		// blas
		renderDevice->BeginRender(swapChain);
		auto renderContext = renderDevice->BeginRenderContext();
		PD3D12GraphicsCommandList4 commandList4;
		AssertHr(renderContext->commandList->QueryInterface(IID_PPV_ARGS(&commandList4.Get())));
		
		vector<D3D12_RAYTRACING_GEOMETRY_DESC> geomDescs;
		D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc{};
		geometryDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
		geometryDesc.Triangles.IndexBuffer = indexBuffer->resource[0].resource->GetGPUVirtualAddress();
		geometryDesc.Triangles.IndexCount = indices.size();
		geometryDesc.Triangles.IndexFormat = DXGI_FORMAT_R32_UINT;
		geometryDesc.Triangles.Transform3x4 = 0;
		geometryDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		geometryDesc.Triangles.VertexCount = vertices.size();
		geometryDesc.Triangles.VertexBuffer.StartAddress = vertexBuffer->resource[0].resource->GetGPUVirtualAddress();
		geometryDesc.Triangles.VertexBuffer.StrideInBytes = sizeof(Vertex);
		geometryDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		D3D12_GPU_VIRTUAL_ADDRESS indexAddr = indexBuffer->resource[0].resource->GetGPUVirtualAddress();
		for (const auto& batch : solidBatches)
		{
			geometryDesc.Triangles.IndexBuffer = indexAddr + batch.first * 4;
			geometryDesc.Triangles.IndexCount = batch.count;
			geomDescs.push_back(geometryDesc);
		}

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS  bottomLevelInputs{};
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO bottomLevelPrebuildInfo{};
		bottomLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
		bottomLevelInputs.NumDescs = geomDescs.size();
		bottomLevelInputs.pGeometryDescs = geomDescs.data();
		device5->GetRaytracingAccelerationStructurePrebuildInfo(&bottomLevelInputs, &bottomLevelPrebuildInfo);
		Assert(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

		AssertHr(device5->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(bottomLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			IID_PPV_ARGS(&bottomLevelAccelerationStructure.Get())));
		bottomLevelAccelerationStructure->SetName(L"BLAS");
		
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bottomLevelBuildDesc{};
		bottomLevelBuildDesc.Inputs = bottomLevelInputs;
		bottomLevelBuildDesc.ScratchAccelerationStructureData = renderDevice->GetUavScratch()->Alloc(
			bottomLevelPrebuildInfo.ScratchDataSizeInBytes);
		bottomLevelBuildDesc.DestAccelerationStructureData = bottomLevelAccelerationStructure->GetGPUVirtualAddress();

		renderContext->AddResourceBarrier(vertexBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		renderContext->AddResourceBarrier(indexBuffer, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		renderContext->FlushResourceBarriers();
		commandList4->BuildRaytracingAccelerationStructure(&bottomLevelBuildDesc, 0, nullptr);

		// tlas
		D3D12_RAYTRACING_INSTANCE_DESC instanceDesc{};
		instanceDesc.Transform[0][0] = instanceDesc.Transform[1][1] = instanceDesc.Transform[2][2] = 1;
		instanceDesc.InstanceMask = 1;
		instanceDesc.AccelerationStructure = bottomLevelAccelerationStructure->GetGPUVirtualAddress();
		instanceDesc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;

		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS tlasBuildFlags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS topLevelInputs{};
		D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO topLevelPrebuildInfo{};
		topLevelInputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
		topLevelInputs.Flags = tlasBuildFlags;
		topLevelInputs.NumDescs = 1;
		topLevelInputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
		device5->GetRaytracingAccelerationStructurePrebuildInfo(&topLevelInputs, &topLevelPrebuildInfo);
		Assert(topLevelPrebuildInfo.ResultDataMaxSizeInBytes > 0);

		AssertHr(device5->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(topLevelPrebuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
			D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
			nullptr,
			IID_PPV_ARGS(&topLevelAccelerationStructure.Get())));
		topLevelAccelerationStructure->SetName(L"TLAS");

		D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC topLevelBuildDesc{};
		topLevelInputs.InstanceDescs = renderDevice->GetUploadScratch()->Alloc(
			sizeof(instanceDesc), &instanceDesc);
		topLevelBuildDesc.Inputs = topLevelInputs;
		topLevelBuildDesc.ScratchAccelerationStructureData = renderDevice->GetUavScratch()->Alloc(
			topLevelPrebuildInfo.ScratchDataSizeInBytes);
		topLevelBuildDesc.DestAccelerationStructureData = topLevelAccelerationStructure->GetGPUVirtualAddress();

		commandList4->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(bottomLevelAccelerationStructure));
		commandList4->BuildRaytracingAccelerationStructure(&topLevelBuildDesc, 0, nullptr);
	}

	//

	OrbitalCamera camera(canvas);
	camera.SetPerspective(1.4f, 1.0f / 64.0f, 10000.0f);
	if (!hasCamera)
		camera.SetCamera(Vector3f(zero), Quaternionf(zero), Distance(bBox.min, bBox.max));
	else
		camera.SetCamera(camTarget, camRot, camDist);

	TimeCounter counter;
	uint frameIdx = 0;

	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		// Update

		Time deltaTime = counter.GetElapsed();
		camera.Update(deltaTime);
		frameIdx++;

		Matrix44f projTM = camera.PerspectiveMatrix();
		Matrix44f viewTM = camera.ViewMatrix();
		Matrix44f projViewTM = Mul(viewTM, projTM);
		viewConsts.projViewTM = Mul(viewTM, projTM);
		viewConsts.invProjViewTM = Inverse(viewConsts.projViewTM);
		viewConsts.viewportSize = Vector2f(canvasDesc.width, canvasDesc.height);
		viewConsts.invViewportSize = Vector2f(1.0f / canvasDesc.width, 1.0f / canvasDesc.height);
		viewConsts.viewPosition = camera.Position();
		viewConsts.frameIdx = frameIdx % 1024;

		const bool resetTemporal = GetAsyncKeyState(VK_LBUTTON) & 0x8000;

		// Render

		GPUBufferPtr frameBuffer = swapChain->GetBackBuffer();

		renderDevice->BeginRender(swapChain);
		auto renderContext = renderDevice->BeginRenderContext();
		Pointer<ID3D12GraphicsCommandList> commandList = renderContext->commandList;
		PD3D12GraphicsCommandList4 commandList4;
		AssertHr(renderContext->commandList->QueryInterface(IID_PPV_ARGS(&commandList4.Get())));

		// Populate command list
		{
			renderContext->rootSignature = nullptr;
			renderContext->pipelineState = nullptr;
			renderContext->indexBuffer = nullptr;
			renderContext->vertexBuffers.fill(nullptr);
			renderContext->topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

			// update viewCB
			renderContext->CopyResource(viewCB, viewConsts);

			// set and clear render target
			renderContext->ClearRenderTarget(finalBuffer, Color(0.2f, 0.5f, 1.0f));
			renderContext->ClearDepthStencilTarget(depthBuffer);
			renderContext->BeginRenderPass(renderPass);

			// draw
			commandList->SetGraphicsRootSignature(drawBatches[0].pso->rootSignature);
			commandList->SetPipelineState(drawBatches[0].pso->pipelineState);
			renderContext->SetResourceSet(0, viewRS);
			commandList->SetGraphicsRootDescriptorTable(2, sampler.gpu);
						
			// for (const auto& batch : drawBatches)
			for (uint i = 0; i < solidBatches.size(); ++i)
			{
				renderContext->SetResourceSet(1, batchesRSs[solidBatches[i].textureId]);
				renderContext->Draw(drawBatches[i]);
			}
			
			renderContext->EndRenderPass();

			// do raytracing
			{
				uint clearTemporal = 0;
				if (resetTemporal)
				{
					renderContext->ClearRenderTarget(temporalOut, Color(0.0f, 0.0f, 0.0f));
					renderContext->ClearRenderTarget(momentsBuffer, Color(0.0f, 0.0f, 0.0f));
					clearTemporal = 1;
				}
				renderContext->CopyResource(temporalCB, clearTemporal);

				renderContext->AddResourceBarrier(dxrOutput, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				renderContext->FlushResourceBarriers();
				
				commandList4->SetPipelineState1(stateObject);
				commandList4->SetComputeRootSignature(raytracingGlobalRootSignature);
				renderContext->rootSignature = raytracingGlobalRootSignature;
				commandList4->SetComputeRootDescriptorTable(0, globalRS->descTable.gpu);
				commandList4->SetComputeRootShaderResourceView(1, topLevelAccelerationStructure->GetGPUVirtualAddress());

				D3D12_DISPATCH_RAYS_DESC dispatchDesc{};
				dispatchDesc.RayGenerationShaderRecord.StartAddress = rayGenShaderTable->GetGPUVirtualAddress();
				dispatchDesc.RayGenerationShaderRecord.SizeInBytes = rayGenShaderTable->GetDesc().Width;
				dispatchDesc.MissShaderTable.StartAddress = missShaderTable->GetGPUVirtualAddress();
				dispatchDesc.MissShaderTable.SizeInBytes = missShaderTable->GetDesc().Width;
				dispatchDesc.MissShaderTable.StrideInBytes = dispatchDesc.MissShaderTable.SizeInBytes;
				dispatchDesc.HitGroupTable.StartAddress = hitGroupShaderTable->GetGPUVirtualAddress();
				dispatchDesc.HitGroupTable.SizeInBytes = hitGroupShaderTable->GetDesc().Width;
				dispatchDesc.HitGroupTable.StrideInBytes = dispatchDesc.HitGroupTable.SizeInBytes;
				dispatchDesc.Width = width;
				dispatchDesc.Height = height;
				dispatchDesc.Depth = 1;
				commandList4->DispatchRays(&dispatchDesc);
			}

			{
				renderContext->ComputeDispatch(svgfTemporalPass, DivUp(width, 8), DivUp(height, 8), 1);

				renderContext->ComputeDispatch(svgfAtrousPasses[0], DivUp(width, 8), DivUp(height, 8), 1);

				renderContext->CopyResource(temporalOut, temporalCpySrc);
				
				for (uint i = 1; i < svgfAtrousPasses.size(); ++i)
					renderContext->ComputeDispatch(svgfAtrousPasses[i], DivUp(width, 8), DivUp(height, 8), 1);
			}

			renderContext->ComputeDispatch(finalizePass, DivUp(width, 64), height, 1);

			renderContext->CopyResource(frameBuffer, finalBuffer);
		}

		renderDevice->Present();
	}
}
