#pragma once

#include "common.h"
#include "descriptorheap.h"



namespace vidf::dx12
{


enum class GPUBufferType
{
	Undefined,
	VertexBuffer,
	IndexBuffer,
	Texture2D,
	//	Texture3D,
	//	TextureCube,
	Buffer,
	ConstantBuffer,
};



enum GPUBufferUsageFlags
{
	GPUUsage_ShaderResource = 1 << 0,
	GPUUsage_RenderTarget = 1 << 1,
	GPUUsage_DepthStencil = 1 << 2,
	GPUUsage_UnorderedAccess = 1 << 3,
	GPUUsage_Dynamic = 1 << 4,
	//	GPUUsage_Staging = 1 << 5,
};



struct GPUBufferDesc
{
	GPUBufferType type = GPUBufferType::Undefined;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	uint   usage = 0;
	void*  dataPtr = nullptr;
	uint   dataSize = 0;
	uint   dataStride = 0;
	uint   width = 0;
	uint   height = 0;
	uint   mipLevels = 1;
	string name;
	bool   hasFastClear = false;
	D3D12_CLEAR_VALUE fastClear{};
};



class GPUBuffer
{
public:
// private:
	struct Resource
	{
		PD3D12Resource        resource;
		D3D12_RESOURCE_STATES state;
	};

public:
	GPUBuffer();
	GPUBuffer(RenderDevice* renderDevice, ID3D12GraphicsCommandList* submitCL, const GPUBufferDesc& _desc);

// private:
	PD3D12Resource            resource;
	D3D12_RESOURCE_STATES     state;
	D3D12_GPU_VIRTUAL_ADDRESS gpuHandle;
	DescriptorHandle          rtv;
	D3D12_VERTEX_BUFFER_VIEW  vbvs;
	D3D12_INDEX_BUFFER_VIEW   ibv;
	DescriptorHandle          cbv{};
	DescriptorHandle          dsv{};
	DescriptorHandle          srv{};
	DescriptorHandle          uav{};
	GPUBufferDesc             desc{};
};



class ResourceSet
{
private:
	struct Entry
	{
		D3D12_DESCRIPTOR_RANGE_TYPE type;
		D3D12_CPU_DESCRIPTOR_HANDLE cpu;
		uint         index;
		GPUBufferPtr buffer;
	};

public:
	ResourceSet();

	// private:
	GPUBufferPtr cb;
	GPUBufferPtr srv;
	GPUBufferPtr uav;

	void AddConstants(uint index, GPUBufferPtr gpuBuffer);
	void AddShaderResource(uint index, GPUBufferPtr gpuBuffer);
	void AddUnorderedAccess(uint index, GPUBufferPtr gpuBuffer);

// private:
	void AddBuffer(uint index, GPUBufferPtr gpuBuffer, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_DESCRIPTOR_RANGE_TYPE type);

// private:
	friend class RenderDevice;
	friend class RenderContext;
		
	array<D3D12_DESCRIPTOR_RANGE, 3> ranges;
	DescriptorHandle                 descTable;
	vector<Entry> entries;
	bool          dirty = true;
};



class ResourceLayout
{
public:
	void AddResourceSet(uint index, const ResourceSetPtr rs);

// private:
	friend class RenderDevice;
	friend class RenderContext;

	vector<D3D12_ROOT_PARAMETER> rootParams;
	vector<ResourceSetPtr> sets;
	vector<vector<D3D12_DESCRIPTOR_RANGE>> ranges;
	PD3D12RootSignature rootSignature;
	bool dirty = true;
};



}
