#include "pch.h"
#include "renderdevice.h"
#include "rendercontext.h"


namespace vidf::dx12
{

#if 0


	void GetHardwareAdapter(IDXGIFactory2* factory, Pointer<IDXGIAdapter1>* _adapter)
	{
		Pointer<IDXGIAdapter1> adapter;

		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter.Get()); ++adapterIndex)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;

			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr)))
				break;
		}
		*_adapter = adapter;
	}



	RenderDevicePtr RenderDevice::Create(const RenderDeviceDesc& desc)
	{
		RenderDevicePtr device = std::make_shared<RenderDevice>();
		if (!device->CreateDevice(desc))
			return RenderDevicePtr();
		return device;
	}



	bool RenderDevice::CreateDevice(const RenderDeviceDesc& desc)
	{
		if (desc.enableValidation)
		{
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&deviceDebug.Get()))))
			{
				deviceDebug->EnableDebugLayer();
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}

		AssertHr(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory.Get())));
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);
		AssertHr(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device.Get())));
				
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		AssertHr(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue.Get())));

		AssertHr(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&graphicsCommandAllocator.Get())));

		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = frameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		AssertHr(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap.Get())));
		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		AssertHr(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence.Get())));
		fenceEvent = CreateEvent(nullptr, false, false, nullptr);
		Assert(fenceEvent);

		return true;
	}



	uint64 RenderDevice::SetFence()
	{
		const uint64 fenceIdx = fenceValue;
		AssertHr(commandQueue->Signal(fence, fenceIdx));
		fenceValue++;
		return fenceIdx;
	}


	void RenderDevice::WaitForFence(uint64 fenceId)
	{
		if (fence->GetCompletedValue() < fenceId)
		{
			AssertHr(fence->SetEventOnCompletion(fenceId, fenceEvent));
			WaitForSingleObject(fenceEvent, INFINITE);
		}
	}



	SwapChainPtr RenderDevice::CreateSwapChain(const SwapChainDesc& _swapChainDesc)
	{
		SwapChainPtr swapChain = std::make_shared<SwapChain>();

		Pointer<IDXGISwapChain1> _swapChain;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = frameCount;
		swapChainDesc.Width = _swapChainDesc.width;
		swapChainDesc.Height = _swapChainDesc.height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		AssertHr(factory->CreateSwapChainForHwnd(
			commandQueue.Get(),
			HWND(_swapChainDesc.windowHandle),
			&swapChainDesc,
			nullptr,
			nullptr,
			&_swapChain.Get()));

		AssertHr(factory->MakeWindowAssociation(
			HWND(_swapChainDesc.windowHandle),
			DXGI_MWA_NO_ALT_ENTER));

		swapChain->swapChain = QueryInterface<IDXGISwapChain3>(_swapChain);
		Assert(swapChain->swapChain);
		swapChain->frameIndex = swapChain->swapChain->GetCurrentBackBufferIndex();

		return swapChain;
	}



	void SwapChain::Present(bool vsync)
	{
	}


#endif



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
	case DXGI_FORMAT_R16G16B16A16_SNORM:
		return DXGI_FORMAT_R16G16B16A16_TYPELESS;

	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return DXGI_FORMAT_R32G32B32A32_TYPELESS;

	case DXGI_FORMAT_R11G11B10_FLOAT:
		return DXGI_FORMAT_R11G11B10_FLOAT;

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



DXGI_FORMAT GetUAVFormat(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_UNKNOWN:
	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
		return format;
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM;	// apperently cannot use SRGB on UAVs on Dx12. Go figure.
	default:
		__debugbreak();
	}
}



// TODO : should not have swap chain desc as input here
RenderDevicePtr RenderDevice::Create(const RenderDeviceDesc& desc)
{
	RenderDevicePtr renderDevice = make_shared<RenderDevice>();

	// find adapter and create device		
	UINT dxgiFactoryFlags = 0;
	if (desc.enableValidation && D3D12GetDebugInterface(IID_PPV_ARGS(&renderDevice->debugController.Get())) == S_OK)
	{
		renderDevice->debugController->EnableDebugLayer();
		dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	}
	AssertHr(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&renderDevice->dxgiFactory.Get())));

	SIZE_T MaxSize = 0;

	for (uint32_t idx = 0; DXGI_ERROR_NOT_FOUND != renderDevice->dxgiFactory->EnumAdapters1(idx, &renderDevice->dxgiAdapter.Get()); ++idx)
	{
		DXGI_ADAPTER_DESC1 desc;
		renderDevice->dxgiAdapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;
		if (desc.DedicatedVideoMemory > MaxSize && D3D12CreateDevice(renderDevice->dxgiAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&renderDevice->device.Get())) == S_OK)
			break;
	}
	Assert(renderDevice->device != nullptr);

	// check features
	D3D12_FEATURE_DATA_D3D12_OPTIONS featureData = {};
	D3D12_FEATURE_DATA_D3D12_OPTIONS1 featureData1 = {};
	D3D12_FEATURE_DATA_D3D12_OPTIONS2 featureData2 = {};
	D3D12_FEATURE_DATA_D3D12_OPTIONS3 featureData3 = {};
	D3D12_FEATURE_DATA_D3D12_OPTIONS4 featureData4 = {};
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureData5 = {};
	renderDevice->device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureData, sizeof(featureData));
	renderDevice->device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &featureData1, sizeof(featureData1));
	renderDevice->device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS2, &featureData2, sizeof(featureData2));
	renderDevice->device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS3, &featureData3, sizeof(featureData3));
	renderDevice->device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS4, &featureData4, sizeof(featureData4));
	renderDevice->device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureData5, sizeof(featureData5));
	AssertHr(renderDevice->device->QueryInterface(IID_PPV_ARGS(&renderDevice->device5.Get())));

	// create command queue
	Pointer<ID3D12CommandQueue> commandQueue;
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	AssertHr(renderDevice->device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&renderDevice->commandQueue.Get())));

	// create descriptor heaps
	renderDevice->rtvHeap = make_unique<DescriptorHeap>(
		renderDevice->device,
		D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 256);
	renderDevice->dsvHeap = make_unique<DescriptorHeap>(
		renderDevice->device,
		D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 256);
	renderDevice->viewHeap = make_unique<DescriptorHeap>(
		renderDevice->device,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 256);
	renderDevice->viewTableHeap = make_unique<DescriptorHeap>(
		renderDevice->device,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1024 * 24);
	renderDevice->samplerHeap = make_unique<DescriptorHeap>(
		renderDevice->device,
		D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER,
		D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 256);
	renderDevice->uploadScratch = make_unique<ScratchAllocator>(
		renderDevice->device,
		D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ);
	renderDevice->uavScratch = make_unique<ScratchAllocator>(
		renderDevice->device,
		D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// Create the submit command list.
	AssertHr(renderDevice->device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(&renderDevice->submitCLAlloc.Get())));

	AssertHr(renderDevice->device->CreateCommandList(
		0, D3D12_COMMAND_LIST_TYPE_DIRECT,
		renderDevice->submitCLAlloc, nullptr,
		IID_PPV_ARGS(&renderDevice->submitCL.Get())));
	renderDevice->submitCL->Close();

	return renderDevice;
}



SwapChainPtr RenderDevice::CreateSwapChain(const SwapChainDesc& swapChainDesc)
{
	return make_shared<SwapChain>(
		swapChainDesc, device, dxgiFactory, rtvHeap.get(),
		this, commandQueue);
}



RenderFence RenderDevice::CreateFence()
{
	RenderFence fence;
	AssertHr(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence.fence.Get())));
	fence.value = 1;
	fence.event = CreateEvent(nullptr, false, false, nullptr);
	return fence;
}



GPUBufferPtr RenderDevice::CreateBuffer(const GPUBufferDesc& desc)
{
	// TODO : add optional fast clear feature and disable EXECUTION WARNING #820: CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE goofy warning

	GPUBufferPtr buffer = make_shared<GPUBuffer>();
	buffer->desc = desc;

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
		IID_PPV_ARGS(&buffer->resource[0].resource.Get())));
	buffer->resource[0].resource->SetName(ToWString(desc.name.c_str()).c_str());
	buffer->resource[0].state = state;
	buffer->resource[1] = buffer->resource[0];
	if (hasGpuHandle)
		buffer->gpuHandle = buffer->resource[0].resource->GetGPUVirtualAddress();

	switch (desc.type)
	{
	case GPUBufferType::VertexBuffer:
		buffer->vbvs.BufferLocation = buffer->resource[0].resource->GetGPUVirtualAddress();
		buffer->vbvs.StrideInBytes = desc.dataStride;
		buffer->vbvs.SizeInBytes = desc.dataSize;
		break;
	case GPUBufferType::IndexBuffer:
		buffer->ibv.BufferLocation = buffer->resource[0].resource->GetGPUVirtualAddress();
		buffer->ibv.Format = desc.format;
		buffer->ibv.SizeInBytes = desc.dataSize;
		break;
	case GPUBufferType::ConstantBuffer:
	{
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = buffer->resource[0].resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = bufferDesc.Width;
		buffer->cbv = viewHeap->Alloc();
		device->CreateConstantBufferView(&cbvDesc, buffer->cbv.cpu);
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

		buffer->dsv = dsvHeap->Alloc();
		device->CreateDepthStencilView(buffer->resource[0].resource, &viewDesc, buffer->dsv.cpu);
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
		buffer->srv = viewHeap->Alloc();
		device->CreateShaderResourceView(buffer->resource[0].resource, &viewDesc, buffer->srv.cpu);
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
		buffer->uav = viewHeap->Alloc();
		device->CreateUnorderedAccessView(buffer->resource[0].resource, nullptr, &viewDesc, buffer->uav.cpu);
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

		buffer->rtvs[0] = rtvHeap->Alloc();
		device->CreateRenderTargetView(buffer->resource[0].resource, &viewDesc, buffer->rtvs[0].cpu);
		buffer->rtvs[1] = buffer->rtvs[0];
	}

	if ((desc.usage & GPUUsage_Dynamic) != 0)
	{
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(buffer->resource[0].resource, 0, 1);
		AssertHr(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&buffer->copyBuffer.Get())));
	}

	if (desc.dataPtr)
	{
		if (!submitingResources)
		{
			submitCLAlloc->Reset();
			submitCL->Reset(submitCLAlloc, nullptr);
			commitedCLs.insert(commitedCLs.begin(), submitCL);
		}
		submitingResources = true;

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
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(buffer->resource[0].resource, firstResource, numResources);

		PD3D12Resource transient;
		AssertHr(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&transient.Get())));

		submitCL->ResourceBarrier(
			1, &CD3DX12_RESOURCE_BARRIER::Transition(buffer->resource[0].resource,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				D3D12_RESOURCE_STATE_COPY_DEST));
		buffer->resource[0].state = D3D12_RESOURCE_STATE_COPY_DEST;
		UpdateSubresources(submitCL, buffer->resource[0].resource, transient, 0, firstResource, numResources, data.data());

		pendingResources.push_back(transient);
	}

	return buffer;
}



RenderPassPtr RenderDevice::CreateRenderPass(const RenderPassDesc& desc, const char* name)
{
	RenderPassPtr renderPass = make_shared<RenderPass>();
	renderPass->name = name;
	renderPass->desc = desc;

	for (uint frame = 0; frame < frameCount; ++frame)
	{
		for (uint i = 0; i < desc.rtvs.size(); ++i)
			renderPass->rtvs[frame].push_back(desc.rtvs[i]->rtvs[frame].cpu);
	}
	if (desc.dsv)
		renderPass->dsv = desc.dsv->dsv.cpu;

	renderPass->scissor.left = renderPass->scissor.right = 0;
	renderPass->scissor.right = desc.viewport.Width;
	renderPass->scissor.bottom = desc.viewport.Height;

	return renderPass;
}



GraphicsPSOPtr RenderDevice::CreateGraphicsPSO(const GraphicsPSODesc& desc)
{
	GraphicsPSOPtr pso = make_shared<GraphicsPSO>();

	CD3DX12_ROOT_PARAMETER rootParameters[3];
	rootParameters[0].InitAsDescriptorTable(1, &CD3DX12_DESCRIPTOR_RANGE{ D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0 });
	rootParameters[1].InitAsDescriptorTable(1, &CD3DX12_DESCRIPTOR_RANGE{ D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0 });
	rootParameters[2].InitAsDescriptorTable(1, &CD3DX12_DESCRIPTOR_RANGE{ D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 1, 0 });
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(_countof(rootParameters), rootParameters);
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	Pointer<ID3DBlob> signature;
	Pointer<ID3DBlob> error;
	AssertHr(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature.Get(), &error.Get()));
	if (error)
		cout << (const char*)error->GetBufferPointer() << endl;
	AssertHr(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&pso->rootSignature.Get())));

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout.pInputElementDescs = desc.geometryDesc.data();
	psoDesc.InputLayout.NumElements = desc.geometryDesc.size();
	psoDesc.pRootSignature = pso->rootSignature;
	psoDesc.VS.pShaderBytecode = desc.vertexShader->GetBufferPointer();
	psoDesc.VS.BytecodeLength = desc.vertexShader->GetBufferSize();
	psoDesc.PS.pShaderBytecode = desc.pixelShader->GetBufferPointer();
	psoDesc.PS.BytecodeLength = desc.pixelShader->GetBufferSize();
	psoDesc.RasterizerState = desc.rasterizer;
	psoDesc.BlendState = desc.blend;
	psoDesc.DepthStencilState = desc.depthStencil;
	psoDesc.SampleMask = desc.sampleMask;
	psoDesc.PrimitiveTopologyType = desc.topologyType;
	psoDesc.NumRenderTargets = desc.renderPass->rtvs->size();
	psoDesc.SampleDesc = desc.sampleDesc;
	for (uint i = 0; i < psoDesc.NumRenderTargets; ++i)
		psoDesc.RTVFormats[i] = desc.renderPass->desc.rtvs[i]->desc.format;
	psoDesc.DSVFormat = desc.renderPass->desc.dsv->desc.format;
	AssertHr(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso->pipelineState.Get())));

	return pso;
}



ComputePtr RenderDevice::CreateCompute(const ComputeDesc& desc)
{
	ComputePtr compute = make_shared<Compute>();
	compute->rl = desc.resourceLayout;
	PrepareResourceLayout(compute->rl);

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc{};
	psoDesc.pRootSignature = compute->rl->rootSignature;
	psoDesc.CS.pShaderBytecode = desc.shader->GetBufferPointer();
	psoDesc.CS.BytecodeLength = desc.shader->GetBufferSize();
	device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&compute->pso.Get()));

	return compute;
}



ResourceSetPtr RenderDevice::CreateResourceSet()
{
	return make_shared<ResourceSet>();
}



ResourceLayoutPtr RenderDevice::CreateResourceLayout()
{
	return make_shared<ResourceLayout>();
}



DescriptorHandle RenderDevice::CreateSampler(const D3D12_SAMPLER_DESC& desc)
{
	DescriptorHandle sampler = samplerHeap->Alloc();
	device->CreateSampler(&desc, sampler.cpu);
	return sampler;
}



void RenderDevice::PrepareResourceSet(ResourceSetPtr rs)
{
	// TODO : properly design this thing
	if (!rs->dirty)
		return;
	if (rs->cb || rs->srv || rs->uav)
	{
		DescriptorHandle descTable = viewTableHeap->Alloc();
		rs->descTable = descTable;
		if (rs->cb)
		{
			device->CopyDescriptorsSimple(
				1, descTable.cpu, rs->cb->cbv.cpu,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		if (rs->srv)
		{
			device->CopyDescriptorsSimple(
				1, descTable.cpu, rs->srv->srv.cpu,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		if (rs->uav)
		{
			device->CopyDescriptorsSimple(
				1, descTable.cpu, rs->uav->uav.cpu,
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}
		rs->dirty = false;
	}
	else
	{
		Assert(!rs->entries.empty());	// no resources?
		uint tableSize = 0;
		for (uint i = 0; i < rs->ranges.size(); ++i)
			tableSize += rs->ranges[i].NumDescriptors;

		rs->descTable = viewTableHeap->Alloc(tableSize);

		vector<D3D12_CPU_DESCRIPTOR_HANDLE> destHandles;
		vector<uint> destRangeSz;
		vector<D3D12_CPU_DESCRIPTOR_HANDLE> srcHandles;
		vector<uint> srcRangeSz;
		destHandles.reserve(rs->entries.size());
		destRangeSz.resize(rs->entries.size(), 1);
		srcHandles.reserve(rs->entries.size());
		srcRangeSz.resize(rs->entries.size(), 1);
		for (const auto& entry : rs->entries)
		{
			const D3D12_DESCRIPTOR_RANGE& range = rs->ranges[uint(entry.type)];
			D3D12_CPU_DESCRIPTOR_HANDLE dest = rs->descTable.cpu;
			dest.ptr += (range.OffsetInDescriptorsFromTableStart + entry.index - range.BaseShaderRegister) * viewTableHeap->descriptorSize;
			destHandles.push_back(dest);
			srcHandles.push_back(entry.cpu);
		}
		device->CopyDescriptors(
			destHandles.size(), destHandles.data(), destRangeSz.data(),
			srcHandles.size(), srcHandles.data(), srcRangeSz.data(),
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		rs->dirty = false;
	}
}



void RenderDevice::PrepareResourceLayout(ResourceLayoutPtr rl)
{
	if (!rl->dirty)
		return;

	for (uint i = 0; i < rl->sets.size(); ++i)
		PrepareResourceSet(rl->sets[i]);

	D3D12_ROOT_SIGNATURE_DESC desc{};
	desc.NumParameters = rl->rootParams.size();
	desc.pParameters = rl->rootParams.data();

	PD3D12RootSignature rootSig;
	Pointer<ID3DBlob> blob;
	Pointer<ID3DBlob> error;

	AssertHr(D3D12SerializeRootSignature(
		&desc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&blob.Get(), &error.Get()));

	AssertHr(device->CreateRootSignature(
		1,
		blob->GetBufferPointer(),
		blob->GetBufferSize(),
		IID_PPV_ARGS(&rl->rootSignature.Get())));

	rl->dirty = false;
}



void RenderDevice::BeginRender(SwapChainPtr swapChain)
{
	Assert(curSwapChain == nullptr || swapChain == curSwapChain);
	curSwapChain = swapChain;
}



RenderContextPtr RenderDevice::BeginRenderContext()
{
	Assert(curSwapChain != nullptr);

	RenderContextPtr renderContext;

	if (freeContexts.empty())
	{
		PD3D12CommandAllocator    commandAllocator;
		PD3D12GraphicsCommandList commandList;

		AssertHr(device->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&commandAllocator.Get())));

		AssertHr(device->CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_DIRECT,
			commandAllocator, nullptr,
			IID_PPV_ARGS(&commandList.Get())));

		renderContext = make_shared<RenderContext>();
		renderContext->commandAllocator = commandAllocator;
		renderContext->commandList = commandList;
	}
	else
	{
		renderContext = freeContexts.back();
		freeContexts.pop_back();
		renderContext->commandAllocator->Reset();
		renderContext->commandList->Reset(renderContext->commandAllocator, nullptr);
	}

	ID3D12DescriptorHeap* heaps[] = { viewTableHeap->GetHeap(), samplerHeap->GetHeap() };
	renderContext->commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	renderContext->frameIndex = curSwapChain->GetFrameIndex();

	commitedContexts.push_back(renderContext);
	commitedCLs.push_back(renderContext->commandList);

	return renderContext;
}



void RenderDevice::Present()
{
	Assert(curSwapChain != nullptr);

	auto finalRC = BeginRenderContext();
	finalRC->AddResourceBarrier(curSwapChain->GetBackBuffer(), D3D12_RESOURCE_STATE_PRESENT);
	finalRC->FlushResourceBarriers();

	if (submitingResources)
		submitCL->Close();

	for (auto& renderContext : commitedContexts)
	{
		renderContext->commandList->Close();
		freeContexts.push_back(renderContext);
	}
	commandQueue->ExecuteCommandLists(commitedCLs.size(), commitedCLs.data());
	commitedContexts.clear();
	commitedCLs.clear();
	submitingResources = false;

	curSwapChain->Present(this);

	pendingResources.clear();
	uploadScratch->Reset();
	uavScratch->Reset();
	curSwapChain.reset();
}



void RenderDevice::SetFence(RenderFence& fence)
{
	fence.waitValue = fence.value;
	AssertHr(commandQueue->Signal(fence.fence, fence.waitValue));
	fence.value++;
}



void RenderDevice::WaitForFence(RenderFence& fence)
{
	if (fence.fence->GetCompletedValue() < fence.waitValue)
	{
		AssertHr(fence.fence->SetEventOnCompletion(fence.waitValue, fence.event));
		WaitForSingleObject(fence.event, INFINITE);
	}
}



SwapChain::SwapChain(const SwapChainDesc& swapChainDesc, ID3D12Device* device, IDXGIFactory4* dxgiFactory, DescriptorHeap* rtvHeap, RenderDevice* renderDevice, ID3D12CommandQueue* commandQueue)
{
	DXGI_SWAP_CHAIN_DESC1 dxSwapChainDesc{};
	dxSwapChainDesc.BufferCount = frameCount;
	dxSwapChainDesc.Width = swapChainDesc.width;
	dxSwapChainDesc.Height = swapChainDesc.height;
	dxSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxSwapChainDesc.SampleDesc.Count = 1;

	Pointer<IDXGISwapChain1> swapChain;
	AssertHr(dxgiFactory->CreateSwapChainForHwnd(
		commandQueue,        // Swap chain needs the queue so that it can force a flush on it.
		HWND(swapChainDesc.windowHandle),
		&dxSwapChainDesc,
		nullptr,
		nullptr,
		&swapChain.Get()));
	AssertHr(dxgiFactory->MakeWindowAssociation(HWND(swapChainDesc.windowHandle), DXGI_MWA_NO_ALT_ENTER));
	swapChain3 = QueryInterface<IDXGISwapChain3>(swapChain);

	frameBuffer = make_shared<GPUBuffer>();
	for (uint n = 0; n < frameCount; ++n)
	{
		DescriptorHandle rtvHandle = rtvHeap->Alloc();
		AssertHr(swapChain3->GetBuffer(n, IID_PPV_ARGS(&frameBuffer->resource[n].resource.Get())));
		device->CreateRenderTargetView(frameBuffer->resource[n].resource, nullptr, rtvHandle.cpu);
		frameBuffer->rtvs[n] = rtvHandle;
	}
	frameBuffer->desc.type = GPUBufferType::Texture2D;
	frameBuffer->desc.usage = GPUUsage_ShaderResource | GPUUsage_RenderTarget;
	frameBuffer->desc.format = dxSwapChainDesc.Format;
	frameBuffer->frameIndex = swapChain3->GetCurrentBackBufferIndex();

	frameFence = renderDevice->CreateFence();
}



void SwapChain::Present(RenderDevice* renderDevice)
{
	// Present the frame.
	AssertHr(swapChain3->Present(1, 0));
	renderDevice->SetFence(frameFence);

	// Wait for frame
	renderDevice->WaitForFence(frameFence);
	frameBuffer->frameIndex = swapChain3->GetCurrentBackBufferIndex();
}



}
