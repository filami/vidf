#pragma once

#include "common.h"
#include "vidf/common/matrix44.h"
#include "vidf/common/rect.h"
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
			Vector2f tc;
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
			uint       srvIdx = 0;
		};

	public:
		WikiDraw(RenderDevicePtr _renderDevice, ShaderManager* _shaderManager);

		void Begin(StreamType type);
		void End();
		void SetColor(uint8 r, uint8 g, uint8 b, uint8 a);
		void SetTexCoord(Vector2f tc);
		void PushVertex(Vector3f vertex);
		void PushVertex(Vertex vertex);

		void PushProjViewTM(const Matrix44f& tm);
		void PushWorldTM(const Matrix44f& tm);
		void PushSRV(PD3D11ShaderResourceView srv, PD3D11SamplerState sampler);
		void PushSRV(PD3D11ShaderResourceView srv);
		void ResetSRV();

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
		Texture2D                   defaultTexture;
		PD3D11SamplerState          defaultSampler;

		std::vector<Vertex>    primitive;
		std::vector<Vertex>    vertices;
		std::vector<Batch>     batches;
		std::vector<Matrix44f> viewProjTMs;
		std::vector<Matrix44f> worldTMs;
		std::vector<PD3D11ShaderResourceView> srvs;
		std::vector<PD3D11SamplerState> samplers;
		Vertex                 curVertex;
		Batch                  curBatch;
		bool                   streaming = false;
	};



	class WikiText
	{
	private:
		static const int numChars = 96;
		static const int firstChar = 32;

	public:
		WikiText(RenderDevicePtr _renderDevice);

		void OutputText(WikiDraw& wikiDraw, const Vector2f& position, const float size, const char* text, ...) const;

	private:
		Rectf     rects[numChars];
		float     scale;
		Texture2D texture;
	};



	typedef std::shared_ptr<WikiDraw> WikiGeomPtr;



} }
