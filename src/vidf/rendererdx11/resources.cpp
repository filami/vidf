#include "pch.h"
#include "resources.h"
#include "renderdevice.h"

namespace vidf { namespace dx11 {


	uint GetFormatSize(DXGI_FORMAT format)
	{
		switch (format)
		{
		case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		case DXGI_FORMAT_R8G8B8A8_SNORM:
		case DXGI_FORMAT_R8G8B8A8_SINT:
		case DXGI_FORMAT_R8G8B8A8_UINT:
		case DXGI_FORMAT_R8G8B8A8_UNORM:
		case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
			return 4;
		default:
			return 0;
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
		renderDevice->GetDevice()->CreateBuffer(&bufferDesc, nullptr, &output.buffer.Get());
		if (desc.name)
			NameObject(output.buffer, desc.name);

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.NumElements = desc.count;
		srvDesc.Buffer.ElementWidth = desc.stride;
		renderDevice->GetDevice()->CreateShaderResourceView(output.buffer, &srvDesc, &output.srv.Get());

		return output;
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



} }
