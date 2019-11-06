#include "pch.h"
#include "descriptorheap.h"

namespace vidf::dx12
{



DescriptorHeap::DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint _maxDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = _maxDescriptors;
	heapDesc.Type = type;
	heapDesc.Flags = flags;
	AssertHr(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap.Get())));
	descriptorSize = device->GetDescriptorHandleIncrementSize(type);
	first.cpu = heap->GetCPUDescriptorHandleForHeapStart();
	first.gpu = heap->GetGPUDescriptorHandleForHeapStart();
	maxDescriptors = _maxDescriptors;
}



DescriptorHandle DescriptorHeap::Alloc(uint count)
{
	Assert(lastDescriptor + count <= maxDescriptors);
	DescriptorHandle handle;
	handle.cpu.ptr = first.cpu.ptr + descriptorSize * lastDescriptor;
	handle.gpu.ptr = first.gpu.ptr + descriptorSize * lastDescriptor;
	lastDescriptor += count;
	return handle;
}



ScratchAllocator::ScratchAllocator(PD3D12Device _device, D3D12_HEAP_TYPE _heap, D3D12_RESOURCE_STATES _state)
	: device(_device)
	, heap(_heap)
	, state(_state)
{
	flags = D3D12_RESOURCE_FLAG_NONE;
	if (state == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
}



void ScratchAllocator::Reset()
{
	commited.clear();
}



D3D12_GPU_VIRTUAL_ADDRESS ScratchAllocator::Alloc(uint size, void* data)
{
	PD3D12Resource page;

	AssertHr(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(heap),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size, flags),
		state,
		nullptr,
		IID_PPV_ARGS(&page.Get())));

	if (data)
	{
		void *pMappedData;
		page->Map(0, nullptr, &pMappedData);
		memcpy(pMappedData, data, size);
		page->Unmap(0, nullptr);
	}

	commited.push_back(page);

	return page->GetGPUVirtualAddress();
}



}
