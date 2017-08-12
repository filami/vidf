#pragma once

#include "common.h"

namespace vidf { namespace dx11
{



	struct GraphicsPSODesc
	{
		D3D11_INPUT_ELEMENT_DESC* geometryDesc = nullptr;
		uint                      numGeomDesc = 0;
		D3D11_RASTERIZER_DESC2    rasterizer = {};
		D3D11_DEPTH_STENCIL_DESC  depthStencil = {};
		ShaderPtr                 vertexShader;
		ShaderPtr                 pixelShader;
		D3D_PRIMITIVE_TOPOLOGY    topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	};

	struct GraphicsPSO
	{
		PD3D11InputLayout       inputAssembly;
		PD3D11RasterizerState2  rasterizer;
		PD3D11DepthStencilState depthStencil;
		PD3D11VertexShader      vertexShader;
		PD3D11PixelShader       pixelShader;
		D3D_PRIMITIVE_TOPOLOGY  topology;

		static GraphicsPSOPtr Create(RenderDevicePtr renderDevice, const GraphicsPSODesc& desc);
	};



	class CommandBuffer
	{
	private:
		static const uint numSrvs          = 8;
		static const uint numVertexStreams = 8;
		struct VertexStream
		{
			PD3D11Buffer buffer;
			uint stride = 0;
			uint offset = 0;
		};
		typedef std::array<PD3D11ShaderResourceView, numSrvs> SRVArray;
		typedef std::array<VertexStream, numSrvs>             VertexStreamArray;

	public:
		CommandBuffer(RenderDevicePtr _renderDevice);

		void SetGraphicsPSO(GraphicsPSOPtr pso);
		void SetVertexStream(uint index, PD3D11Buffer stream, uint stride, uint offset=0);
		void SetSRV(uint index, PD3D11ShaderResourceView srv);
		void Draw(uint vertexCount, uint startVertexLocation);
		void ClearState();

	private:
		void FlushGraphicsState();

	private:
		RenderDevicePtr   renderDevice;
		GraphicsPSOPtr    currentGraphicsPSO;
		SRVArray          srvs;
		VertexStreamArray vertexStreams;
	};



} }
