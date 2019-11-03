#pragma once

#include "common.h"
#include "renderdevice.h"

namespace vidf { namespace dx11
{


	struct GPUBuffer;



	struct GraphicsPSODesc
	{
		GraphicsPSODesc()
		{
			for (auto& rt : blend.RenderTarget)
				rt.RenderTargetWriteMask = 0xf;
		}
		std::vector<D3D11_INPUT_ELEMENT_DESC> geometryDesc;
		D3D11_RASTERIZER_DESC2    rasterizer = {};
		D3D11_DEPTH_STENCIL_DESC  depthStencil = {};
		D3D11_BLEND_DESC1         blend = {};
		ShaderPtr                 vertexShader;
		ShaderPtr                 geometryShader;
		ShaderPtr                 pixelShader;
		D3D_PRIMITIVE_TOPOLOGY    topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
	};

	struct GraphicsPSO
	{
		PD3D11InputLayout       inputAssembly;
		PD3D11RasterizerState2  rasterizer;
		PD3D11DepthStencilState depthStencil;
		PD3D11BlendState1       blend;
		ShaderPtr               vertexShader;
		ShaderPtr               geometryShader;
		ShaderPtr               pixelShader;
		D3D_PRIMITIVE_TOPOLOGY  topology;

		static GraphicsPSOPtr Create(RenderDevicePtr renderDevice, const GraphicsPSODesc& desc);
	};



	struct RenderPassDesc
	{
		static const uint numRtvs = 8;
		static const uint numUavs = 8;
		typedef std::array<PD3D11RenderTargetView, numRtvs>    RTVArray;
		typedef std::array<PD3D11UnorderedAccessView, numUavs> UAVArray;
		RTVArray               rtvs;
		UAVArray               uavs;
		PD3D11DepthStencilView dsv;
		D3D11_VIEWPORT         viewport;
	};

	struct RenderPass
	{
	public:
		~RenderPass();

		static RenderPassPtr Create(RenderDevicePtr renderDevice, const RenderPassDesc& desc);

	private:
		friend class CommandBuffer;
		std::vector<ID3D11RenderTargetView*>    rtvs;
		std::vector<ID3D11UnorderedAccessView*> uavs;
		PD3D11DepthStencilView dsv;
		uint                   firstUAV;
		D3D11_VIEWPORT         viewport;
		bool                   hasViewport;
	};



	class CommandBuffer
	{
	private:
		static const uint numCBs           = 8;
		static const uint numSrvs          = 8;
		static const uint numUavs          = 8;
		static const uint numVertexStreams = 8;
		struct VertexStream
		{
			PD3D11Buffer buffer;
			uint stride = 0;
			uint offset = 0;
		};
		struct IndexBuffer
		{
			PD3D11Buffer indexBuffer;
			DXGI_FORMAT  format = DXGI_FORMAT_R16_UINT;
			uint         offset = 0;
		};
		typedef std::array<PD3D11Buffer, numSrvs>             CBArray;
		typedef std::array<PD3D11ShaderResourceView, numSrvs> SRVArray;
		typedef std::array<VertexStream, numSrvs>             VertexStreamArray;

	public:
		CommandBuffer(RenderDevicePtr _renderDevice);
				
		void SetConstantBuffer(uint index, PD3D11Buffer cb);
		void SetConstantBuffer(uint index, GPUBuffer& cb);
		void SetSRV(uint index, PD3D11ShaderResourceView srv);

		void BeginRenderPass(RenderPassPtr renderPass);
		void EndRenderPass();
		void SetGraphicsPSO(GraphicsPSOPtr pso);
		void SetVertexStream(uint index, PD3D11Buffer stream, uint stride, uint offset = 0);
		void SetIndexBuffer(PD3D11Buffer indexBuffer, DXGI_FORMAT format, uint offset = 0);

		void Draw(uint vertexCount, uint startVertexLocation);
		void DrawIndexed(uint indexCount, uint startIndexLocation, int baseVertexLocation);
		void DrawInstanced(uint vertexCountPerInstance, uint instanceCount, uint startVertexLocation, uint startInstanceLocation);

		void BeginComputePass(RenderPassPtr renderPass);
		void EndComputePass();
		void SetCompute(ShaderPtr shader);
		void Dispatch(Vector3i threadGroupCount);

		RenderDevicePtr     GetRenderDevice() const { return renderDevice; }
		PD3D11DeviceContext GetContext() const { return renderDevice->GetContext(); }

	private:
		void FlushCommonState();
		void FlushGraphicsState();
		void FlushComputeState();

	private:
		RenderDevicePtr   renderDevice;
		GraphicsPSOPtr    currentGraphicsPSO;
		ShaderPtr         currentCompute;
		CBArray           cbs;
		SRVArray          srvs;
		VertexStreamArray vertexStreams;
		IndexBuffer       indexBuffer;
	};



} }
