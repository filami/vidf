#pragma once

#include "common.h"



namespace vidf::dx12
{


	struct DescriptorHandle
	{
		D3D12_CPU_DESCRIPTOR_HANDLE cpu;
		D3D12_GPU_DESCRIPTOR_HANDLE gpu;
	};



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


}
