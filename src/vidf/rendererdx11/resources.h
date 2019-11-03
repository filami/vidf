#pragma once

#include "common.h"

namespace vidf { namespace dx11 {



	struct Texture2DDesc
	{
		Texture2DDesc() = default;
		Texture2DDesc(DXGI_FORMAT _format, uint _width, uint _heigh, const char* _name)
			: format(_format)
			, width(_width)
			, heigh(_heigh)
			, name(_name) {}

		const char* name = nullptr;
		DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
		void*       dataPtr = nullptr;
		uint        dataSize = 0;
		uint        width = 16;
		uint        heigh = 16;
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
		PD3D11DepthStencilView dsv;

		static DepthStencil Create(RenderDevicePtr renderDevice, const DepthStencilDesc& desc);
	};



	struct StructuredBufferDesc
	{
		StructuredBufferDesc(uint _stride, uint _count, const char* _name, bool _dynamic=false)
			: stride(_stride)
			, count(_count)
			, name(_name)
			, dynamic(_dynamic) {}

		const char* name = nullptr;
		void*       dataPtr = nullptr;
		uint        dataSize = 0;
		uint        stride;
		uint        count;
		bool        dynamic;
	};

	struct StructuredBuffer
	{
		PD3D11Buffer             buffer;
		PD3D11ShaderResourceView srv;

		static StructuredBuffer Create(RenderDevicePtr renderDevice, const StructuredBufferDesc& desc);
		void Update(PD3D11DeviceContext context, const void* data, uint dataSize);
	};



	struct RWStructuredBufferDesc : public StructuredBufferDesc
	{
		RWStructuredBufferDesc(uint _stride, uint _count, const char* _name, bool _dynamic = false)
			: StructuredBufferDesc(_stride, _count, _name, _dynamic) {}
	};

	struct RWStructuredBuffer : public StructuredBuffer
	{
		PD3D11UnorderedAccessView uav;

		static RWStructuredBuffer Create(RenderDevicePtr renderDevice, const RWStructuredBufferDesc& desc);
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
		bool        dynamic = false;
	};

	struct VertexBuffer
	{
		PD3D11Buffer buffer;

		static VertexBuffer Create(RenderDevicePtr renderDevice, const VertexBufferDesc& desc);
		void Update(PD3D11DeviceContext context, const void* data, uint dataSize);
	};



	struct ConstantBufferDesc
	{
		ConstantBufferDesc(uint _size, const char* _name)
			: size(_size)
			, name(_name) {}

		const char* name;
		uint        size;
	};

	struct ConstantBuffer
	{
		PD3D11Buffer buffer;

		static ConstantBuffer Create(RenderDevicePtr renderDevice, const ConstantBufferDesc& desc);
		void Update(PD3D11DeviceContext context, const void* data, uint dataSize);
		template<typename Type>
		void Update(PD3D11DeviceContext context, const Type& object) { Update(context, &object, sizeof(Type)); }
	};



	////////////////////////////////////////////



	class GPUBuffer;


	enum class GPUBufferType
	{
		Undefined,
		VertexBuffer,
		IndexBuffer,
		Texture2D,
		Texture3D,
		TextureCube,
		Structured,
		ConstantBuffer,
	};



	enum GPUBufferUsageFlags
	{
		GPUUsage_ShaderResource  = 1 << 0,
		GPUUsage_RenderTarget    = 1 << 1,
		GPUUsage_DepthStencil    = 1 << 2,
		GPUUsage_UnorderedAccess = 1 << 3,
		GPUUsage_Dynamic         = 1 << 4,
		GPUUsage_Staging         = 1 << 5,
	};



	struct GPUBufferDesc
	{
		GPUBufferDesc()
			: width(1)
			, height(1)
			, depth(1)
			, mipLevels(1)
			, arraySize(1)
		{
		}

		GPUBuffer*    aliasBuffer = nullptr;
		GPUBufferType type = GPUBufferType::Undefined;
		union
		{
			struct
			{
				uint width;
				uint height;
				uint depth;
				uint mipLevels;
				uint arraySize;
			};
			struct
			{
				uint elementCount;
				uint elementStride;
			};
		};
		uint viewMipLevel = 0;
		int viewArrayLevel = -1;
		DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
		uint  usageFlags = 0;
		void* dataPtr = nullptr;
		uint  dataSize = 0;
		uint  dataSlice = 0;
		bool  stencilSRV = false;
		const char* name = nullptr;
	};


	struct GPUBuffer
	{
		GPUBufferDesc       desc;
		PD3D11Resource      buffer;
		PD3D11UnorderedAccessView uav;
		PD3D11ShaderResourceView  srv;
		PD3D11RenderTargetView    rtv;
		PD3D11DepthStencilView    dsv;

		PD3D11Buffer GetBuffer() { return reinterpret_cast<ID3D11Buffer*>(buffer.Get()); }

		static GPUBuffer Create(RenderDevicePtr renderDevice, const GPUBufferDesc& desc);
		void Update(PD3D11DeviceContext context, const void* data, uint dataSize);
		template<typename Type>
		void Update(PD3D11DeviceContext context, const Type& object) { Update(context, &object, sizeof(Type)); }
	};



} }
