#pragma once

#include "common.h"



namespace vidf::dx12
{


	struct DescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpu;
		D3D12_GPU_DESCRIPTOR_HANDLE gpu;
	};



	// TODO - implement Free and make it a slab allocator
	class DescriptorHeap
	{
	public:
		DescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, uint _maxDescriptors);

		ID3D12DescriptorHeap* GetHeap() { return heap; }
		DescriptorHandle      Alloc(uint count = 1);

	public:
		// private:
		PD3D12DescriptorHeap heap;
		DescriptorHandle     first;
		uint descriptorSize;
		uint lastDescriptor = 0;
		uint maxDescriptors;
	};



	// TODO - allocate pages instead of individual resources
	class ScratchAllocator
	{
	public:
		ScratchAllocator(PD3D12Device _device, D3D12_HEAP_TYPE _heap, D3D12_RESOURCE_STATES _state);

		void Reset();

		PD3D12Resource Alloc(uint size, void* data = nullptr);

	private:
		PD3D12Device           device;
		D3D12_HEAP_TYPE        heap;
		D3D12_RESOURCE_STATES  state;
		D3D12_RESOURCE_FLAGS   flags;
		vector<PD3D12Resource> commited;
	};


}
