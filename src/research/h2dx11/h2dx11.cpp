#include "pch.h"
#include <unordered_map>
#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"
#include "vidf/proto/mesh.h"


using namespace vidf;
using namespace dx11;
using namespace proto;


class Dx11CanvasListener : public CanvasListener
{
public:
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



namespace h2
{



	enum class StreamResult
	{
		Ok,
		Fail,
	};
	
	template<typename TStream>
	struct StreamTraits
	{
	};

	template<>
	struct StreamTraits<std::istream>
	{
		static constexpr bool IsInput() { return true; }
		static constexpr bool IsOutput() { return false; }
		static constexpr bool IsBinary() { return true; }
	};
	template<> struct StreamTraits<std::ifstream> : public StreamTraits<std::istream> {};

	StreamResult Stream(std::istream& stream, int16& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return StreamResult::Ok;
	};

	StreamResult Stream(std::istream& stream, uint16& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return StreamResult::Ok;
	};

	StreamResult Stream(std::istream& stream, int32& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return StreamResult::Ok;
	};
	
	StreamResult Stream(std::istream& stream, uint32& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return StreamResult::Ok;
	};

	StreamResult Stream(std::istream& stream, float& value)
	{
		stream.read(reinterpret_cast<char*>(&value), sizeof(value));
		return StreamResult::Ok;
	};

	template<typename TStream, typename T>
	StreamResult Stream(TStream& stream, Vector3<T>& value)
	{
		Stream(stream, value.x);
		Stream(stream, value.y);
		Stream(stream, value.z);
		return StreamResult::Ok;
	};

	template<int N>
	StreamResult Stream(std::istream& stream, char(& value)[N])
	{
		stream.read(value, N);
		return StreamResult::Ok;
	};

	template<int N>
	StreamResult Stream(std::istream& stream, uint8(&value)[N])
	{
		stream.read((char*)value, N);
		return StreamResult::Ok;
	};

	template<typename TStream, typename It>
	StreamResult Stream(TStream& stream, It& begin, It& end)
	{
		for (It it = begin; it != end; ++it)
			Stream(stream, *it);
		return StreamResult::Ok;
	};

	template<int N>
	StreamResult Stream(std::istream& stream, std::array<char, N>& value)
	{
		stream.read(value.data(), N);
		return StreamResult::Ok;
	};



	struct PakHeader
	{
		uint32 signature = 0;
		uint32 offset = 0;
		uint32 length = 0;
	};



	struct PakFile
	{
		char   fileName[56];
		uint32 position;
		uint32 length;
	};



	template<typename TStream>
	StreamResult Stream(TStream& stream, PakHeader& header)
	{
		Stream(stream, header.signature);
		if (header.signature != 0x4b434150)
			return StreamResult::Fail;
		Stream(stream, header.offset);
		Stream(stream, header.length);
		return StreamResult::Ok;
	}



	template<typename TStream>
	StreamResult Stream(TStream& stream, PakFile& dir)
	{
		Stream(stream, dir.fileName);
		Stream(stream, dir.position);
		Stream(stream, dir.length);
		return StreamResult::Ok;
	}



	class FileManager
	{
	public:
		typedef std::shared_ptr<std::ifstream> PakFileHandle;

	private:
		struct PakfileRef
		{
			PakFileHandle handle;
			uint          offset;
		};

	public:
		void          AddPak(const char* pakPath);
		PakFileHandle OpenFile(const char* fileName);

	private:
		std::unordered_map<std::string, PakfileRef> files;
	};



	void FileManager::AddPak(const char* pakPath)
	{
		PakFileHandle fileHandle = std::make_shared<std::ifstream>(pakPath, std::ios::binary);
		if (!*fileHandle)
			return;

		PakHeader header;
		if (Stream(*fileHandle, header) != StreamResult::Ok)
			return;
		const uint numFiles = header.length / sizeof(PakFile);
		fileHandle->seekg(header.offset);
		for (uint i = 0; i < numFiles; ++i)
		{
			PakFile directory;
			Stream(*fileHandle, directory);
			files[ToLower(directory.fileName)] = PakfileRef{ fileHandle, directory.position };
		}
	}



	FileManager::PakFileHandle FileManager::OpenFile(const char* fileName)
	{
		auto it = files.find(ToLower(fileName));
		if (it == files.end())
			return PakFileHandle();
		it->second.handle->seekg(it->second.offset);
		return it->second.handle;
	}



	struct BspTexture
	{
		std::string name;
		Texture2D   gpuTexture;
		uint        width  = 1;
		uint        heigth = 1;
		uint        flags = 0;
	};



	template<typename TStream>
	StreamResult StreamM8(TStream& stream, BspTexture& m8Data)
	{
		static_assert(StreamTraits<TStream>::IsInput(), "Only supports Input streams");
		static_assert(StreamTraits<TStream>::IsBinary(), "Only supports Binary streams");

		const size_t fileOffset = stream.tellg();

		uint32 signature;
		Stream(stream, signature);
		if (signature != 0x02)
			return StreamResult::Fail;

		char name[32];
		Stream(stream, name);

		std::array<uint32, 16> values;
		Stream(stream, values.begin(), values.end());
		m8Data.width = values[0];
		Stream(stream, values.begin(), values.end());
		m8Data.heigth = values[0];

		stream.seekg(fileOffset + 1028);
		Stream(stream, m8Data.flags);

		return StreamResult::Ok;
	}



	template<typename TStream>
	Texture2D LoadTextureM8(TStream& stream, RenderDevicePtr renderDevice, const char* textureName)
	{
		static_assert(StreamTraits<TStream>::IsInput(), "Only supports Input streams");
		static_assert(StreamTraits<TStream>::IsBinary(), "Only supports Binary streams");

		Texture2D texture;

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
		Texture2DDesc desc;
		desc.width = widths[0];
		desc.heigh = heigths[0];
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
		texture = Texture2D::Create(renderDevice, desc);

		return texture;
	}



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
	};

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

		return StreamResult::Ok;
	}


}


void H2Dx11()
{
	using namespace h2;

	FileManager fileManager;
	fileManager.AddPak("data/h2dx11/Htic2-0.pak");
	fileManager.AddPak("data/h2dx11/Htic2-1.pak");

	FileManager::PakFileHandle map = fileManager.OpenFile("maps/ssdocks.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/sstown.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/andplaza.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/andslums.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/hive1.bsp");
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

	RenderDevicePtr renderDevice = RenderDevice::Create(RenderDeviceDesc());
	if (!renderDevice)
		return;
	ShaderManager shaderManager(renderDevice);
	CommandBuffer commandBuffer(renderDevice);

	Dx11CanvasListener canvasListener;

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

	RenderTargetDesc solidRTDesc{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, uint(canvasDesc.width), uint(canvasDesc.height), "diffuseRT"};
	RenderTarget solidRT = RenderTarget::Create(renderDevice, solidRTDesc);

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
	PSODesc.depthStencil.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	PSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	PSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PSODesc.vertexShader = vertexShader;
	PSODesc.pixelShader = pixelShader;
	GraphicsPSOPtr solidPSO = GraphicsPSO::Create(renderDevice, PSODesc);
		
	PSODesc.pixelShader = pixelShaderOIT;
	PSODesc.depthStencil.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	GraphicsPSOPtr oitPSO = GraphicsPSO::Create(renderDevice, PSODesc);

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
	camera.SetCamera((bBox.max + bBox.min)*0.5f, Quaternionf(zero), Distance(bBox.min, bBox.max));

	struct ViewConsts
	{
		Matrix44f projTM;
		Matrix44f viewTM;
		Vector2f viewportSize;
		Vector2f invViewportSize;
		Vector3f viewPosition;
		float _;
	};
	ConstantBufferDesc viewCBDesc(sizeof(ViewConsts), "viewCB");
	ConstantBuffer viewCB = ConstantBuffer::Create(renderDevice, viewCBDesc);

	TimeCounter counter;
	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		Time deltaTime = counter.GetElapsed();
		camera.Update(deltaTime);

		ViewConsts viewConsts;
		viewConsts.projTM = camera.PerspectiveMatrix();
		viewConsts.viewTM = camera.ViewMatrix();
		viewConsts.viewportSize = Vector2f(canvasDesc.width, canvasDesc.height);
		viewConsts.invViewportSize = Vector2f(1.0f / canvasDesc.width, 1.0f / canvasDesc.height);
		viewConsts.viewPosition = camera.Position();
		viewCB.Update(renderDevice->GetContext(), viewConsts);

		{
			VIDF_GPU_EVENT(renderDevice, Frame);

			{
				VIDF_GPU_EVENT(renderDevice, Solid);

				FLOAT black[] = { 0.0f, 0.0f, 0.0f, 0.0f };
				commandBuffer.GetContext()->ClearRenderTargetView(solidRT.rtv, black);
				commandBuffer.GetContext()->ClearDepthStencilView(depthDSV.dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);

				commandBuffer.BeginRenderPass(solidPass);
				commandBuffer.SetConstantBuffer(0, viewCB.buffer);

				commandBuffer.SetVertexStream(0, vertexBuffer.buffer, sizeof(Vertex));
				commandBuffer.SetGraphicsPSO(solidPSO);
				commandBuffer.GetContext()->PSSetSamplers(0, 1, &diffuseSS.Get());
				for (const auto& batch : solidBatches)
				{
					commandBuffer.SetSRV(0, bspData.textures[batch.textureId].gpuTexture.srv);
					commandBuffer.Draw(batch.count, batch.first);
				}

				commandBuffer.EndRenderPass();
			}

			{
				VIDF_GPU_EVENT(renderDevice, OIT);

				commandBuffer.BeginRenderPass(oitPass);
				commandBuffer.SetConstantBuffer(0, viewCB.buffer);

				commandBuffer.SetGraphicsPSO(oitClearPSO);
				commandBuffer.Draw(3, 0);

				commandBuffer.SetVertexStream(0, vertexBuffer.buffer, sizeof(Vertex));
				commandBuffer.SetGraphicsPSO(oitPSO);
				commandBuffer.GetContext()->PSSetSamplers(0, 1, &diffuseSS.Get());
				for (const auto& batch : oitBatches)
				{
					commandBuffer.SetSRV(0, bspData.textures[batch.textureId].gpuTexture.srv);
					commandBuffer.Draw(batch.count, batch.first);
				}

				commandBuffer.EndRenderPass();
			}

			{
				VIDF_GPU_EVENT(renderDevice, Finalize);

				commandBuffer.BeginRenderPass(finalizePass);

				commandBuffer.SetSRV(0, solidRT.srv);
				commandBuffer.SetSRV(1, oitRT.srv);
				commandBuffer.SetGraphicsPSO(finalPSO);
				commandBuffer.Draw(3, 0);

				commandBuffer.EndRenderPass();
			}
		}

		swapChain->Present();
	}
}
