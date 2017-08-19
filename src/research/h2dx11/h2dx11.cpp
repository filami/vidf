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
		// std::unordered_map<std::string, PakfileRef> files;
		std::map<std::string, PakfileRef> files;
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
			files[directory.fileName] = PakfileRef{ fileHandle, directory.position };
		}
	}



	FileManager::PakFileHandle FileManager::OpenFile(const char* fileName)
	{
		auto it = files.find(fileName);
		if (it == files.end())
			return PakFileHandle();
		it->second.handle->seekg(it->second.offset);
		return it->second.handle;
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
		uint32          plane;
		int32           frontChild;
		int32           backChild;
		Vector3<int16>  bboxMin;
		Vector3<int16>  bboxMax;
		uint16          firstFace;
		uint16          numFaces;
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

	struct BspData
	{
		std::vector<Vector3f> vertices;
		std::vector<BspEdge>  edges;
		std::vector<BspNode>  nodes;
		std::vector<int32>    faceEdges;
		std::vector<BspFace>  faces;
		std::vector<BspPlane> planes;
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

		return StreamResult::Ok;
	}


}


void H2Dx11()
{
	using namespace h2;

	FileManager fileManager;
	fileManager.AddPak("data/h2dx11/Htic2-0.pak");
	fileManager.AddPak("data/h2dx11/Htic2-1.pak");

	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/ssdocks.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/sstown.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/andslums.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/hive1.bsp");
	FileManager::PakFileHandle map = fileManager.OpenFile("maps/oglemine1.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/tutorial.bsp");
	// FileManager::PakFileHandle map = fileManager.OpenFile("maps/tutorial2.bsp");
	if (!map)
		return;

	BspData bspData;
	if (Stream(*map, bspData) != h2::StreamResult::Ok)
		return;

	const uint width = 1280;
	const uint height = 720;

	// std::cout << "Loading Model . . . ";
	// auto model = LoadObjModuleFromFile("data/leather_chair/leather_chair.obj");
	// std::cout << "DONE" << std::endl;

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
	std::vector<Vertex> vertices;
	for (const BspFace& face : bspData.faces)
	{
		auto GetVertexId = [&bspData, face](uint i)
		{
			int edgeId = bspData.faceEdges[face.firstEdge + i];
			return edgeId > 0 ? bspData.edges[edgeId].vId[1] : bspData.edges[-edgeId].vId[0];
		};

		const Vector3f normal = Normalize(bspData.planes[face.plane].normal) * (face.planeSide ? 1.0f : -1.0f);

		Vertex v0;
		v0.position = bspData.vertices[GetVertexId(0)];
		v0.normal = normal;
		v0.texCoord = Vector2f(zero);

		Vertex v1;
		v1.position = bspData.vertices[GetVertexId(1)];
		v1.normal = normal;
		v1.texCoord = Vector2f(zero);

		for (uint vertIdx = 2; vertIdx < face.numEdges; ++vertIdx)
		{
			Vertex v2;
			v2.position = bspData.vertices[GetVertexId(vertIdx)];
			v2.normal = normal;
			v2.texCoord = Vector2f(zero);

			bBox = Union(bBox, v0.position);
			bBox = Union(bBox, v1.position);
			bBox = Union(bBox, v2.position);

			vertices.push_back(v0);
			vertices.push_back(v1);
			vertices.push_back(v2);
			v1 = v2;
		}
	}
	VertexBuffer vertexBuffer = VertexBuffer::Create(
		renderDevice,
		VertexBufferDesc(vertices.data(), vertices.size(), "vertexBuffer"));

	ShaderPtr vertexShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "vsMain", ShaderType::VertexShader);
	ShaderPtr pixelShader = shaderManager.CompileShaderFile("data/shaders/shader.hlsl", "psMain", ShaderType::PixelShader);
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

	struct OIT
	{
		Vector4f fragments[8];
		float    depth[8];
		uint     numFrags;
	};
	RWStructuredBufferDesc rovTestDesc(sizeof(OIT), canvasDesc.width * canvasDesc.height, "rovTest");
	RWStructuredBuffer rovTest = RWStructuredBuffer::Create(renderDevice, rovTestDesc);

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
	PSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
	PSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	PSODesc.rasterizer.FrontCounterClockwise = true;
	PSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	PSODesc.vertexShader = vertexShader;
	PSODesc.pixelShader = pixelShader;
	GraphicsPSOPtr pso = GraphicsPSO::Create(renderDevice, PSODesc);

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

	RenderPassDesc renderPassDesc;
	renderPassDesc.viewport = viewport;
	renderPassDesc.uavs[0] = rovTest.uav;
	RenderPassPtr renderPass = RenderPass::Create(renderDevice, renderPassDesc);

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
				VIDF_GPU_EVENT(renderDevice, Render);

				commandBuffer.BeginRenderPass(renderPass);
				commandBuffer.SetConstantBuffer(0, viewCB.buffer);

				commandBuffer.SetGraphicsPSO(oitClearPSO);
				commandBuffer.Draw(3, 0);

				commandBuffer.SetVertexStream(0, vertexBuffer.buffer, sizeof(Vertex));
				commandBuffer.SetGraphicsPSO(pso);
				commandBuffer.Draw(vertices.size(), 0);

				commandBuffer.EndRenderPass();
			}

			{
				VIDF_GPU_EVENT(renderDevice, Finalize);

				commandBuffer.BeginRenderPass(finalizePass);

				commandBuffer.SetSRV(0, rovTest.srv);
				commandBuffer.SetGraphicsPSO(finalPSO);
				commandBuffer.Draw(3, 0);

				commandBuffer.EndRenderPass();
			}
		}

		swapChain->Present();
	}
}
