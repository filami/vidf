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

	renderDevice->infoQeue = QueryInterface<ID3D12InfoQueue>(renderDevice->device);
	if (renderDevice->infoQeue)
	{
		D3D12_MESSAGE_ID disable[] =
		{
			// disabling missing fast clear mismatch, render context and GPUBuffer now has
			// fast clear as an option
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
		};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(disable);
		filter.DenyList.pIDList = disable;
		renderDevice->infoQeue->AddStorageFilterEntries(&filter);
	}

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
	if (desc.dataPtr != nullptr && !submitingResources)
	{
		submitCLAlloc->Reset();
		submitCL->Reset(submitCLAlloc, nullptr);
		commitedCLs.insert(commitedCLs.begin(), submitCL);
		submitingResources = true;
	}
	return make_shared<GPUBuffer>(this, submitCL, desc);
}



RenderPassPtr RenderDevice::CreateRenderPass(const RenderPassDesc& desc, const char* name)
{
	RenderPassPtr renderPass = make_shared<RenderPass>();
	renderPass->name = name;
	renderPass->desc = desc;

	for (uint i = 0; i < desc.rtvs.size(); ++i)
		renderPass->rtvs.push_back(desc.rtvs[i]->rtv.cpu);
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
	psoDesc.NumRenderTargets = desc.renderPass->rtvs.size();
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
		renderContext->renderDevice = shared_from_this();
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
	frameIndex = swapChain3->GetCurrentBackBufferIndex();
		
	for (uint n = 0; n < frameCount; ++n)
	{
		frameBuffers[n] = make_shared<GPUBuffer>();
		frameBuffers[n]->desc.type = GPUBufferType::Texture2D;
		frameBuffers[n]->desc.usage = GPUUsage_ShaderResource | GPUUsage_RenderTarget;
		frameBuffers[n]->desc.format = dxSwapChainDesc.Format;

		DescriptorHandle rtvHandle = rtvHeap->Alloc();
		AssertHr(swapChain3->GetBuffer(n, IID_PPV_ARGS(&frameBuffers[n]->resource.Get())));
		device->CreateRenderTargetView(frameBuffers[n]->resource, nullptr, rtvHandle.cpu);
		frameBuffers[n]->state = D3D12_RESOURCE_STATE_PRESENT;
		frameBuffers[n]->rtv = rtvHandle;
	}

	frameFence = renderDevice->CreateFence();
}



void SwapChain::Present(RenderDevice* renderDevice)
{
	// Present the frame.
	AssertHr(swapChain3->Present(1, 0));
	renderDevice->SetFence(frameFence);

	// Wait for frame
	renderDevice->WaitForFence(frameFence);
	frameIndex = swapChain3->GetCurrentBackBufferIndex();
}



}
