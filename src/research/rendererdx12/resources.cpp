#include "pch.h"
#include "resources.h"

namespace vidf::dx12
{



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
