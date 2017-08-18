#pragma once

#include "common.h"
#include "vidf/common/matrix44.h"
#include "resources.h"

namespace vidf { namespace dx11
{


	class WikiDraw
	{
	public:
		enum StreamType
		{
			Undefined,
			Points,
			Lines,
			Triangles,
			Quads,
		};

	private:
		enum PsoType
		{
			PointsPso,
			LinesPso,
			TrianglesPso,
			NumPsos,
		};

		struct CBuffer
		{
			Matrix44f viewProjTM;
			Matrix44f worldTM;
		};

		struct Vertex
		{
			Vector3f vertex;
			uint32   color = ~0;
		};

		struct Batch
		{
			StreamType type = Undefined;
			uint       firstVertex = 0;
			uint       numVertices = 0;
			uint       psoIdx = -1;
			uint       projViewTMIdx = 0;
			uint       worldTMIdx = 0;
		};

	public:
		WikiDraw(RenderDevicePtr _renderDevice, ShaderManager* _shaderManager);

		void Begin(StreamType type);
		void End();
		void SetColor(uint8 r, uint8 g, uint8 b, uint8 a);
		void PushVertex(Vector3f vertex);
		void PushVertex(Vertex vertex);

		void PushProjViewTM(const Matrix44f& tm);
		void PushWorldTM(const Matrix44f& tm);

		void Flush(CommandBuffer* commandBuffer);

	private:
		void RestartBatch();
		void UpdateCBuffer(CommandBuffer* commandBuffer, const Batch& batch);
		void UpdateVertexBuffer(CommandBuffer* commandBuffer);

	private:
		RenderDevicePtr             renderDevice;
		ShaderPtr                   vertexShader;
		ShaderPtr                   pixelShader;
		std::vector<GraphicsPSOPtr> PSOs;
		VertexBuffer                vertexBuffer;
		ConstantBuffer              cBuffer;
		uint                        gpuVertexCount = 0;

		std::vector<Vertex>    primitive;
		std::vector<Vertex>    vertices;
		std::vector<Batch>     batches;
		std::vector<Matrix44f> viewProjTMs;
		std::vector<Matrix44f> worldTMs;
		Vertex                 curVertex;
		Batch                  curBatch;
		bool                   streaming = false;
	};



	typedef std::shared_ptr<WikiDraw> WikiGeomPtr;



} }
