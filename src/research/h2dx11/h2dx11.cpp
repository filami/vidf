#include "pch.h"

#if 0

#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"
#include "vidf/rendererdx11/wikidraw.h"
#include "vidf/rendererdx11/gputimer.h"
#include "vidf/proto/mesh.h"
#include "vidf/common/random.h"

#include "renderpasses/averagebrightness.h"
#include "renderpasses/utilitypasses.h"

#include "stream.h"
#include "pak.h"
#include "bsptexture.h"
#include "bih.h"
#include "kdtreeaccel.h"
#include "renderpass.h"


using namespace vidf;
using namespace dx11;
using namespace proto;


class Dx11CanvasListener : public CanvasListener
{
public:
	Dx11CanvasListener(ShaderManager& _shaderManager)
		: shaderManager(_shaderManager) {}

	virtual void Close()
	{
		PostQuitMessage();
	}
	virtual void KeyDown(KeyCode keyCode)
	{
		if (keyCode == KeyCode::Escape)
			PostQuitMessage();
		else if (keyCode == KeyCode::F5)
			shaderManager.RecompileShaders();
	}
	ShaderManager& shaderManager;
};



namespace h2
{


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
		BspSurf_LIGHT	= 0x01,	
		BspSurf_SLICK	= 0x02,
		BspSurf_SKY	    = 0x04,
		BspSurf_WARP	= 0x08,
		BspSurf_TRANS33 = 0x10,
		BspSurf_TRANS66 = 0x20,
		BspSurf_FLOWING = 0x40,
		BspSurf_NODRAW	= 0x80,
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
		std::vector<Vector3f>   vertices;
		std::vector<BspEdge>    edges;
		std::vector<BspNode>    nodes;
		std::vector<int32>      faceEdges;
		std::vector<BspFace>    faces;
		std::vector<BspPlane>   planes;
		std::vector<BspTexInfo> texInfo;

		std::vector<uint32>     texInfoToTexture;
		std::vector<BspTexture> textures;
		std::map<std::string, uint> textureMap;
		std::vector<BspEntity>  entities;
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


	//////////////////////////////////////////////////////////////////////////

	/*
	// const uint cascadeShadowSize = 1024 * 4;
	// const float cascadeShadowLength = 2048.0f * 2 * bspUnitsToMeters;
	// const float cascadeShadowDepth = 1024.0f * 2.0f * bspUnitsToMeters;
	const uint cascadeShadowSize = 1024;
//	const float cascadeShadowLength = 512.0f * bspUnitsToMeters;
//	const float cascadeShadowDepth = 256.0f * bspUnitsToMeters;
	const float cascadeShadowLength = 256.0f * bspUnitsToMeters;
	const float cascadeShadowDepth = 1024.0f * bspUnitsToMeters;
	const DXGI_FORMAT cascadeShadowFormat = DXGI_FORMAT_R16_UNORM;
	const uint numShadowCascades = 3;
	const float cascadeMultiplier = 3.0f;
	*/

	const uint cascadeShadowSize = 2048;
	const float cascadeShadowLength = 128.0f * bspUnitsToMeters;
	const float cascadeShadowDepth = 256.0f * bspUnitsToMeters;
	const DXGI_FORMAT cascadeShadowFormat = DXGI_FORMAT_R16_UNORM;
	const uint numShadowCascades = 5;
	const float cascadeMultiplier = 3.0f;

	struct CascadeShadowConsts
	{
		Matrix44f tm;
		Vector3f lightDir;
		float _;
		Vector2f texelSize;
		Vector2f __;
	};



	Matrix44f MakeCascadeProjTM(Vector3f viewPosition, Vector3f lightDir, int level)
	{
		const float multiplier = std::pow(cascadeMultiplier, level);

		const Vector3f up = Vector3f(0, 1, 0);

		Vector3f pos = viewPosition + lightDir * (cascadeShadowDepth * multiplier) * 0.5f;
		Vector3f zAxis = up;
		Vector3f xAxis = Vector3f(1.0f, 0.0f, 0.0f);
		Vector3f yAxis = Vector3f(0.0f, 1.0f, 0.0f);
		if (std::abs(Dot(lightDir, up)) < 0.99f)
		{
			zAxis = Normalize(-lightDir);
			xAxis = Normalize(Cross(up, zAxis));
			yAxis = Cross(zAxis, xAxis);
		}

		Matrix44f tm0;
		tm0.m00 = xAxis.x; tm0.m01 = yAxis.x; tm0.m02 = zAxis.x; tm0.m03 = 0;
		tm0.m10 = xAxis.y; tm0.m11 = yAxis.y; tm0.m12 = zAxis.y; tm0.m13 = 0;
		tm0.m20 = xAxis.z; tm0.m21 = yAxis.z; tm0.m22 = zAxis.z; tm0.m23 = 0;
		tm0.m30 = -Dot(xAxis, pos); tm0.m31 = -Dot(yAxis, pos); tm0.m32 = -Dot(zAxis, pos); tm0.m33 = 1.0f;

		Matrix44f tm1(zero);
		tm1.m00 = 1.0f / (cascadeShadowLength * multiplier) * 0.5f;
		tm1.m11 = 1.0f / (cascadeShadowLength * multiplier) * 0.5f;
		tm1.m22 = 1.0f / (cascadeShadowDepth * multiplier);
		tm1.m33 = 1.0f;

		Matrix44f tm = Mul(tm0, tm1);

		return tm;
	}



	//////////////////////////////////////////////////////////////////////////


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
		int32    children[2] = {0, 0};	// negative: child is a leaf, otherwise, child is index-1
	};


	struct LightShade
	{
		Vector3f position;
		Vector3f normal;
		float radius;
		float  brightness;
		uint32 color;
	};



	struct Surfel
	{
		Vector3f position;
		Vector3f normal;
		float skyVisibility;
	};


	struct SurfelConnector
	{
		Vector3f point;
		uint surfelId;
	};


	class LightManager
	{
	public:
		void BuildFromBSP(const std::vector<BspEntity>& entities);
		void BuildFromSurfels(const std::vector<Surfel>& surfels);
		void CreateBuffers(RenderDevicePtr renderDevice);
		StructuredBuffer GetCullsBuffer() const { return lightCullsBuffer; }
		StructuredBuffer GetShadesBuffer() const { return lightShadesBuffer; }
		StructuredBuffer GetKdNodesBuffer() const { return lightKdNodesBuffer; }
		StructuredBuffer GetKdLeafsBuffer() const { return lightKdLeafsBuffer; }
		StructuredBuffer GetLightIdsBuffer() const { return lightIdsBuffer; }
		uint GetNumLightCulls() const { return lightCulls.size(); }

	private:
		void InsertLight(Vector3f position, float radius, LightShade shade);
		void BuildKdTree();
		void BuildKdNode(uint nodeIdx, Boxf bounds, const std::vector<uint>& shadeIds, uint depth, bool forceLeaf = false);
		void MakeNode(uint nodeIdx, float plane, uint axis, uint aboveChild);
		void MakeLeaf(uint nodeIdx, const std::vector<uint>& shadeIds);

	private:
		std::vector<LightKdNode> kdNodes;
		std::vector<LightKdLeaf> kdLeafs;
		std::vector<uint>        lightIds;
		std::vector<LightCull>   lightCulls;
		std::vector<LightShade>  lightShades;
		StructuredBuffer         lightCullsBuffer;
		StructuredBuffer         lightShadesBuffer;
		StructuredBuffer         lightKdNodesBuffer;
		StructuredBuffer         lightKdLeafsBuffer;
		StructuredBuffer         lightIdsBuffer;
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



	void LightManager::BuildFromSurfels(const std::vector<Surfel>& surfels)
	{
		lightCulls.clear();
		lightShades.clear();
		for (const auto& surfel : surfels)
		{
			LightCull lightCull;
			LightShade lightShade;

			lightCull.position = surfel.position;
			lightShade.position = surfel.position;

			lightShade.color = ~0;

			lightShade.brightness = surfel.skyVisibility;
			lightCull.cullRadiusSqr = 0.2f;
			lightShade.radius = 0.5f;
			lightShade.normal = surfel.normal;
			lightShades.push_back(lightShade);
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
	//	if (testBox.max.x < box.min.x) return false;
	//	if (testBox.max.y < box.min.y) return false;
	//	if (testBox.max.z < box.min.z) return false;
	//	if (testBox.min.x > box.max.x) return false;
	//	if (testBox.min.y > box.max.y) return false;
	//	if (testBox.min.z > box.max.z) return false;

		if (testBox.min.x >= box.min.x && testBox.max.x <= box.max.x &&
			testBox.min.y >= box.min.y && testBox.max.y <= box.max.y &&
			testBox.min.z >= box.min.z && testBox.max.z <= box.max.z)
			return true;
		return false;
	}


	bool Intersects(Boxf box, Boxf testBox)
	{
	//	if (testBox.max.x >= box.min.x) return false;
	//	if (testBox.max.y >= box.min.y) return false;
	//	if (testBox.max.z >= box.min.z) return false;
	//	if (testBox.min.x > box.max.x) return false;
	//	if (testBox.min.y > box.max.y) return false;
	//	if (testBox.min.z > box.max.z) return false;

		if (testBox.max.x < box.min.x) return false;
		if (testBox.max.y < box.min.y) return false;
		if (testBox.max.z < box.min.z) return false;
		if (testBox.min.x > box.max.x) return false;
		if (testBox.min.y > box.max.y) return false;
		if (testBox.min.z > box.max.z) return false;

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


	void LightManager::MakeLeaf(uint nodeIdx, const std::vector<uint>& shadeIds)
	{
		kdNodes[nodeIdx].plane = 0;
		kdNodes[nodeIdx].data = kdLeafs.size() << 2;

		LightKdLeaf leaf;
		leaf.firstLightIdx = lightIds.size();
		leaf.numLightIdx = shadeIds.size();
		lightIds.insert(lightIds.end(), shadeIds.begin(), shadeIds.end());
		kdLeafs.push_back(leaf);
	}



	void LightManager::BuildKdNode(uint nodeIdx, Boxf bounds, const std::vector<uint>& shadeIds, uint depth, bool forceLeaf)
	{
		kdNodes.emplace_back();

	//	const uint maxDepth = 10;
		const uint maxDepth = 20;
		const uint maxShades = 1;
		if (forceLeaf || shadeIds.size() <= maxShades || depth >= maxDepth)
		{
			MakeLeaf(nodeIdx, shadeIds);
			return;
		}
				
		int axis = MaximumExtent(bounds);
		Vector3f d = bounds.max - bounds.min;
		float bestOffset = bounds.min[axis] + d[axis] * 0.5f;

		std::vector<uint> leftPartitionShadeIds;
		std::vector<uint> rightPartitionShadeIds;
		leftPartitionShadeIds.reserve(shadeIds.size());
		rightPartitionShadeIds.reserve(shadeIds.size());
		float splitLeft = bounds.min[axis];
		float splitRight = bounds.max[axis];
		for (uint i = 0; i < shadeIds.size(); ++i)
		{
			uint shadeId = shadeIds[i];
			Boxf shadeBounds = SphereToBox(lightShades[shadeId].position, lightShades[shadeId].radius);
			if (shadeBounds.min[axis] < bestOffset)
			{
				leftPartitionShadeIds.push_back(shadeId);
				splitLeft = Max(splitLeft, shadeBounds.max[axis]);
			}
		}
		for (uint i = 0; i < shadeIds.size(); ++i)
		{
			uint shadeId = shadeIds[i];
			Boxf shadeBounds = SphereToBox(lightShades[shadeId].position, lightShades[shadeId].radius);
			if (shadeBounds.max[axis] >= bestOffset)
			{
				rightPartitionShadeIds.push_back(shadeId);
				splitRight = Min(splitRight, shadeBounds.min[axis]);
			}
		}
		if (leftPartitionShadeIds.size() == rightPartitionShadeIds.size() == shadeIds.size())
			__debugbreak();
		Boxf leftBounds = bounds;
		Boxf rightBounds = bounds;
		leftBounds.max[axis] = splitLeft;
		rightBounds.min[axis] = splitRight;

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
	//	StructuredBufferDesc lightCullsDesc(
	//		sizeof(LightCull),
	//		lightCulls.size(),
	//		"lightCulls");
	//	lightCullsDesc.dataPtr = lightCulls.data();
	//	lightCullsDesc.dataSize = lightCullsDesc.count * lightCullsDesc.stride;
	//	lightCullsBuffer = StructuredBuffer::Create(renderDevice, lightCullsDesc);

		StructuredBufferDesc lightShadesDesc(
			sizeof(LightShade),
			lightShades.size(),
			"lightShades");
		lightShadesDesc.dataPtr = lightShades.data();
		lightShadesDesc.dataSize = lightShadesDesc.count * lightShadesDesc.stride;
		lightShadesBuffer = StructuredBuffer::Create(renderDevice, lightShadesDesc);

		StructuredBufferDesc kdNodesDesc(
			sizeof(LightKdNode),
			kdNodes.size(),
			"lightKdNodes");
		kdNodesDesc.dataPtr = kdNodes.data();
		kdNodesDesc.dataSize = kdNodesDesc.count * kdNodesDesc.stride;
		lightKdNodesBuffer = StructuredBuffer::Create(renderDevice, kdNodesDesc);

		StructuredBufferDesc kdLeafsDesc(
			sizeof(LightKdLeaf),
			kdLeafs.size(),
			"lightkdLeafs");
		kdLeafsDesc.dataPtr = kdLeafs.data();
		kdLeafsDesc.dataSize = kdLeafsDesc.count * kdLeafsDesc.stride;
		lightKdLeafsBuffer = StructuredBuffer::Create(renderDevice, kdLeafsDesc);

		StructuredBufferDesc lightIdsDesc(
			sizeof(uint),
			lightIds.size(),
			"lightIds");
		lightIdsDesc.dataPtr = lightIds.data();
		lightIdsDesc.dataSize = lightIdsDesc.count * lightIdsDesc.stride;
		lightIdsBuffer = StructuredBuffer::Create(renderDevice, lightIdsDesc);
	}



	//////////////////////////////////////////////////////////////////////////


	template <int base>
	static float RadicalInverseSpecialized(uint64_t a)
	{
		const float OneMinusEpsilon = 1.0f - std::numeric_limits<float>::epsilon();
		const float invBase = (float)1 / (float)base;
		uint64_t reversedDigits = 0;
		float invBaseN = 1;
		while (a) {
			uint64_t next = a / base;
			uint64_t digit = a - next * base;
			reversedDigits = reversedDigits * base + digit;
			invBaseN *= invBase;
			a = next;
		}
		return std::min(reversedDigits * invBaseN, OneMinusEpsilon);
	}


	Vector3f CosineHemisphere(Vector3f normal, Vector2f sample)
	{
		Vector3f xx = Cross(normal, Vector3f(0.0f, 0.0f, 1.0f));
		if (std::abs(Dot(xx, xx)) < 0.99f)
			xx = Vector3f(0.0f, 1.0f, 0.0f);
		else
			xx = Normalize(xx);
		Vector3f yy = Normalize(Cross(xx, normal));
		float u = sample.x * PI * 2.0f;
		float r = std::sqrt(sample.y * 0.5f + 0.5f);
		Vector3f d;
		d.x = std::cos(u) * r;
		d.y = std::sin(u) * r;
		d.z = std::sqrt(1.0f - d.x * d.x - d.y * d.y);
		return d.x * xx + d.y * yy + d.z * normal;
	}


}


void __H2Dx11()
{
	using namespace h2;

	FileManager fileManager;
	fileManager.AddPak("data/h2dx11/Htic2-0.pak");
	fileManager.AddPak("data/h2dx11/Htic2-1.pak");

	FileManager::PakFileHandle map = fileManager.OpenFile("maps/ssdocks.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/sstown.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/sspalace.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/andplaza.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/andslums.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/hive1.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/kellcaves.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/canyon.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/oglemine1.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/oglemine2.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/tutorial.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/tutorial2.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/dmandoria.bsp");
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

	const uint width = 1280;
	const uint height = 720;

	RenderDeviceDesc renderDeviceDesc{};
	renderDeviceDesc.enableValidation = true;
	RenderDevicePtr renderDevice = RenderDevice::Create(renderDeviceDesc);
	if (!renderDevice)
		return;
	ShaderManager shaderManager(renderDevice);
	CommandBuffer commandBuffer(renderDevice);
	GpuTimer gpuTimer{renderDevice};
	WikiDraw wikiDraw{renderDevice, &shaderManager};
	WikiText wikiText{renderDevice};

	Dx11CanvasListener canvasListener{shaderManager};

	CanvasDesc canvasDesc{};
	canvasDesc.width = width;
	canvasDesc.height = height;
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	canvas->AddListener(&canvasListener);
	if (!canvas)
		return;

	SwapChainDesc swapChainDesc{};
	swapChainDesc.width = canvasDesc.width;
	swapChainDesc.height = canvasDesc.height;
	swapChainDesc.windowHandle = canvas->GetHandle();
	SwapChainPtr swapChain = renderDevice->CreateSwapChain(swapChainDesc);
	if (!swapChain)
		return;

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
	std::vector<Vertex> vertices;
	std::vector<Batch> solidBatches;
	std::vector<Batch> oitBatches;
	for (uint i = 0; i < batches.size(); ++i)
	{
		Batch& batch = batches[i];
		batch.first = vertices.size();
		batch.count = batch.vertices.size();
		batch.textureId = i;
		vertices.insert(vertices.end(), batch.vertices.begin(), batch.vertices.end());
		batch.vertices.clear();

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
	VertexBuffer vertexBuffer = VertexBuffer::Create(
		renderDevice,
		VertexBufferDesc(vertices.data(), vertices.size(), "vertexBuffer"));

	for (auto& texture : bspData.textures)
	{
		FileManager::PakFileHandle m8 = fileManager.OpenFile(texture.name.c_str());
		if (m8)
			texture.gpuTexture = LoadTextureM8(*m8, renderDevice, texture.name.c_str());
	}

	//////////////////////////////////////////////////////////////////////////
	/*
	std::vector<Surfel> surfels;
	std::vector<SurfelConnector> surfelConns;

	std::vector<pbrt::Primitive> kdPrimitives;

	vidf::Rand48 rand48;
	vidf::UniformReal<float> unorm(0.0f, 1.0f);
	vidf::UniformReal<float> snorm(-1.0f, 1.0f);

	uint64_t sample = 0;
	for (uint v = 0; v < vertices.size(); v += 3)
	{
		const Vertex v0 = vertices[v];
		const Vertex v1 = vertices[v + 1];
		const Vertex v2 = vertices[v + 2];
		Surfel surfel;
		surfel.skyVisibility = 0.0f;
		surfel.normal = Normalize(v0.normal + v1.normal + v2.normal);

		const float density = 8.0f;
		const Vector3f e0 = v1.position - v0.position;
		const Vector3f e1 = v2.position - v0.position;
		const Vector3f e2 = v2.position - v1.position;
		const float area = Length(Cross(e0, e1)) * 0.5f;
		uint numSamples = uint(area * density * density);
		numSamples = Max(1u, uint(Length(e0) * density));
		numSamples = Max(1u, uint(Length(e1) * density));
		numSamples = Max(1u, uint(Length(e2) * density));

		for (uint s = 0; s < numSamples; ++s)
		{
		//	float u = unorm(rand48);
		//	float v = unorm(rand48);
			float u = RadicalInverseSpecialized<2>(sample);
			float v = RadicalInverseSpecialized<3>(sample);
			sample++;
			if (v > 1.0f - u)
			{
				u = 1.0f - u;
				v = 1.0f - v;
			}
			surfel.position = v0.position + e0 * u + e1 * v;
			surfels.push_back(surfel);
		}

		pbrt::Primitive primitive;
		primitive.vertices[0] = vertices[v + 0].position;
		primitive.vertices[1] = vertices[v + 1].position;
		primitive.vertices[2] = vertices[v + 2].position;
		kdPrimitives.push_back(primitive);
	}

	std::cout << "Building KdTree . . . ";
	pbrt::KdTreeAccel kdTree(kdPrimitives);
	std::cout << "DONE" << std::endl;

	std::cout << "Raytracing";
	for (uint i = 0; i < surfels.size()-1; ++i)
	{
		const uint numSamples = 64;
		for (uint j = 0; j < numSamples; ++j)
		{
		//	float u = snorm(rand48) * PI;
		//	float v = snorm(rand48) * PI * 0.5f;
		//	Vector3f dir;
		//	dir.x = std::cos(u) * std::cos(v);
		//	dir.y = std::sin(v);
		//	dir.z = std::sin(u) * std::cos(v);
		//	if (Dot(dir, surfels[i].normal) < 0.0f)
		//		dir = -dir;

		//	float u = snorm(rand48);
		//	float v = snorm(rand48);
			float u = RadicalInverseSpecialized<2>(sample) * 2.0f - 1.0f;
			float v = RadicalInverseSpecialized<3>(sample) * 2.0f - 1.0f;
			sample++;
			Vector3f dir = CosineHemisphere(surfels[i].normal, Vector2f(u, v));

			Rayf ray{ zero };
			ray.origin = surfels[i].position;
			ray.direction = dir;
			ray.origin = ray.origin + ray.direction * 0.01f;
			pbrt::SurfaceInteraction isect;
			const bool isected = kdTree.Intersect(ray, &isect);
			if (isected)
			{
				SurfelConnector conn;
				conn.surfelId = i;
				conn.point = isect.point;
				surfelConns.push_back(conn);
			}
			else
			{
				surfels[i].skyVisibility += 1.0f / numSamples;
			}
		}
		if (i % (surfels.size() / 80) == 0)
			std::cout << ".";
	}
	std::cout << "DONE" << std::endl;
	std::cout << "Prims   : " << vertices.size() / 3 << std::endl;
	std::cout << "Surfels : " << surfels.size() << std::endl;
	std::cout << "Conns   : " << surfelConns.size() << std::endl;
	*/
	//////////////////////////////////////////////////////////////////////////
	/*
	BIH bih;
	std::vector<BIH::Triangle> bihTriangles;
	for (uint i = 0; i < vertices.size(); i += 3)
	{
		BIH::Triangle bihTriangles;
		bihTriangles.vertices[0] = vertices[i + 0].position;
		bihTriangles.vertices[1] = vertices[i + 1].position;
		bihTriangles.vertices[1] = vertices[i + 2].position;
	}
	bih.Build(bihTriangles);
	*/
	//////////////////////////////////////////////////////////////////////////

	D3D11_SAMPLER_DESC samplerDesc{};
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.MinLOD = -std::numeric_limits<float>::max();
	samplerDesc.MaxLOD = std::numeric_limits<float>::max();
	PD3D11SamplerState diffuseSS;
	renderDevice->GetDevice()->CreateSamplerState(&samplerDesc, &diffuseSS.Get());

	ShaderPtr vertexShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "vsMain", ShaderType::VertexShader);
	ShaderPtr pixelShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "psMain", ShaderType::PixelShader);
	ShaderPtr pixelShaderOIT = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "psMainOIT", ShaderType::PixelShader);
	ShaderPtr oitClearPS = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "psOITClear", ShaderType::PixelShader);
	ShaderPtr finalVertexShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "vsFinalMain", ShaderType::VertexShader);
	ShaderPtr finalPixelShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "psFinalMain", ShaderType::PixelShader);

	D3D11_INPUT_ELEMENT_DESC elements[3]{};
	elements[0].SemanticName = "POSITION";
	elements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elements[0].AlignedByteOffset = offsetof(Vertex, position);
	elements[1].SemanticName = "NORMAL";
	elements[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	elements[1].AlignedByteOffset = offsetof(Vertex, normal);
	elements[2].SemanticName = "TEXCOORD";
	elements[2].Format = DXGI_FORMAT_R32G32_FLOAT;
	elements[2].AlignedByteOffset = offsetof(Vertex, texCoord);

//	RenderTargetDesc solidRTDesc{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, uint(canvasDesc.width), uint(canvasDesc.height), "diffuseRT"};
//	RenderTarget solidRT = RenderTarget::Create(renderDevice, solidRTDesc);
	GPUBufferDesc solidRTDesc;
	solidRTDesc.type = GPUBufferType::Texture2D;
	solidRTDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	solidRTDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_RenderTarget;
	solidRTDesc.width = canvasDesc.width;
	solidRTDesc.height = canvasDesc.height;
	solidRTDesc.name = "diffuseRT";
	GPUBuffer solidRT = GPUBuffer::Create(renderDevice, solidRTDesc);

	DepthStencilDesc depthDSVDesc{DXGI_FORMAT_R32_FLOAT, uint(canvasDesc.width), uint(canvasDesc.height), "depthDSV" };
	DepthStencil depthDSV = DepthStencil::Create(renderDevice, depthDSVDesc);

	struct OIT
	{
		Vector4f fragments[8];
		float    depth[8];
		uint     numFrags;
	};
	RWStructuredBufferDesc oitFragmentRT(sizeof(OIT), canvasDesc.width * canvasDesc.height, "oitRT");
	RWStructuredBuffer oitRT = RWStructuredBuffer::Create(renderDevice, oitFragmentRT);

	GraphicsPSODesc oitClearPSODesc;
	oitClearPSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
	oitClearPSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	oitClearPSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	oitClearPSODesc.vertexShader = finalVertexShader;
	oitClearPSODesc.pixelShader = oitClearPS;
	GraphicsPSOPtr oitClearPSO = GraphicsPSO::Create(renderDevice, oitClearPSODesc);

	GraphicsPSODesc PSODesc;
	PSODesc.geometryDesc = elements;
	PSODesc.numGeomDesc = ARRAYSIZE(elements);
	PSODesc.rasterizer.CullMode = D3D11_CULL_BACK;
	PSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	PSODesc.rasterizer.FrontCounterClockwise = false;
	PSODesc.depthStencil.DepthEnable = true;
	PSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_EQUAL;
	PSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	PSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PSODesc.vertexShader = vertexShader;
	PSODesc.pixelShader = pixelShader;
	GraphicsPSOPtr solidPSO = GraphicsPSO::Create(renderDevice, PSODesc);

	GraphicsPSODesc zOnlyPSODesc = PSODesc;
	zOnlyPSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	zOnlyPSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	zOnlyPSODesc.blend.RenderTarget[0].RenderTargetWriteMask = 0;
	zOnlyPSODesc.pixelShader.reset();
	GraphicsPSOPtr zOnlyPSO = GraphicsPSO::Create(renderDevice, zOnlyPSODesc);
		
	GraphicsPSODesc oitPSODesc = PSODesc;
	oitPSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	oitPSODesc.pixelShader = pixelShaderOIT;
	oitPSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	GraphicsPSOPtr oitPSO = GraphicsPSO::Create(renderDevice, oitPSODesc);

	GraphicsPSODesc finalPSODesc;
	finalPSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
	finalPSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	finalPSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	finalPSODesc.vertexShader = finalVertexShader;
	finalPSODesc.pixelShader = finalPixelShader;
	GraphicsPSOPtr finalPSO = GraphicsPSO::Create(renderDevice, finalPSODesc);

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = canvasDesc.width;
	viewport.Height = canvasDesc.height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	RenderPassDesc solidPassDesc;
	solidPassDesc.viewport = viewport;
	solidPassDesc.rtvs[0] = solidRT.rtv;
	solidPassDesc.dsv = depthDSV.dsv;
	RenderPassPtr solidPass = RenderPass::Create(renderDevice, solidPassDesc);

	RenderPassDesc oitPassDesc;
	oitPassDesc.viewport = viewport;
	oitPassDesc.uavs[0] = oitRT.uav;
	oitPassDesc.dsv = depthDSV.dsv;
	RenderPassPtr oitPass = RenderPass::Create(renderDevice, oitPassDesc);

	RenderPassDesc finalizePassDesc;
	finalizePassDesc.viewport = viewport;
	finalizePassDesc.rtvs[0] = swapChain->GetBackBufferRTV();
	RenderPassPtr finalizePass = RenderPass::Create(renderDevice, finalizePassDesc);

	OrbitalCamera camera(canvas);
	camera.SetPerspective(1.4f, 1.0f, 10000.0f);
	// camera.SetCamera((bBox.max + bBox.min)*0.5f, Quaternionf(zero), Distance(bBox.min, bBox.max));
	camera.SetCamera(Vector3f(zero), Quaternionf(zero), Distance(bBox.min, bBox.max));

	struct ViewConsts
	{
		Matrix44f projTM;
		Matrix44f viewTM;
		Vector2f viewportSize;
		Vector2f invViewportSize;
		Vector3f viewPosition;
		uint     numLights;
	};
	ConstantBufferDesc viewCBDesc(sizeof(ViewConsts), "viewCB");
	ConstantBuffer viewCB = ConstantBuffer::Create(renderDevice, viewCBDesc);

	//////////////////////////////////////////////////////////////////////////

	ShaderPtr shadowVertexShader = shaderManager.CompileShaderFile("data/shaders/shadows.hlsl", "vsMain", ShaderType::VertexShader);

	D3D11_SAMPLER_DESC shadowSSDesc{};
	shadowSSDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	shadowSSDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	shadowSSDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	shadowSSDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	shadowSSDesc.MinLOD = 0;
	shadowSSDesc.MaxLOD = 0;
	shadowSSDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	PD3D11SamplerState shadowSS;
	renderDevice->GetDevice()->CreateSamplerState(&shadowSSDesc, &shadowSS.Get());

	GraphicsPSODesc shadowPSODesc = PSODesc;
	shadowPSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
	shadowPSODesc.rasterizer.DepthBias = 2;
	shadowPSODesc.rasterizer.SlopeScaledDepthBias = 1.0f;
	shadowPSODesc.rasterizer.DepthBiasClamp = 16.0f;
	shadowPSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	shadowPSODesc.blend.RenderTarget[0].RenderTargetWriteMask = 0;
	shadowPSODesc.vertexShader = shadowVertexShader;
	shadowPSODesc.pixelShader.reset();
	GraphicsPSOPtr shadowPSO = GraphicsPSO::Create(renderDevice, shadowPSODesc);

	D3D11_VIEWPORT cascadeShadowViewport{};
	cascadeShadowViewport.TopLeftX = 0;
	cascadeShadowViewport.TopLeftY = 0;
	cascadeShadowViewport.Width = cascadeShadowSize;
	cascadeShadowViewport.Height = cascadeShadowSize;
	cascadeShadowViewport.MinDepth = 0.0f;
	cascadeShadowViewport.MaxDepth = 1.0f;

	std::vector<ConstantBuffer> cascadeShadowsCBs;
	std::vector<RenderPassPtr> cascadeShadowMapPasses;
	std::vector<Matrix44f> cascadeShadowTMs;

	DepthStencilDesc _cascadeShadowMapDesc{ cascadeShadowFormat, cascadeShadowSize, cascadeShadowSize, "cascadeShadowMap" };
	ConstantBufferDesc cascadeShadowsCBDesc(sizeof(CascadeShadowConsts), "cascadeShadowdCB");
	RenderPassDesc cascadeShadowMapPassDesc;
	cascadeShadowMapPassDesc.viewport = cascadeShadowViewport;

	cascadeShadowTMs.resize(numShadowCascades);
	GPUBufferDesc cascadeShadowTMBufferDesc;
	cascadeShadowTMBufferDesc.type = GPUBufferType::Structured;
	cascadeShadowTMBufferDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_Dynamic;
	cascadeShadowTMBufferDesc.elementStride = sizeof(Matrix44f);
	cascadeShadowTMBufferDesc.elementCount = numShadowCascades;
	cascadeShadowTMBufferDesc.name = "cascadeShadowdTMs";
	GPUBuffer cascadeShadowTMBuffer = GPUBuffer::Create(renderDevice, cascadeShadowTMBufferDesc);

	GPUBufferDesc cascadeShadowMapDesc;
	cascadeShadowMapDesc.type = GPUBufferType::Texture2D;
	cascadeShadowMapDesc.format = cascadeShadowFormat;
	cascadeShadowMapDesc.usageFlags = GPUUsage_ShaderResource | GPUUsage_DepthStencil;
	cascadeShadowMapDesc.width = cascadeShadowSize;
	cascadeShadowMapDesc.height = cascadeShadowSize;
	cascadeShadowMapDesc.arraySize = numShadowCascades;
	cascadeShadowMapDesc.name = "cascadeShadowMap";
	GPUBuffer cascadeShadowMap = GPUBuffer::Create(renderDevice, cascadeShadowMapDesc);
	std::vector<GPUBuffer> cascadeShadowMapLayers;
	cascadeShadowMapLayers.reserve(numShadowCascades);

	for (uint i = 0; i < numShadowCascades; ++i)
	{
		cascadeShadowMapDesc.aliasBuffer = &cascadeShadowMap;
		cascadeShadowMapDesc.usageFlags = GPUUsage_DepthStencil;
		cascadeShadowMapDesc.viewArrayLevel = i;
		cascadeShadowMapLayers.emplace_back(GPUBuffer::Create(renderDevice, cascadeShadowMapDesc));

		cascadeShadowMapPassDesc.dsv = cascadeShadowMapLayers.back().dsv;
		cascadeShadowsCBs.emplace_back(ConstantBuffer::Create(renderDevice, cascadeShadowsCBDesc));

		cascadeShadowMapPasses.emplace_back(RenderPass::Create(renderDevice, cascadeShadowMapPassDesc));
	}

	float lightTheta = 0.25f;
	float lightAlpha = 0.8f;

	//////////////////////////////////////////////////////////////////////////

	LightManager lightManager;
	lightManager.BuildFromBSP(bspData.entities);
	// lightManager.BuildFromSurfels(surfels);
	lightManager.CreateBuffers(renderDevice);

	//////////////////////////////////////////////////////////////////////////

	ReducePass reducePass0;
	ReducePass reducePass1;
	GaussianBlurPass blur0;
	GaussianBlurPass blur1;
	GaussianBlurPass blur2;
	GaussianBlurPass blur3;
	GaussianBlurPass blur4;
		
	blur0.Prepare(renderDevice, shaderManager, solidRT, GaussianBlurPass::Gauss7Samples);
	reducePass0.Prepare(renderDevice, shaderManager, blur0.GetOutput(), ReducePass::HalfResolution);
	blur1.Prepare(renderDevice, shaderManager, reducePass0.GetOutput(), GaussianBlurPass::Gauss7Samples);
	blur2.Prepare(renderDevice, shaderManager, blur1.GetOutput(), GaussianBlurPass::Gauss15Samples);
	reducePass1.Prepare(renderDevice, shaderManager, blur2.GetOutput(), ReducePass::HalfResolution);
	blur3.Prepare(renderDevice, shaderManager, reducePass1.GetOutput(), GaussianBlurPass::Gauss15Samples);
	blur4.Prepare(renderDevice, shaderManager, blur3.GetOutput(), GaussianBlurPass::Gauss15Samples);

	//////////////////////////////////////////////////////////////////////////

	GpuSection gpuFrame{ &gpuTimer };
	GpuSection gpuShadows{ &gpuTimer };
	GpuSection gpuSolid{ &gpuTimer };
	GpuSection gpuOIT{ &gpuTimer };

	float movingFrameTime = 0.0f;
	TimeCounter counter;

	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		Time deltaTime = counter.GetElapsed();
		camera.Update(deltaTime);

		if (GetAsyncKeyState(VK_UP) & 0x8000)
			lightAlpha += deltaTime.AsFloat() * 0.5f;
		if (GetAsyncKeyState(VK_DOWN) & 0x8000)
			lightAlpha -= deltaTime.AsFloat() * 0.5f;
		if (GetAsyncKeyState(VK_LEFT) & 0x8000)
			lightTheta += deltaTime.AsFloat() * 0.5f;
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
			lightTheta -= deltaTime.AsFloat() * 0.5f;
		if (GetAsyncKeyState('R') & 0x8000)
			lightAlpha = PI * 0.5f;
		Vector3f lightDir;
		lightDir.x = std::cos(lightTheta) * std::cos(lightAlpha);
		lightDir.y = std::sin(lightTheta) * std::cos(lightAlpha);
		lightDir.z = std::sin(lightAlpha);

		ViewConsts viewConsts;
		viewConsts.projTM = camera.PerspectiveMatrix();
		viewConsts.viewTM = camera.ViewMatrix();
		viewConsts.viewportSize = Vector2f(canvasDesc.width, canvasDesc.height);
		viewConsts.invViewportSize = Vector2f(1.0f / canvasDesc.width, 1.0f / canvasDesc.height);
		viewConsts.viewPosition = camera.Position();
	//	viewConsts.numLights = lightManager.GetNumLightCulls();
		viewCB.Update(renderDevice->GetContext(), viewConsts);

		for (uint level = 0; level < numShadowCascades; ++level)
		{
			const float multiplier = std::pow(cascadeMultiplier, level);
			CascadeShadowConsts shadowConts;
			shadowConts.tm = MakeCascadeProjTM(camera.Position(), lightDir, level);
			shadowConts.lightDir = lightDir;
			shadowConts.texelSize.x = (cascadeShadowSize / cascadeShadowLength) * multiplier;
			shadowConts.texelSize.y = 1.0 / (cascadeShadowDepth * multiplier);
			cascadeShadowsCBs[level].Update(renderDevice->GetContext(), shadowConts);
			cascadeShadowTMs[level] = shadowConts.tm;
		}
		cascadeShadowTMBuffer.Update(
			renderDevice->GetContext(), cascadeShadowTMs.data(),
			cascadeShadowTMs.size() * sizeof(Matrix44f));

		{
			wikiDraw.PushProjViewTM(Mul(camera.ViewMatrix(), camera.PerspectiveMatrix()));
			wikiDraw.PushWorldTM(Matrix44f{zero});

			/*
			wikiDraw.Begin(WikiDraw::Lines);
		//	for (uint s = 0; s < surfels.size(); ++s)
		//	{
		//		wikiDraw.SetColor(255, 255, 255, 255);
		//		wikiDraw.PushVertex(surfels[s].position);
		//		wikiDraw.SetColor(255, 128, 0, 255);
		//		wikiDraw.PushVertex(surfels[s].position + surfels[s].normal * 0.05f);
		//	}
			for (uint s = 0; s < surfelConns.size(); ++s)
			{
				if (surfelConns[s].sky != 0.0f)
					wikiDraw.SetColor(255, 255, 255, 255);
				else
					wikiDraw.SetColor(0, 128, 255, 255);
				wikiDraw.PushVertex(surfels[s / 16].position);
				wikiDraw.PushVertex(surfels[s / 16].position + surfelConns[s].dir * 0.15f);
			}
			wikiDraw.End();
			*/
			/*
			wikiDraw.Begin(WikiDraw::Points);
			for (uint s = 0; s < surfels.size(); ++s)
			{
				const float sky = surfels[s].skyVisibility;
				wikiDraw.SetColor(sky * 128, sky * 255, 128 + sky * 127, 255);
				wikiDraw.PushVertex(surfels[s].position);
			}
			wikiDraw.End();
			*/
			/*
			wikiDraw.Begin(WikiDraw::Lines);
			for (uint s = 0; s < surfelConns.size(); ++s)
			{
				const Vector3f source = surfels[surfelConns[s].surfelId].position;
				const Vector3f dest = surfelConns[s].point;
				wikiDraw.SetColor(255, 255, 255, 255);
				wikiDraw.PushVertex(source);
				wikiDraw.SetColor(255, 128, 0, 255);
				wikiDraw.PushVertex(dest);
			}
			wikiDraw.End();
			*/
			/*
			wikiDraw.Begin(WikiDraw::Lines);
			wikiDraw.SetColor(255, 255, 0, 255);
			for (uint s = 0; s < testIds.size(); ++s)
			{
				const pbrt::Primitive& p = kdTree.primitives[testIds[s]];
				wikiDraw.PushVertex(p.vertices[0]);
				wikiDraw.PushVertex(p.vertices[1]);
				wikiDraw.PushVertex(p.vertices[1]);
				wikiDraw.PushVertex(p.vertices[2]);
				wikiDraw.PushVertex(p.vertices[2]);
				wikiDraw.PushVertex(p.vertices[0]);
			}
			wikiDraw.End();
			*/

			const float size = 0.035f;
			float y = 0.85f;
			wikiDraw.PushProjViewTM(Matrix44f{zero});
			const float frameTime = deltaTime.AsFloat();
			if (movingFrameTime == 0.0f)
				movingFrameTime = frameTime;
			else
				movingFrameTime = Lerp(frameTime, movingFrameTime, std::exp(-frameTime*2.0f));
			wikiText.OutputText(
				wikiDraw, Vector2f{ -0.95f, y }, size,
				"%4.3f ms / %4.1f fps",
				movingFrameTime * 1000.0f,
				1.0f / movingFrameTime);
			y -= size;

			wikiText.OutputText(
				wikiDraw, Vector2f{ -0.95f, y }, size,
				"Frame: %4.3f ms",
				gpuFrame.AsFloat() * 1000.0f);
			y -= size;
			wikiText.OutputText(
				wikiDraw, Vector2f{ -0.95f, y }, size,
				"Solid: %4.3f ms",
				gpuSolid.AsFloat() * 1000.0f);
			y -= size;
			wikiText.OutputText(
				wikiDraw, Vector2f{ -0.95f, y }, size,
				"OIT: %4.3f ms",
				gpuOIT.AsFloat() * 1000.0f);
			y -= size;
			wikiText.OutputText(
				wikiDraw, Vector2f{ -0.95f, y }, size,
				"Shadows: %4.3f ms",
				gpuShadows.AsFloat() * 1000.0f);
			y -= size;
		}

		gpuTimer.Begin();
		{
			VIDF_GPU_EVENT(renderDevice, Frame);
			GpuAutoSection _{ gpuFrame };

			{
				VIDF_GPU_EVENT(renderDevice, CascadeShadows);
				GpuAutoSection _{ gpuShadows };

				for (uint level = 0; level < numShadowCascades; ++level)
				{
					VIDF_GPU_EVENT(renderDevice, ShadowPass);

					commandBuffer.GetContext()->ClearDepthStencilView(cascadeShadowMapLayers[level].dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
					commandBuffer.BeginRenderPass(cascadeShadowMapPasses[level]);
					commandBuffer.SetConstantBuffer(0, viewCB.buffer);
					commandBuffer.SetConstantBuffer(1, cascadeShadowsCBs[level].buffer);

					commandBuffer.SetVertexStream(0, vertexBuffer.buffer, sizeof(Vertex));
					commandBuffer.SetGraphicsPSO(shadowPSO);
					commandBuffer.GetContext()->PSSetSamplers(0, 1, &diffuseSS.Get());
					for (const auto& batch : solidBatches)
					{
						commandBuffer.SetSRV(0, bspData.textures[batch.textureId].gpuTexture.srv);
						commandBuffer.Draw(batch.count, batch.first);
					}
					commandBuffer.EndRenderPass();
				}
			}

			{
				VIDF_GPU_EVENT(renderDevice, Solid);
				GpuAutoSection _{ gpuSolid };

				FLOAT black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
				commandBuffer.GetContext()->ClearRenderTargetView(solidRT.rtv, black);
				commandBuffer.GetContext()->ClearDepthStencilView(depthDSV.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

				commandBuffer.BeginRenderPass(solidPass);
				commandBuffer.SetVertexStream(0, vertexBuffer.buffer, sizeof(Vertex));
				commandBuffer.SetConstantBuffer(0, viewCB.buffer);

				{
					VIDF_GPU_EVENT(renderDevice, ZOnly);
					commandBuffer.SetGraphicsPSO(zOnlyPSO);
					for (const auto& batch : solidBatches)
						commandBuffer.Draw(batch.count, batch.first);
				}
				
				{
					VIDF_GPU_EVENT(renderDevice, ForwardPlus);
					commandBuffer.SetGraphicsPSO(solidPSO);
					commandBuffer.SetConstantBuffer(1, cascadeShadowsCBs[0].buffer);
					commandBuffer.GetContext()->PSSetSamplers(0, 1, &diffuseSS.Get());
					commandBuffer.GetContext()->PSSetSamplers(1, 1, &shadowSS.Get());
					commandBuffer.SetSRV(0, lightManager.GetKdNodesBuffer().srv);
					commandBuffer.SetSRV(1, lightManager.GetKdLeafsBuffer().srv);
					commandBuffer.SetSRV(2, lightManager.GetLightIdsBuffer().srv);
					commandBuffer.SetSRV(3, lightManager.GetShadesBuffer().srv);
					commandBuffer.SetSRV(4, cascadeShadowMap.srv);
					commandBuffer.SetSRV(5, cascadeShadowTMBuffer.srv);
					for (const auto& batch : solidBatches)
					{
						commandBuffer.SetSRV(6, bspData.textures[batch.textureId].gpuTexture.srv);
						commandBuffer.Draw(batch.count, batch.first);
					}
				}

				commandBuffer.EndRenderPass();
			}

			{
				VIDF_GPU_EVENT(renderDevice, OIT);
				GpuAutoSection _{ gpuOIT };

				commandBuffer.BeginRenderPass(oitPass);

				commandBuffer.SetGraphicsPSO(oitClearPSO);
				commandBuffer.Draw(3, 0);

				commandBuffer.SetVertexStream(0, vertexBuffer.buffer, sizeof(Vertex));
				commandBuffer.SetGraphicsPSO(oitPSO);
				commandBuffer.GetContext()->PSSetSamplers(0, 1, &diffuseSS.Get());
				commandBuffer.GetContext()->PSSetSamplers(1, 1, &shadowSS.Get());
				commandBuffer.SetConstantBuffer(0, viewCB.buffer);
				commandBuffer.SetConstantBuffer(1, cascadeShadowsCBs[0].buffer);
				commandBuffer.SetSRV(0, lightManager.GetKdNodesBuffer().srv);
				commandBuffer.SetSRV(1, lightManager.GetKdLeafsBuffer().srv);
				commandBuffer.SetSRV(2, lightManager.GetLightIdsBuffer().srv);
				commandBuffer.SetSRV(3, lightManager.GetShadesBuffer().srv);
				commandBuffer.SetSRV(4, cascadeShadowMap.srv);
				commandBuffer.SetSRV(5, cascadeShadowTMBuffer.srv);
				for (const auto& batch : oitBatches)
				{
					commandBuffer.SetSRV(6, bspData.textures[batch.textureId].gpuTexture.srv);
					commandBuffer.Draw(batch.count, batch.first);
				}

				commandBuffer.EndRenderPass();
			}
						
			blur0.Draw(commandBuffer);
			reducePass0.Draw(commandBuffer);
			blur1.Draw(commandBuffer);
			blur2.Draw(commandBuffer);
			reducePass1.Draw(commandBuffer);
			blur3.Draw(commandBuffer);
			blur4.Draw(commandBuffer);

			{
				VIDF_GPU_EVENT(renderDevice, Finalize);

				commandBuffer.BeginRenderPass(finalizePass);

				commandBuffer.SetConstantBuffer(0, viewCB.buffer);
				commandBuffer.GetContext()->PSSetSamplers(0, 1, &diffuseSS.Get());
				commandBuffer.SetSRV(0, solidRT.srv);
				commandBuffer.SetSRV(1, oitRT.srv);
				commandBuffer.SetSRV(2, blur0.GetOutput().srv);
				commandBuffer.SetSRV(3, blur1.GetOutput().srv);
				commandBuffer.SetSRV(4, blur2.GetOutput().srv);
				commandBuffer.SetSRV(5, blur3.GetOutput().srv);
				commandBuffer.SetSRV(6, blur4.GetOutput().srv);
				commandBuffer.SetGraphicsPSO(finalPSO);
				commandBuffer.Draw(3, 0);

				{
					VIDF_GPU_EVENT(renderDevice, WikiDraw);
					wikiDraw.Flush(&commandBuffer);
				}

				commandBuffer.EndRenderPass();
			}
		}

		gpuTimer.End();
		swapChain->Present(false);
	}
}




/////////////////////////////////////////////////////////////////////



class GBufferPass : public PassNode
{
public:
	GBufferPass()
	{
		AddOutput("Diffuse");
		AddOutput("Normal");
	}

	void Build(uint outputIdx) override
	{
	}

private:
};



class FinalPass : public PassNode
{
public:
	FinalPass()
	{
		AddInput("FrameBuffer");
		AddOutput("Output");
	}

	void Build(uint outputIdx) override
	{
	}

private:
};



class DisplayPass : public PassNode
{
public:
	DisplayPass(uint width, uint height, DXGI_FORMAT format)
	{
		AddInput("Display");
	}

	virtual void Build(uint outputIdx) override
	{
	}

	void Build()
	{
		auto input = GetInput(0);
	}

private:
};



void H2Dx11()
{
	auto gBufferPass = std::make_shared<GBufferPass>();
	auto displayPass = std::make_shared<DisplayPass>(1024, 780, DXGI_FORMAT_R8G8B8A8_UNORM);
	PassNode::Connect(gBufferPass, "Diffuse", displayPass, "Display");

	displayPass->Build();
}

#endif
