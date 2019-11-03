#include "pch.h"
#include "resources.h"
#include "renderdevice.h"

namespace vidf { namespace dx11 {


	uint GetFormatSize(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_A8_UNORM:
			return 1;
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return 4;
		default:
			__debugbreak();
		}
	}



	DXGI_FORMAT GetFormatTypeless(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_UNKNOWN:
			return DXGI_FORMAT_UNKNOWN;

		case DXGI_FORMAT_R8_TYPELESS:
		case DXGI_FORMAT_R8_SNORM:
		case DXGI_FORMAT_R8_SINT:
		case DXGI_FORMAT_R8_UINT:
		case DXGI_FORMAT_R8_UNORM:
		case DXGI_FORMAT_A8_UNORM:
			return DXGI_FORMAT_R8_TYPELESS;

		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return DXGI_FORMAT_R8G8B8A8_TYPELESS;
					
		case DXGI_FORMAT_R16_FLOAT:
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_R16_UINT:
		case DXGI_FORMAT_R16_SNORM:
		case DXGI_FORMAT_R16_SINT:
			return DXGI_FORMAT_R16_TYPELESS;

		case DXGI_FORMAT_R32_UINT:
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_D32_FLOAT:
			return DXGI_FORMAT_R32_TYPELESS;

		case DXGI_FORMAT_R16G16B16A16_FLOAT:
		case DXGI_FORMAT_R16G16B16A16_UNORM:
			return DXGI_FORMAT_R16G16B16A16_TYPELESS;

		case DXGI_FORMAT_R32G32B32A32_FLOAT:
			return DXGI_FORMAT_R32G32B32A32_TYPELESS;

		default:
			__debugbreak();
		}
	}



	DXGI_FORMAT GetDepthStencilFormat(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R16_UNORM:
		case DXGI_FORMAT_D16_UNORM:
			return DXGI_FORMAT_D16_UNORM;
		case DXGI_FORMAT_R32_FLOAT:
		case DXGI_FORMAT_D32_FLOAT:
			return DXGI_FORMAT_D32_FLOAT;
		default:
			__debugbreak();
		}
	}


	DXGI_FORMAT GetShaderResourceFormat(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_D16_UNORM:
			return DXGI_FORMAT_R16_UNORM;
		case DXGI_FORMAT_D32_FLOAT:
			return DXGI_FORMAT_R32_FLOAT;
		default:
			return format;
		}
	}



	Texture2D Texture2D::Create(RenderDevicePtr renderDevice, const Texture2DDesc& desc)
	{
		Texture2D output;

		D3D11_TEXTURE2D_DESC bufferDesc{};
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.Format = desc.format;
		bufferDesc.ArraySize = 1;
		bufferDesc.Width = desc.width;
		bufferDesc.Height = desc.heigh;
		bufferDesc.MipLevels = desc.mipLevels;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		std::vector<D3D11_SUBRESOURCE_DATA> data;
		const uint formatSize = GetFormatSize(desc.format);
		assert(formatSize != 0);	// using unknownw format
		if (desc.dataPtr != nullptr && desc.dataSize != 0)
		{
			data.resize(desc.mipLevels);
			const uint8* begin = reinterpret_cast<const uint8*>(desc.dataPtr);
			const uint8* end = begin + desc.dataSize;
			int width = bufferDesc.Width;
			int height = bufferDesc.Height;
			for (uint i = 0; i < desc.mipLevels; ++i)
			{
				data[i].pSysMem = begin;
				data[i].SysMemPitch = width* formatSize;
				data[i].SysMemSlicePitch = width * height * formatSize;
				begin += data[i].SysMemSlicePitch;
				width = Max(1, width / 2);
				height = Max(1, height / 2);
				assert(begin <= end);	// dataPtr is not large enough
			}
		}

		renderDevice->GetDevice()->CreateTexture2D(&bufferDesc, data.data(), &output.buffer.Get());
		if (desc.name)
			NameObject(output.buffer, desc.name);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.mipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		renderDevice->GetDevice()->CreateShaderResourceView(output.buffer, &srvDesc, &output.srv.Get());

		return output;
	}



	RWTexture2D RWTexture2D::Create(RenderDevicePtr renderDevice, const RWTexture2DDesc& desc)
	{
		RWTexture2D output;

		D3D11_TEXTURE2D_DESC bufferDesc{};
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		// bufferDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
		bufferDesc.Format = desc.format;
		bufferDesc.ArraySize = 1;
		bufferDesc.Width = desc.width;
		bufferDesc.Height = desc.heigh;
		bufferDesc.MipLevels = desc.mipLevels;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		renderDevice->GetDevice()->CreateTexture2D(&bufferDesc, nullptr, &output.buffer.Get());
		if (desc.name)
			NameObject(output.buffer, desc.name);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.mipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		renderDevice->GetDevice()->CreateShaderResourceView(output.buffer, &srvDesc, &output.srv.Get());

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		renderDevice->GetDevice()->CreateUnorderedAccessView(output.buffer, &uavDesc, &output.uav.Get());

		return output;
	}



	RenderTarget RenderTarget::Create(RenderDevicePtr renderDevice, const RenderTargetDesc& desc)
	{
		RenderTarget output;

		D3D11_TEXTURE2D_DESC bufferDesc{};
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		bufferDesc.Format = desc.format;
		bufferDesc.ArraySize = 1;
		bufferDesc.Width = desc.width;
		bufferDesc.Height = desc.heigh;
		bufferDesc.MipLevels = desc.mipLevels;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;

		renderDevice->GetDevice()->CreateTexture2D(&bufferDesc, nullptr, &output.buffer.Get());
		if (desc.name)
			NameObject(output.buffer, desc.name);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = desc.mipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		renderDevice->GetDevice()->CreateShaderResourceView(output.buffer, &srvDesc, &output.srv.Get());

		D3D11_RENDER_TARGET_VIEW_DESC rtvDes{};
		rtvDes.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		rtvDes.Texture2D.MipSlice = 0;
		renderDevice->GetDevice()->CreateRenderTargetView(output.buffer, &rtvDes, &output.rtv.Get());

		return output;
	}


	DepthStencil DepthStencil::Create(RenderDevicePtr renderDevice, const DepthStencilDesc& desc)
	{
		DepthStencil output;

		D3D11_TEXTURE2D_DESC bufferDesc{};
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_DEPTH_STENCIL;
		bufferDesc.Format = desc.format == DXGI_FORMAT_R32_FLOAT ? DXGI_FORMAT_R32_TYPELESS : DXGI_FORMAT_R16_TYPELESS;
		bufferDesc.ArraySize = 1;
		bufferDesc.Width = desc.width;
		bufferDesc.Height = desc.heigh;
		bufferDesc.MipLevels = desc.mipLevels;
		bufferDesc.SampleDesc.Count = 1;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT;
		renderDevice->GetDevice()->CreateTexture2D(&bufferDesc, nullptr, &output.buffer.Get());
		if (desc.name)
			NameObject(output.buffer, desc.name);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Format = desc.format;
		srvDesc.Texture2D.MipLevels = desc.mipLevels;
		srvDesc.Texture2D.MostDetailedMip = 0;
		renderDevice->GetDevice()->CreateShaderResourceView(output.buffer, &srvDesc, &output.srv.Get());

		D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = desc.format == DXGI_FORMAT_R32_FLOAT ? DXGI_FORMAT_D32_FLOAT : DXGI_FORMAT_D16_UNORM;
		dsvDesc.Texture2D.MipSlice = 0;
		renderDevice->GetDevice()->CreateDepthStencilView(output.buffer, &dsvDesc, &output.dsv.Get());

		return output;
	}



	StructuredBuffer StructuredBuffer::Create(RenderDevicePtr renderDevice, const StructuredBufferDesc& desc)
	{
		StructuredBuffer output;

		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		bufferDesc.ByteWidth = desc.stride * desc.count;
		bufferDesc.StructureByteStride = desc.stride;
		bufferDesc.Usage = desc.dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bufferDesc.CPUAccessFlags = desc.dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bool hasData = false;
		D3D11_SUBRESOURCE_DATA data{};
		if (desc.dataPtr != nullptr && desc.dataSize != 0)
		{
			hasData = true;
			data.pSysMem = desc.dataPtr;
			data.SysMemPitch = UINT(desc.dataSize);
		}
		renderDevice->GetDevice()->CreateBuffer(
			&bufferDesc,
			hasData ? &data : nullptr,
			&output.buffer.Get());
		if (desc.name)
			NameObject(output.buffer, desc.name);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.NumElements = desc.count;
		renderDevice->GetDevice()->CreateShaderResourceView(output.buffer, &srvDesc, &output.srv.Get());

		return output;
	}



	void StructuredBuffer::Update(PD3D11DeviceContext context, const void* data, uint dataSize)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		assert(mapped.RowPitch >= dataSize);
		memcpy(mapped.pData, data, dataSize);
		context->Unmap(buffer, 0);
	}



	RWStructuredBuffer RWStructuredBuffer::Create(RenderDevicePtr renderDevice, const RWStructuredBufferDesc& desc)
	{
		RWStructuredBuffer output;

		D3D11_BUFFER_DESC bufferDesc{};
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		bufferDesc.ByteWidth = desc.stride * desc.count;
		bufferDesc.StructureByteStride = desc.stride;
		bufferDesc.Usage = desc.dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		bufferDesc.CPUAccessFlags = desc.dynamic ? D3D11_CPU_ACCESS_WRITE : 0;
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		renderDevice->GetDevice()->CreateBuffer(&bufferDesc, nullptr, &output.buffer.Get());
		if (desc.name)
			NameObject(output.buffer, desc.name);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementWidth = desc.count;
		renderDevice->GetDevice()->CreateShaderResourceView(output.buffer, &srvDesc, &output.srv.Get());

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.NumElements = desc.count;
		renderDevice->GetDevice()->CreateUnorderedAccessView(output.buffer, &uavDesc, &output.uav.Get());

		return output;
	}



	VertexBuffer VertexBuffer::Create(RenderDevicePtr renderDevice, const VertexBufferDesc& desc)
	{
		VertexBuffer output;

		D3D11_BUFFER_DESC vertexBufferDesc{};
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.ByteWidth = desc.stride * desc.count;
		vertexBufferDesc.StructureByteStride = desc.stride;
		vertexBufferDesc.Usage = desc.dynamic ? D3D11_USAGE_DYNAMIC : D3D11_USAGE_DEFAULT;
		vertexBufferDesc.CPUAccessFlags = desc.dynamic ? D3D11_CPU_ACCESS_WRITE : 0;

		bool hasData = false;
		D3D11_SUBRESOURCE_DATA vertexData{};
		if (desc.dataPtr != nullptr && desc.dataSize != 0)
		{
			hasData = true;
			vertexData.pSysMem = desc.dataPtr;
			vertexData.SysMemPitch = UINT(desc.dataSize);
		}
		
		renderDevice->GetDevice()->CreateBuffer(
			&vertexBufferDesc,
			hasData ? &vertexData : nullptr,
			&output.buffer.Get());
		if (desc.name)
			NameObject(output.buffer, desc.name);

		return output;
	}



	void VertexBuffer::Update(PD3D11DeviceContext context, const void* data, uint dataSize)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		assert(mapped.RowPitch >= dataSize);
		memcpy(mapped.pData, data, dataSize);
		context->Unmap(buffer, 0);
	}



	ConstantBuffer ConstantBuffer::Create(RenderDevicePtr renderDevice, const ConstantBufferDesc& desc)
	{
		ConstantBuffer output;
				
		D3D11_BUFFER_DESC cBufferDesc{};
		cBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cBufferDesc.ByteWidth = desc.size;
		cBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		cBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		renderDevice->GetDevice()->CreateBuffer(&cBufferDesc, nullptr, &output.buffer.Get());
		if (desc.name)
			NameObject(output.buffer, desc.name);

		return output;
	}



	void ConstantBuffer::Update(PD3D11DeviceContext context, const void* data, uint dataSize)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		assert(mapped.RowPitch >= dataSize);
		memcpy(mapped.pData, data, dataSize);
		context->Unmap(buffer, 0);
	}



	////////////////////////////////////////////



	GPUBuffer GPUBuffer::Create(RenderDevicePtr renderDevice, const GPUBufferDesc& desc)
	{
#		define ESCAPE_ASSERT(cond) if (!(cond)) { __debugbreak(); goto escape; }

		PD3D11Device device = renderDevice->GetDevice();

		GPUBuffer output;

		if (!desc.aliasBuffer)
		{
			// Undefined type is only valid when alising buffers
			ESCAPE_ASSERT(desc.type != GPUBufferType::Undefined);
			// staging buffers can only be used for staging and nothing else
			ESCAPE_ASSERT((desc.usageFlags & GPUUsage_Staging) == 0 || ((desc.usageFlags & GPUUsage_Staging) != 0 && (desc.usageFlags == GPUUsage_Staging)));
			// constant buffers can only have one element
			ESCAPE_ASSERT(desc.type != GPUBufferType::ConstantBuffer || desc.elementCount == 1);

			UINT bindFlags = 0;
			UINT CPUAccessFlags = 0;
			UINT miscFlags = 0;
			D3D11_USAGE usage = D3D11_USAGE_DEFAULT;
			DXGI_FORMAT format = GetFormatTypeless(desc.format);

			if (desc.type == GPUBufferType::VertexBuffer)
			{
				bindFlags |= D3D11_BIND_VERTEX_BUFFER;
			}
			if (desc.type == GPUBufferType::IndexBuffer)
			{
				bindFlags |= D3D11_BIND_INDEX_BUFFER;
			}
			if (desc.type == GPUBufferType::ConstantBuffer)
			{
				bindFlags |= D3D11_BIND_CONSTANT_BUFFER;
			}
			if (desc.type == GPUBufferType::Structured)
			{
				miscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
			}
			if (desc.usageFlags & GPUUsage_ShaderResource)
			{
				bindFlags |= D3D11_BIND_SHADER_RESOURCE;
			}
			if (desc.usageFlags & GPUUsage_RenderTarget)
			{
				bindFlags |= D3D11_BIND_RENDER_TARGET;
			}
			if (desc.usageFlags & GPUUsage_DepthStencil)
			{
				bindFlags |= D3D11_BIND_DEPTH_STENCIL;
			}
			if (desc.usageFlags & GPUUsage_UnorderedAccess)
			{
				bindFlags |= D3D11_BIND_UNORDERED_ACCESS;
			}
			if (desc.usageFlags & GPUUsage_Dynamic)
			{
				CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
				usage = D3D11_USAGE_DYNAMIC;
			}
			if (desc.usageFlags & GPUUsage_Staging)
			{
				CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				usage = D3D11_USAGE_STAGING;
			}

			bool hasData = false;
			D3D11_SUBRESOURCE_DATA data{};
			if (desc.dataPtr != nullptr && desc.dataSize != 0)
			{
				hasData = true;
				data.pSysMem = desc.dataPtr;
				data.SysMemPitch = UINT(desc.dataSize);
				data.SysMemSlicePitch = UINT(desc.dataSlice);
			}

			switch (desc.type)
			{
			case GPUBufferType::Texture2D:
				{
					D3D11_TEXTURE2D_DESC bufferDesc{};
					bufferDesc.BindFlags = bindFlags;
					bufferDesc.Format = format;
					bufferDesc.Width = desc.width;
					bufferDesc.Height = desc.height;
					bufferDesc.MipLevels = desc.mipLevels;
					bufferDesc.ArraySize = desc.arraySize;
					bufferDesc.SampleDesc.Count = 1;
					bufferDesc.Usage = usage;
					bufferDesc.CPUAccessFlags = CPUAccessFlags;
					bufferDesc.MiscFlags = miscFlags;
					device->CreateTexture2D(
						&bufferDesc,
						hasData ? &data : nullptr,
						reinterpret_cast<ID3D11Texture2D**>(&output.buffer.Get()));
				}
				break;
			case GPUBufferType::Texture3D:
				{
					D3D11_TEXTURE3D_DESC bufferDesc{};
					bufferDesc.BindFlags = bindFlags;
					bufferDesc.Format = format;
					bufferDesc.Width = desc.width;
					bufferDesc.Height = desc.height;
					bufferDesc.Depth = desc.depth;
					bufferDesc.MipLevels = desc.mipLevels;
					bufferDesc.Usage = usage;
					bufferDesc.CPUAccessFlags = CPUAccessFlags;
					bufferDesc.MiscFlags = miscFlags;
					device->CreateTexture3D(
						&bufferDesc,
						hasData ? &data : nullptr,
						reinterpret_cast<ID3D11Texture3D**>(&output.buffer.Get()));
				}
				break;
			case GPUBufferType::TextureCube:
				ESCAPE_ASSERT(false); // TODO
				break;
			case GPUBufferType::VertexBuffer:
			case GPUBufferType::IndexBuffer:
			case GPUBufferType::Structured:
			case GPUBufferType::ConstantBuffer:
				{
					D3D11_BUFFER_DESC bufferDesc{};
					bufferDesc.BindFlags = bindFlags;					
					bufferDesc.ByteWidth = desc.elementStride * desc.elementCount;
					bufferDesc.StructureByteStride = desc.elementStride;
					bufferDesc.Usage = usage;
					bufferDesc.CPUAccessFlags = CPUAccessFlags;
					bufferDesc.MiscFlags = miscFlags;
					device->CreateBuffer(
						&bufferDesc,
						hasData ? &data : nullptr,
						reinterpret_cast<ID3D11Buffer**>(&output.buffer.Get()));
				}
				break;
			default:
				// unrecognized type
				ESCAPE_ASSERT(false);
				break;
			}
			if (desc.name)
				NameObject(output.buffer, desc.name);
			output.desc = desc;
		}
		else
		{
			// aliasied buffers either have to match source type or use undefined
			ESCAPE_ASSERT(desc.type == GPUBufferType::Undefined || desc.type == desc.aliasBuffer->desc.type);
			// aliasied buffers cannot have their own name, only parent's name
			ESCAPE_ASSERT(desc.name == nullptr || desc.name == desc.aliasBuffer->desc.name);
			// aliasied buffers cannot be initialized with data, only the parent
			ESCAPE_ASSERT(desc.dataPtr == nullptr);
			// staging buffers cannot be aliased
			ESCAPE_ASSERT((desc.usageFlags & GPUUsage_Staging) == 0);

			output.desc = desc.aliasBuffer->desc;
			output.desc.aliasBuffer = desc.aliasBuffer;
			output.desc.usageFlags = desc.usageFlags;
			output.desc.stencilSRV = desc.stencilSRV;
			output.buffer = desc.aliasBuffer->buffer;
		}

		if (desc.usageFlags & GPUUsage_ShaderResource)
		{
			D3D_SRV_DIMENSION viewDimension;
			switch (desc.type)
			{
			case GPUBufferType::Texture2D:
				if (desc.arraySize > 1 && desc.viewArrayLevel == -1)
					viewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
				else
					viewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				break;
			case GPUBufferType::Texture3D:
				viewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
				break;
			case GPUBufferType::TextureCube:
				ESCAPE_ASSERT(false); // TODO
				break;
			case GPUBufferType::Structured:
				viewDimension = D3D11_SRV_DIMENSION_BUFFER;
				break;
			default:
				// this type cannot have a shader resource type
				ESCAPE_ASSERT(false);
				break;
			}
						
			D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
			srvDesc.ViewDimension = viewDimension;
			srvDesc.Format = GetShaderResourceFormat(desc.format);
			switch (viewDimension)
			{
			case D3D11_SRV_DIMENSION_TEXTURE2D:
				srvDesc.Texture2D.MipLevels = desc.mipLevels;
				srvDesc.Texture2D.MostDetailedMip = 0;
				break;
			case D3D11_SRV_DIMENSION_TEXTURE2DARRAY:
				srvDesc.Texture2DArray.MipLevels = desc.mipLevels;
				srvDesc.Texture2DArray.MostDetailedMip = 0;
				srvDesc.Texture2DArray.FirstArraySlice = 0;
				srvDesc.Texture2DArray.ArraySize  = desc.arraySize;
				break;
			case D3D11_SRV_DIMENSION_TEXTURE3D:
				srvDesc.Texture3D.MipLevels = desc.mipLevels;
				srvDesc.Texture3D.MostDetailedMip = 0;
				break;
			case D3D11_SRV_DIMENSION_BUFFER:
				srvDesc.Buffer.NumElements = desc.elementCount;
				break;
			}
			device->CreateShaderResourceView(output.buffer, &srvDesc, &output.srv.Get());
		}

		if (desc.usageFlags & GPUUsage_RenderTarget)
		{
			D3D11_RTV_DIMENSION viewDimension;
			switch (desc.type)
			{
			case GPUBufferType::Texture2D:
				if (desc.arraySize > 1)
					viewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
				else
					viewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
				break;
			case GPUBufferType::Texture3D:
				ESCAPE_ASSERT(false); // TODO
				break;
			case GPUBufferType::TextureCube:
				ESCAPE_ASSERT(false); // TODO
				break;
			default:
				// this type cannot have a shader resource type
				ESCAPE_ASSERT(false);
				break;
			}

			D3D11_RENDER_TARGET_VIEW_DESC rtvDesc{};
			rtvDesc.ViewDimension = viewDimension;
			rtvDesc.Format = desc.format;
			switch (viewDimension)
			{
			case D3D11_RTV_DIMENSION_TEXTURE2D:
				rtvDesc.Texture2D.MipSlice = 0;
				break;
			case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
				rtvDesc.Texture2DArray.MipSlice = desc.viewMipLevel;
				rtvDesc.Texture2DArray.FirstArraySlice = desc.viewArrayLevel != -1 ? desc.viewArrayLevel : 0;
				rtvDesc.Texture2DArray.ArraySize = desc.viewArrayLevel != -1 ? 1 : desc.arraySize;
				break;
			default:
				// this type cannot have a render target
				ESCAPE_ASSERT(false);
				break;
			}
			device->CreateRenderTargetView(output.buffer, &rtvDesc, &output.rtv.Get());
		}

		if (desc.usageFlags & GPUUsage_DepthStencil)
		{
			D3D11_DSV_DIMENSION viewDimension;
			switch (desc.type)
			{
			case GPUBufferType::Texture2D:
				if (desc.arraySize > 1)
					viewDimension = D3D11_DSV_DIMENSION_TEXTURE2DARRAY;
				else
					viewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				break;
			case GPUBufferType::TextureCube:
				ESCAPE_ASSERT(false); // TODO
				break;
			default:
				// this type cannot have a shader resource type
				ESCAPE_ASSERT(false);
				break;
			}

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
			dsvDesc.ViewDimension = viewDimension;
			dsvDesc.Format = GetDepthStencilFormat(desc.format);
			switch (viewDimension)
			{
			case D3D11_DSV_DIMENSION_TEXTURE2D:
				dsvDesc.Texture2D.MipSlice = 0;
				break;
			case D3D11_DSV_DIMENSION_TEXTURE2DARRAY:
				dsvDesc.Texture2DArray.MipSlice = desc.viewMipLevel;
				dsvDesc.Texture2DArray.FirstArraySlice = desc.viewArrayLevel != -1 ? desc.viewArrayLevel : 0;
				dsvDesc.Texture2DArray.ArraySize = desc.viewArrayLevel != -1 ? 1 : desc.arraySize;
				break;
			default:
				// this type cannot have a render target
				ESCAPE_ASSERT(false);
				break;
			}

			device->CreateDepthStencilView(output.buffer, &dsvDesc, &output.dsv.Get());
		}

		if (desc.usageFlags & GPUUsage_UnorderedAccess)
		{			
			D3D11_UAV_DIMENSION viewDimension;
			switch (desc.type)
			{
			case GPUBufferType::Texture2D:
				if (desc.arraySize > 1 && desc.viewArrayLevel == -1)
					viewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
				else
					viewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
				break;
			case GPUBufferType::Texture3D:
				ESCAPE_ASSERT(false); // TODO
				break;
			case GPUBufferType::TextureCube:
				ESCAPE_ASSERT(false); // TODO
				break;
			case GPUBufferType::Structured:
				viewDimension = D3D11_UAV_DIMENSION_BUFFER;
				break;
			default:
				// this type cannot have a shader resource type
				ESCAPE_ASSERT(false);
				break;
			}
			
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
			uavDesc.ViewDimension = viewDimension;
			uavDesc.Format = desc.format;
			switch (viewDimension)
			{
			case D3D11_RTV_DIMENSION_TEXTURE2D:
				uavDesc.Texture2D.MipSlice = 0;
				break;
			case D3D11_RTV_DIMENSION_TEXTURE2DARRAY:
				uavDesc.Texture2DArray.MipSlice = desc.viewMipLevel;
				uavDesc.Texture2DArray.FirstArraySlice = desc.viewArrayLevel != -1 ? desc.viewArrayLevel : 0;
				uavDesc.Texture2DArray.ArraySize = desc.viewArrayLevel != -1 ? 1 : desc.arraySize;
				break;
			default:
				// this type cannot have a render target
				ESCAPE_ASSERT(false);
				break;
			}
			device->CreateUnorderedAccessView(output.buffer, &uavDesc, &output.uav.Get());
		}

		return output;

	escape:
		return GPUBuffer();
	}



	void GPUBuffer::Update(PD3D11DeviceContext context, const void* data, uint dataSize)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		assert(mapped.RowPitch >= dataSize);
		memcpy(mapped.pData, data, dataSize);
		context->Unmap(buffer, 0);
	}



} }
