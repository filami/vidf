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



}
