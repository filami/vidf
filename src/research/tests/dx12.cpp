#include "pch.h"

#if 0

#include <d3d12.h>
#include <dxgi1_4.h>
#include <D3Dcompiler.h>

#include "vidf/common/pointer.h"
#include "vidf/platform/canvas.h"
#include "rendererdx12/d3dx12.h"
#include "rendererdx12/renderdevice.h"
#include "rendererdx12/resources.h"
#include "rendererdx12/pipeline.h"

using namespace vidf;
using namespace dx12;



class Dx12CanvasListener : public CanvasListener
{
public:
	virtual void Close()
	{
		PostQuitMessage();
	}
	virtual void KeyDown(KeyCode keyCode)
	{
		if (keyCode == KeyCode::Escape)
			PostQuitMessage();
	}
};



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



void _TestDx12()
{
	const uint width = 1280;
	const uint height = 720;

	Dx12CanvasListener canvasListener;

	CanvasDesc canvasDesc{};
	canvasDesc.width = width;
	canvasDesc.height = height;
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	Assert(canvas);
	canvas->AddListener(&canvasListener);

	/////////////////////////////////////////////////////

	const uint frameCount = 2;
	uint dxgiFactoryFlags = 0;
	Pointer<IDXGIFactory4> factory;
	Pointer<IDXGIAdapter1> hardwareAdapter;
	Pointer<ID3D12Device> device;
	Pointer<ID3D12CommandQueue> commandQueue;
	Pointer<IDXGISwapChain3> swapChain;
	Pointer<ID3D12CommandAllocator> commandAllocator;
	Pointer<ID3D12GraphicsCommandList> commandList;
	Pointer<ID3D12DescriptorHeap> rtvHeap;
	uint rtvDescriptorSize;
	uint frameIndex = 0;

	Pointer<ID3D12Fence> fence;
	HANDLE fenceEvent;
	uint64 fenceValue;

	Pointer<ID3D12Resource> renderTargets[frameCount];

	Pointer<ID3D12RootSignature> rootSignature;
	Pointer<ID3D12PipelineState> pipelineState;
	Pointer<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	/////////////////////////////////////////////////////

	{
		// Device
		{
			Pointer<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController.Get()))))
			{
				debugController->EnableDebugLayer();
				dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}

		AssertHr(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory.Get())));

		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		AssertHr(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device.Get())));

		// command queue
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		AssertHr(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue.Get())));

		// swap chain
		Pointer<IDXGISwapChain1> _swapChain;
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = frameCount;
		swapChainDesc.Width = width;
		swapChainDesc.Height = height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;
		AssertHr(factory->CreateSwapChainForHwnd(
			commandQueue.Get(),
			HWND(canvas->GetHandle()),
			&swapChainDesc,
			nullptr,
			nullptr,
			&_swapChain.Get()));
		AssertHr(factory->MakeWindowAssociation(HWND(canvas->GetHandle()), DXGI_MWA_NO_ALT_ENTER));
		swapChain = QueryInterface<IDXGISwapChain3>(_swapChain);
		Assert(swapChain);
		frameIndex = swapChain->GetCurrentBackBufferIndex();

		// rtv heap
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = frameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		AssertHr(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap.Get())));
		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		// back buffer rtv
		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT n = 0; n < frameCount; n++)
		{
			AssertHr(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n].Get())));
			device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
		}

		// graphics command list
		AssertHr(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator.Get())));
		AssertHr(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, nullptr, IID_PPV_ARGS(&commandList.Get())));
		
		// frame fence
		AssertHr(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence.Get())));
		fenceValue = 1;
		fenceEvent = CreateEvent(nullptr, false, false, nullptr);
		Assert(fenceEvent);

		// root signature
		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
		rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		Pointer<ID3DBlob> signature;
		Pointer<ID3DBlob> error;
		AssertHr(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature.Get(), &error.Get()));
		AssertHr(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature.Get())));

		// shaders
		Pointer<ID3DBlob> vertexShader;
		Pointer<ID3DBlob> pixelShader;
		uint compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
		AssertHr(D3DCompileFromFile(L"data/shaders/dx12shader.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader.Get(), nullptr));
		AssertHr(D3DCompileFromFile(L"data/shaders/dx12shader.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader.Get(), nullptr));

		// PSO
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = rootSignature.Get();
		psoDesc.VS.pShaderBytecode = vertexShader->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShader->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShader->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShader->GetBufferSize();
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.FrontCounterClockwise = false;
		psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		psoDesc.RasterizerState.DepthClipEnable = true;
		psoDesc.RasterizerState.MultisampleEnable = false;
		psoDesc.RasterizerState.AntialiasedLineEnable = false;
		psoDesc.RasterizerState.ForcedSampleCount = 0;
		psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		psoDesc.BlendState.AlphaToCoverageEnable = false;
		psoDesc.BlendState.IndependentBlendEnable = false;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
		{
			false, false,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
			D3D12_LOGIC_OP_NOOP,
			D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			psoDesc.BlendState.RenderTarget[i] = defaultRenderTargetBlendDesc;
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		AssertHr(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState.Get())));

		// vertex buffer
		const float aspectRatio = width / float(height);
		struct Vertex
		{
			Vector3f position;
			Vector4f color;
		};
		Vertex triangleVertices[] =
		{
			{ Vector3f(0.0f, 0.25f * aspectRatio, 0.0f),    Vector4f(1.0f, 0.0f, 0.0f, 1.0f) },
			{ Vector3f(0.25f, -0.25f * aspectRatio, 0.0f),  Vector4f(0.0f, 1.0f, 0.0f, 1.0f) },
			{ Vector3f(-0.25f, -0.25f * aspectRatio, 0.0f), Vector4f(0.0f, 0.0f, 1.0f, 1.0f) },
		};
		const uint vertexBufferSize = sizeof(triangleVertices);

		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = 0;
		resourceDesc.Width = vertexBufferSize;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
				
		AssertHr(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&vertexBuffer.Get())));

		const uint64 uploadBufferSize = GetRequiredIntermediateSize(vertexBuffer.Get(), 0, 1);

		Pointer<ID3D12Resource> uploadHeap;
		AssertHr(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadHeap.Get())));

		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = triangleVertices;
		textureData.RowPitch = vertexBufferSize;
		textureData.SlicePitch = vertexBufferSize;

		UpdateSubresources(commandList, vertexBuffer, uploadHeap, 0, 0, 1, &textureData);
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(vertexBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(Vertex);
		vertexBufferView.SizeInBytes = vertexBufferSize;

		// Close the command list and execute it to begin the initial GPU setup.
		AssertHr(commandList->Close());
		ID3D12CommandList* commandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		const uint64 fenceIdx = fenceValue;
		AssertHr(commandQueue->Signal(fence, fenceIdx));
		fenceValue++;
		if (fence->GetCompletedValue() < fenceIdx)
		{
			AssertHr(fence->SetEventOnCompletion(fenceIdx, fenceEvent));
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		frameIndex = swapChain->GetCurrentBackBufferIndex();
	}

	/////////////////////////////////////////////////////

	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		{
			AssertHr(commandAllocator->Reset());

			AssertHr(commandList->Reset(commandAllocator, nullptr));

			D3D12_VIEWPORT viewport = {};
			D3D12_RECT scissorRect = {};
			viewport.Width = width;
			viewport.Height = height;
			viewport.MaxDepth = 1.0f;
			scissorRect.right = width;
			scissorRect.bottom = height;
			commandList->RSSetViewports(1, &viewport);
			commandList->RSSetScissorRects(1, &scissorRect);

			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
			rtvHandle.ptr += rtvDescriptorSize * frameIndex;

			commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

			const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
			commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

			commandList->SetGraphicsRootSignature(rootSignature);
			commandList->SetPipelineState(pipelineState);
			commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
			commandList->DrawInstanced(3, 1, 0, 0);

			commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

			AssertHr(commandList->Close());
		}

		ID3D12CommandList* commandLists[] = { commandList };
		commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
		AssertHr(swapChain->Present(1, 0));

		const uint64 fenceIdx = fenceValue;
		AssertHr(commandQueue->Signal(fence, fenceIdx));
		fenceValue++;
		if (fence->GetCompletedValue() < fenceIdx)
		{
			AssertHr(fence->SetEventOnCompletion(fenceIdx, fenceEvent));
			WaitForSingleObject(fenceEvent, INFINITE);
		}
		frameIndex = swapChain->GetCurrentBackBufferIndex();
	}

	CloseHandle(fenceEvent);
}



void TestDx12()
{
	const uint width = 1280;
	const uint height = 720;

	Dx12CanvasListener canvasListener;

	CanvasDesc canvasDesc{};
	canvasDesc.width = width;
	canvasDesc.height = height;
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	Assert(canvas);
	canvas->AddListener(&canvasListener);

	RenderDeviceDesc renderDeviceDesc;
	renderDeviceDesc.enableValidation = true;
	RenderDevicePtr renderDevice = RenderDevice::Create(renderDeviceDesc);
	if (!renderDevice)
		return;
	CommandBuffer commandBuffer{ renderDevice };

	SwapChainDesc swapChainDesc{};
	swapChainDesc.width = canvasDesc.width;
	swapChainDesc.height = canvasDesc.height;
	swapChainDesc.windowHandle = canvas->GetHandle();
	SwapChainPtr swapChain = renderDevice->CreateSwapChain(swapChainDesc);
	if (!swapChain)
		return;

	Viewport viewport{width, height};

	RenderPassDesc renderPassDesc;
	renderPassDesc.viewport = viewport;
	renderPassDesc.rtvs[0] = swapChain->GetBackBuffer();
	RenderPassPtr renderPass = RenderPass::Create(renderDevice, renderPassDesc);

	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		commandBuffer.Begin();
		commandBuffer.End();
		swapChain->Present();
	}
}

#endif
