#include "pch.h"
#include "resources.h"
#include "renderdevice.h"

namespace vidf::dx12
{



GPUBuffer::GPUBuffer()
{
}



GPUBuffer::GPUBuffer(RenderDevice* renderDevice, ID3D12GraphicsCommandList* submitCL, const GPUBufferDesc& _desc)
{
	// TODO : add optional fast clear feature and disable EXECUTION WARNING #820: CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE goofy warning

	PD3D12Device device = renderDevice->GetDevice();
	desc = _desc;

	Assert(desc.type != GPUBufferType::IndexBuffer || desc.format == DXGI_FORMAT_R16_UINT || desc.format == DXGI_FORMAT_R32_UINT);

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;

	D3D12_CLEAR_VALUE clearValue{};
	bool hasGpuHandle = false;
	bool hasClearValue = false;

	switch (desc.type)
	{
	case GPUBufferType::VertexBuffer:
	case GPUBufferType::IndexBuffer:
	case GPUBufferType::ConstantBuffer:
	case GPUBufferType::Buffer:
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		bufferDesc.Width = DivUp(desc.dataSize, 256) * 256;
		bufferDesc.Height = 1;
		bufferDesc.MipLevels = 1;
		bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		hasGpuHandle = true;
		break;
	case GPUBufferType::Texture2D:
		Assert(desc.width != 0);
		Assert(desc.height != 0);
		Assert(desc.format != DXGI_FORMAT_UNKNOWN);
		bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		bufferDesc.Width = desc.width;
		bufferDesc.Height = desc.height;
		bufferDesc.MipLevels = desc.mipLevels;
		bufferDesc.Format = GetFormatTypeless(desc.format);
		break;
	default:
		// unrecognized type
		__debugbreak();
	};

	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ;
	if (desc.usage & GPUUsage_DepthStencil)
	{
		bufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		state = D3D12_RESOURCE_STATE_DEPTH_WRITE;
	}

	if (desc.usage & GPUUsage_UnorderedAccess)
		bufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	if (desc.usage & GPUUsage_RenderTarget)
		bufferDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	if (desc.usage & GPUUsage_DepthStencil)
	{
		clearValue.Format = desc.format;
		clearValue.DepthStencil.Depth = 1.0f;
		clearValue.DepthStencil.Stencil = 0;
		hasClearValue = true;
	}

	AssertHr(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		state,
		hasClearValue ? &clearValue : nullptr,
		IID_PPV_ARGS(&resource[0].resource.Get())));
	resource[0].resource->SetName(ToWString(desc.name.c_str()).c_str());
	resource[0].state = state;
	resource[1] = resource[0];
	if (hasGpuHandle)
		gpuHandle = resource[0].resource->GetGPUVirtualAddress();

	switch (desc.type)
	{
	case GPUBufferType::VertexBuffer:
		vbvs.BufferLocation = resource[0].resource->GetGPUVirtualAddress();
		vbvs.StrideInBytes = desc.dataStride;
		vbvs.SizeInBytes = desc.dataSize;
		break;
	case GPUBufferType::IndexBuffer:
		ibv.BufferLocation = resource[0].resource->GetGPUVirtualAddress();
		ibv.Format = desc.format;
		ibv.SizeInBytes = desc.dataSize;
		break;
	case GPUBufferType::ConstantBuffer:
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = resource[0].resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = bufferDesc.Width;
		cbv = renderDevice->GetViewHeap()->Alloc();
		device->CreateConstantBufferView(&cbvDesc, cbv.cpu);
	}
	break;
	};

	if (desc.usage & GPUUsage_DepthStencil)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc{};
		viewDesc.Format = GetDepthStencilFormat(desc.format);

		switch (desc.type)
		{
		case GPUBufferType::Texture2D:
			viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = 0;
			break;
		default:
			// unrecognized type
			__debugbreak();
		};

		dsv = renderDevice->GetDSVHeap()->Alloc();
		device->CreateDepthStencilView(resource[0].resource, &viewDesc, dsv.cpu);
	}

	if (desc.usage & GPUUsage_ShaderResource)
	{
		D3D12_SRV_DIMENSION viewDimension;
		switch (desc.type)
		{
		case GPUBufferType::Texture2D:
			viewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			break;
		case GPUBufferType::Buffer:
			viewDimension = D3D12_SRV_DIMENSION_BUFFER;
			break;
		default:
			// unrecognized type
			__debugbreak();
			break;
		}

		D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc{};
		viewDesc.ViewDimension = viewDimension;
		viewDesc.Format = GetShaderResourceFormat(desc.format);
		viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		switch (viewDimension)
		{
		case D3D12_SRV_DIMENSION_TEXTURE2D:
			viewDesc.Texture2D.MipLevels = desc.mipLevels;
			viewDesc.Texture2D.MostDetailedMip = 0;
			break;
		case D3D12_SRV_DIMENSION_BUFFER:
			viewDesc.Buffer.FirstElement = 0;
			viewDesc.Buffer.NumElements = desc.dataSize / desc.dataStride;
			viewDesc.Buffer.StructureByteStride = desc.dataStride;
			viewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
			break;
		default:
			// unrecognized type
			__debugbreak();
			break;
		}
		srv = renderDevice->GetViewHeap()->Alloc();
		device->CreateShaderResourceView(resource[0].resource, &viewDesc, srv.cpu);
	}

	if (desc.usage & GPUUsage_UnorderedAccess)
	{
		D3D12_UAV_DIMENSION viewDimension;
		switch (desc.type)
		{
		case GPUBufferType::Texture2D:
			viewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			break;
		default:
			// unrecognized type
			__debugbreak();
			break;
		}

		D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc{};
		viewDesc.ViewDimension = viewDimension;
		viewDesc.Format = GetUAVFormat(desc.format);
		switch (viewDimension)
		{
		case D3D12_UAV_DIMENSION_TEXTURE2D:
			viewDesc.Texture2D.MipSlice = 0;
			viewDesc.Texture2D.PlaneSlice = 0;
			break;
		default:
			// unrecognized type
			__debugbreak();
			break;
		}
		uav = renderDevice->GetViewHeap()->Alloc();
		device->CreateUnorderedAccessView(resource[0].resource, nullptr, &viewDesc, uav.cpu);
	}

	if (desc.usage & GPUUsage_RenderTarget)
	{
		D3D12_RENDER_TARGET_VIEW_DESC viewDesc{};
		viewDesc.Format = desc.format;

		switch (desc.type)
		{
		case GPUBufferType::Texture2D:
			viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
			viewDesc.Texture2D.MipSlice = 0;
			break;
		default:
			// unrecognized type
			__debugbreak();
		};

		rtvs[0] = renderDevice->GetRTVHeap()->Alloc();
		device->CreateRenderTargetView(resource[0].resource, &viewDesc, rtvs[0].cpu);
		rtvs[1] = rtvs[0];
	}

	if (desc.dataPtr)
	{
		array<D3D12_SUBRESOURCE_DATA, 16> data;
		uint firstResource = 0;
		uint numResources;
		if (desc.type == GPUBufferType::Texture2D)
		{
			numResources = desc.mipLevels;
			uint width = desc.width;
			uint height = desc.height;
			const uint8* ptr = (const uint8*)desc.dataPtr;
			for (uint i = 0; i < numResources; ++i)
			{
				data[i].pData = ptr;
				data[i].RowPitch = width * GetFormatSize(desc.format);
				data[i].SlicePitch = data[i].RowPitch * height;
				ptr += data[i].SlicePitch;
				width = DivUp(width, 2);
				height = DivUp(height, 2);
			}
		}
		else
		{
			numResources = 1;
			data[0].pData = desc.dataPtr;
			data[0].RowPitch = desc.dataSize;
			data[0].SlicePitch = 0;
		}
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(resource[0].resource, firstResource, numResources);

		PD3D12Resource transient = renderDevice->GetUploadScratch()->Alloc(uploadBufferSize);

		submitCL->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(resource[0].resource,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				D3D12_RESOURCE_STATE_COPY_DEST));
		resource[0].state = D3D12_RESOURCE_STATE_COPY_DEST;
		UpdateSubresources(submitCL, resource[0].resource, transient, 0, firstResource, numResources, data.data());
	}
}



ResourceSet::ResourceSet()
{
	for (uint i = 0; i < ranges.size(); ++i)
	{
		ranges[i] = D3D12_DESCRIPTOR_RANGE();
		ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE(i);
		ranges[i].BaseShaderRegister = -1;
		ranges[i].OffsetInDescriptorsFromTableStart = -1;
	}
}



void ResourceSet::AddConstants(uint index, GPUBufferPtr gpuBuffer)
{
	Assert(gpuBuffer->desc.type == GPUBufferType::ConstantBuffer);
	AddBuffer(index, gpuBuffer, gpuBuffer->cbv.cpu, D3D12_DESCRIPTOR_RANGE_TYPE_CBV);
}



void ResourceSet::AddShaderResource(uint index, GPUBufferPtr gpuBuffer)
{
	Assert((gpuBuffer->desc.usage & GPUUsage_ShaderResource) != 0);
	AddBuffer(index, gpuBuffer, gpuBuffer->srv.cpu, D3D12_DESCRIPTOR_RANGE_TYPE_SRV);
}



void ResourceSet::AddUnorderedAccess(uint index, GPUBufferPtr gpuBuffer)
{
	Assert((gpuBuffer->desc.usage & GPUUsage_UnorderedAccess) != 0);
	AddBuffer(index, gpuBuffer, gpuBuffer->uav.cpu, D3D12_DESCRIPTOR_RANGE_TYPE_UAV);
}



void ResourceSet::AddBuffer(uint index, GPUBufferPtr gpuBuffer, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_DESCRIPTOR_RANGE_TYPE type)
{
	auto it = find_if(entries.begin(), entries.end(), [=](const auto& entry)
	{
		return entry.type == type && entry.index == index;
	});
	if (it != entries.end())
	{
		if (it->buffer == gpuBuffer)
			return;
		it->buffer = gpuBuffer;
	}
	else
	{
		Entry entry;
		entry.buffer = gpuBuffer;
		entry.cpu = cpu;
		entry.index = index;
		entry.type = type;
		entries.push_back(entry);
	}

	D3D12_DESCRIPTOR_RANGE& range = ranges[uint(type)];
	range.BaseShaderRegister = min(range.BaseShaderRegister, index);
	range.NumDescriptors = max(range.NumDescriptors, index - range.BaseShaderRegister + 1);

	uint offset = 0;
	for (uint i = 0; i < ranges.size(); ++i)
	{
		ranges[i].OffsetInDescriptorsFromTableStart = offset;
		offset += ranges[i].NumDescriptors;
	}

	dirty = true;
}



void ResourceLayout::AddResourceSet(uint index, const ResourceSetPtr rs)
{
	rootParams.resize(max(index + 1, uint(rootParams.size())));
	ranges.resize(max(index + 1, uint(ranges.size())));
	sets.resize(max(index + 1, uint(sets.size())));

	D3D12_ROOT_PARAMETER& param = rootParams[index];
	auto& range = ranges[index];
	range.clear();
	range.reserve(rs->ranges.size());
	for (uint i = 0; i < rs->ranges.size(); ++i)
	{
		if (rs->ranges[i].NumDescriptors != 0)
			range.push_back(rs->ranges[i]);
	}

	sets[index] = rs;

	param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	param.DescriptorTable.NumDescriptorRanges = range.size();
	param.DescriptorTable.pDescriptorRanges = range.data();
	param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	dirty = true;
}



}
