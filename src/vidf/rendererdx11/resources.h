#pragma once

#include "common.h"

namespace vidf { namespace dx11 {



	struct Texture2DDesc
	{
		Texture2DDesc(DXGI_FORMAT _format, uint _width, uint _heigh, const char* _name)
			: format(_format)
			, width(_width)
			, heigh(_heigh)
			, name(_name) {}

		const char* name;
		DXGI_FORMAT format;
		uint        width;
		uint        heigh;
		uint        mipLevels = 1;
	};

	struct Texture2D
	{
		PD3D11Texture2D          buffer;
		PD3D11ShaderResourceView srv;

		static Texture2D Create(RenderDevicePtr renderDevice, const Texture2DDesc& desc);
	};



	struct RWTexture2DDesc : public Texture2DDesc
	{
		RWTexture2DDesc(DXGI_FORMAT _format, uint _width, uint _heigh, const char* _name)
			: Texture2DDesc(_format, _width, _heigh, _name) {}
	};

	struct RWTexture2D : public Texture2D
	{
		PD3D11UnorderedAccessView uav;

		static RWTexture2D Create(RenderDevicePtr renderDevice, const RWTexture2DDesc& desc);
	};



	struct RenderTargetDesc : public Texture2DDesc
	{
		RenderTargetDesc(DXGI_FORMAT _format, uint _width, uint _heigh, const char* _name)
			: Texture2DDesc(_format, _width, _heigh, _name) {}
	};

	struct RenderTarget : public Texture2D
	{
		PD3D11RenderTargetView rtv;

		static RenderTarget Create(RenderDevicePtr renderDevice, const RenderTargetDesc& desc);
	};



	struct DepthStencilDesc : public Texture2DDesc
	{
		DepthStencilDesc(DXGI_FORMAT _format, uint _width, uint _heigh, const char* _name)
			: Texture2DDesc(_format, _width, _heigh, _name) {}
	};

	struct DepthStencil : public Texture2D
	{
		PD3D11DepthStencilView rtv;

		static DepthStencil Create(RenderDevicePtr renderDevice, const DepthStencilDesc& desc);
	};



	struct VertexBufferDesc
	{
		VertexBufferDesc(uint _stride, uint _count, const char* _name)
			: stride(_stride)
			, count(_count)
			, name(_name) {}

		template<typename VertexPtr>
		VertexBufferDesc(VertexPtr _dataPtr, uint _dataSize, const char* _name)
			: stride(sizeof(*_dataPtr))
			, count(_dataSize)
			, dataPtr(_dataPtr)
			, dataSize(_dataSize)
			, name(_name) {}

		const char* name;
		uint        stride;
		uint        count;
		void*       dataPtr = nullptr;
		uint        dataSize = 0;
	};

	struct VertexBuffer
	{
		PD3D11Buffer buffer;

		static VertexBuffer Create(RenderDevicePtr renderDevice, const VertexBufferDesc& desc);
	};


} }
