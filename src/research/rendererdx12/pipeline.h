#pragma once

#include "common.h"

#if 0

namespace vidf::dx12
{


	struct RenderPassDesc
	{
		static const uint numRtvs = 8;
		static const uint numUavs = 8;
		typedef std::array<GPUBufferPtr, numRtvs> RTVArray;
		typedef std::array<GPUBufferPtr, numUavs> UAVArray;
		RTVArray     rtvs;
		UAVArray     uavs;
		GPUBufferPtr dsv;
		Viewport     viewport;
	};



	struct RenderPass
	{
	public:
		~RenderPass();

		static RenderPassPtr Create(RenderDevicePtr renderDevice, const RenderPassDesc& desc);

	private:
		friend class CommandBuffer;
	};



	class CommandBuffer
	{
	public:
		explicit CommandBuffer(RenderDevicePtr _renderDevice);

		void Begin();
		void End();

	private:
		RenderDevicePtr renderDevice;
		PD3D12GraphicsCommandList graphicsCommandList;
		uint64 fence;
		bool   isOpen = false;
	};


}

#endif