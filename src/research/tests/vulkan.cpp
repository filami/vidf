#include "pch.h"
#include "vidf/renderer/renderdevice.h"
#include "vidf/renderer/rendercontext.h"
#include "vidf/renderer/swapchain.h"
#include "vidf/renderer/renderpass.h"


using namespace vidf;


class VulkanCanvasListener : public CanvasListener
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


VkShaderModule CompileHLSL(const char* path, const char* entryPoint, VkShaderStageFlagBits stage)
{
	VkShaderModule module{};

	const char* compilerPath = "ext/bin/glslangValidator.exe ";
	std::string outputName = std::string(path) + "_" + entryPoint + ".spv";
	std::string cmdLine = compilerPath;

	cmdLine += "-D -V100 ";
	switch (stage)
	{
	case VK_SHADER_STAGE_VERTEX_BIT: cmdLine += "-S vert "; break;
	case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT: cmdLine += "-S tesc "; break;
	case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT: cmdLine += "-S tese "; break;
	case VK_SHADER_STAGE_GEOMETRY_BIT: cmdLine += "-S geom "; break;
	case VK_SHADER_STAGE_FRAGMENT_BIT: cmdLine += "-S frag "; break;
	case VK_SHADER_STAGE_COMPUTE_BIT: cmdLine += "-S comp "; break;
	default:
		break;
	}
	cmdLine += std::string("-e --source-entrypoint ") + entryPoint + " ";
	cmdLine += std::string("-o ") + outputName + " ";
	cmdLine += path;

	STARTUPINFOA si{};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi{};
	CreateProcessA(
		nullptr, const_cast<LPSTR>(cmdLine.c_str()), nullptr, nullptr, false,
		0, nullptr, nullptr, &si, &pi);

	return module;
}


class SimplePass : public BaseRenderPass
{
public:
	SimplePass() : BaseRenderPass("SimplePass") {}
	bool Prepare(RenderDevicePtr device, SwapChainPtr swapChain);
	void Render(RenderContextPtr context, SwapChainPtr swapChain);
};



bool SimplePass::Prepare(RenderDevicePtr device, SwapChainPtr swapChain)
{
	VkClearValue clearValues;
	clearValues.color = { { 0.5f, 0.5f, 0.5f, 0.0f } };
	// clearValues[1].depthStencil = { 1.0f, 0 };
	AppendSwapChain(swapChain, &clearValues);

	if (!Cook(device, swapChain->GetExtent()))
		return false;

	///

	VkPipelineLayoutCreateInfo pipelineLayoutDesc = {};
	pipelineLayoutDesc.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

	VkPipelineLayout pipelineLayout{};
	vkCreatePipelineLayout(device->GetDevice(), &pipelineLayoutDesc, nullptr, &pipelineLayout);

	///

	struct Vertex
	{
		Vector3f position;
		uint32 color;
	};

	VkVertexInputBindingDescription vertexInputBinding{};
	vertexInputBinding.binding = 0;
	vertexInputBinding.stride = sizeof(Vertex);
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::vector<VkVertexInputAttributeDescription> vertexInputAttrs(2);
	vertexInputAttrs[0].location = 0;
	vertexInputAttrs[0].binding = 0;
	vertexInputAttrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttrs[0].offset = offsetof(Vertex, position);
	vertexInputAttrs[1].location = 1;
	vertexInputAttrs[1].binding = 0;
	vertexInputAttrs[1].format = VK_FORMAT_R8G8B8A8_UNORM;
	vertexInputAttrs[1].offset = offsetof(Vertex, color);

	VkPipelineVertexInputStateCreateInfo vertexInputState{};
	vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttrs.size());
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttrs.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
	inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyState.primitiveRestartEnable = false;

	VkPipelineRasterizationStateCreateInfo defaultRaster{};
	defaultRaster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	defaultRaster.depthClampEnable = true;
	defaultRaster.rasterizerDiscardEnable = false;
	defaultRaster.polygonMode = VK_POLYGON_MODE_FILL;
	defaultRaster.cullMode = VK_CULL_MODE_NONE;
	defaultRaster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	defaultRaster.depthBiasEnable = false;
	defaultRaster.lineWidth = 1.0f;

	std::vector<VkPipelineShaderStageCreateInfo> stageInfo(2);
	stageInfo[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	stageInfo[0].module = CompileHLSL("data/shaders/shader.hlsl", "vsMain", VK_SHADER_STAGE_VERTEX_BIT);
	stageInfo[0].pName = "vsMain";
	stageInfo[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageInfo[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	stageInfo[1].module = CompileHLSL("data/shaders/shader.hlsl", "psMain", VK_SHADER_STAGE_FRAGMENT_BIT);
	stageInfo[1].pName = "psMain";

	VkGraphicsPipelineCreateInfo psoDesc{};
	psoDesc.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	psoDesc.renderPass = GetRenderPass();
	psoDesc.pVertexInputState = &vertexInputState;
	psoDesc.pInputAssemblyState = &inputAssemblyState;
	psoDesc.pRasterizationState = &defaultRaster;
	psoDesc.layout = pipelineLayout;
	psoDesc.stageCount = uint32_t(stageInfo.size());
	psoDesc.pStages = stageInfo.data();

	VkPipeline pipeline;
	vkCreateGraphicsPipelines(
		device->GetDevice(), nullptr, 1,
		&psoDesc, nullptr, &pipeline);
}



void SimplePass::Render(RenderContextPtr context, SwapChainPtr swapChain)
{
	Begin(context, swapChain);
	End(context);
}




#include <d3d11_3.h>
#include <D3Dcompiler.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment( lib, "dxguid.lib")


typedef Pointer<IDXGIDevice>               PDXGIDevice;
typedef Pointer<IDXGIAdapter>              PDXGIAdapter;
typedef Pointer<IDXGIFactory>              PDXGIFactory;
typedef Pointer<IDXGISwapChain>            PDXGISwapChain;
typedef Pointer<ID3D11Debug>               PD3D11Debug;
typedef Pointer<ID3D11Device>              PD3D11Device;
typedef Pointer<ID3D11Device3>             PD3D11Device3;
typedef Pointer<ID3D11DeviceContext>       PD3D11DeviceContext;
typedef Pointer<ID3DUserDefinedAnnotation> PD3DUserDefinedAnnotation;
typedef Pointer<ID3D11Texture2D>           PD3D11Texture2D;
typedef Pointer<ID3D11RenderTargetView>    PD3D11RenderTargetView;
typedef Pointer<ID3D11ShaderResourceView>  PD3D11ShaderResourceView;
typedef Pointer<ID3D11UnorderedAccessView> PD3D11UnorderedAccessView;
typedef Pointer<ID3D11Buffer>              PD3D11Buffer;
typedef Pointer<ID3D11VertexShader>        PD3D11VertexShader;
typedef Pointer<ID3D11PixelShader>         PD3D11PixelShader;
typedef Pointer<ID3D11InputLayout>         PD3D11InputLayout;
typedef Pointer<ID3DBlob>                  PD3DBlob;


template<typename OutInterface, typename InInterface>
Pointer<OutInterface> QueryInterface(Pointer<InInterface> inInterface)
{
	Pointer<OutInterface> outInterface;
	inInterface->QueryInterface(__uuidof(OutInterface), (void**)&outInterface.Get());
	return outInterface;
}



template<typename Object>
void NameObject(Pointer<Object> object, const char* name)
{
	object->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(name), name);
}



bool TestVulkan()
{
	{
		HRESULT hr;
		PD3D11Device device;
		PD3D11DeviceContext context;
		D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_1;
		hr = D3D11CreateDevice(
			nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
			D3D11_CREATE_DEVICE_DEBUG,
			&level, 1, D3D11_SDK_VERSION, &device.Get(), nullptr, &context.Get());
		NameObject(device, "device");
		NameObject(context, "context");

		PD3D11Debug deviceDebug = QueryInterface<ID3D11Debug>(device);
		PD3DUserDefinedAnnotation userAnnotations = QueryInterface<ID3DUserDefinedAnnotation>(context);

		PD3D11Device3 device3 = QueryInterface<ID3D11Device3>(device);

		PDXGIDevice dxgiDevice = QueryInterface<IDXGIDevice>(device);

		PDXGIAdapter dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter.Get());

		PDXGIFactory dxgiFactory;
		dxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&dxgiFactory.Get());

		VulkanCanvasListener canvasListener;

		CanvasDesc canvasDesc;
		CanvasPtr canvas = Canvas::Create(canvasDesc);
		canvas->AddListener(&canvasListener);
		if (!canvas)
			return false;

		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferCount = 2;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		swapChainDesc.BufferDesc.Width = canvasDesc.width;
		swapChainDesc.BufferDesc.Height = canvasDesc.height;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.OutputWindow = static_cast<HWND>(canvas->GetHandle());
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Windowed = true;
		PDXGISwapChain swapChain;
		dxgiFactory->CreateSwapChain(dxgiDevice, &swapChainDesc, &swapChain.Get());
		NameObject(dxgiFactory, "swapChain");

		PD3D11Texture2D backBuffer;
		swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer.Get());
		D3D11_RENDER_TARGET_VIEW_DESC backBufferRTVDesc{};
		backBufferRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		PD3D11RenderTargetView backBufferRTV;
		device->CreateRenderTargetView(backBuffer, &backBufferRTVDesc, &backBufferRTV.Get());
		NameObject(backBufferRTV, "backBufferRTV");

		struct Vertex
		{
			Vector2f position;
			uint32 color;
		};
		Vertex vertices[] =
		{
			{ Vector2f(0.0f, 0.0f), 0xffff0000 },
			{ Vector2f(0.0f, 0.5f), 0xff00ff00 },
			{ Vector2f(0.5f, 0.0f), 0xff0000ff },
		};

		D3D11_BUFFER_DESC vertexBufferDesc{};
		vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vertexBufferDesc.ByteWidth = sizeof(vertices);
		vertexBufferDesc.StructureByteStride = sizeof(Vertex);
		vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		D3D11_SUBRESOURCE_DATA vertexData{};
		vertexData.pSysMem = vertices;
		vertexData.SysMemPitch = sizeof(vertices);
		PD3D11Buffer vertexBuffer;
		device->CreateBuffer(&vertexBufferDesc, &vertexData, &vertexBuffer.Get());
		NameObject(vertexBuffer, "vertexBuffer");

		PD3DBlob output;
		PD3DBlob vertexCode;
		D3DCompileFromFile(
			L"data/shaders/shader.hlsl", nullptr, nullptr, "vsMain", "vs_5_0",
			D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0,
			&vertexCode.Get(), &output.Get());
		if (output)
			std::cout << (const char*)output->GetBufferPointer() << std::endl;
		PD3D11VertexShader vertexShader;
		device->CreateVertexShader(vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), nullptr, &vertexShader.Get());
		NameObject(vertexShader, "data/shaders/shader.hlsl : vsMain");

		PD3DBlob pixelCode;
		D3DCompileFromFile(
			L"data/shaders/shader.hlsl", nullptr, nullptr, "psMain", "ps_5_0",
			D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0,
			&pixelCode.Get(), &output.Get());
		if (output)
			std::cout << (const char*)output->GetBufferPointer() << std::endl;
		PD3D11PixelShader pixelShader;
		device->CreatePixelShader(pixelCode->GetBufferPointer(), pixelCode->GetBufferSize(), nullptr, &pixelShader.Get());
		NameObject(pixelShader, "data/shaders/shader.hlsl : psMain");
		
		PD3DBlob _vertexCode;
		D3DCompileFromFile(
			L"data/shaders/shader.hlsl", nullptr, nullptr, "vsFinalMain", "vs_5_0",
			D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0,
			&_vertexCode.Get(), &output.Get());
		if (output)
			std::cout << (const char*)output->GetBufferPointer() << std::endl;
		PD3D11VertexShader finalVertexShader;
		device->CreateVertexShader(_vertexCode->GetBufferPointer(), _vertexCode->GetBufferSize(), nullptr, &finalVertexShader.Get());
		NameObject(finalVertexShader, "data/shaders/shader.hlsl : vsFinalMain");

		D3DCompileFromFile(
			L"data/shaders/shader.hlsl", nullptr, nullptr, "psFinalMain", "ps_5_0",
			D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3 | D3DCOMPILE_WARNINGS_ARE_ERRORS, 0,
			&pixelCode.Get(), &output.Get());
		if (output)
			std::cout << (const char*)output->GetBufferPointer() << std::endl;
		PD3D11PixelShader finalPixelShader;
		device->CreatePixelShader(pixelCode->GetBufferPointer(), pixelCode->GetBufferSize(), nullptr, &finalPixelShader.Get());
		NameObject(finalPixelShader, "data/shaders/shader.hlsl : psFinalMain");

		D3D11_INPUT_ELEMENT_DESC elements[2]{};
		elements[0].SemanticName = "POSITION";
		elements[0].Format = DXGI_FORMAT_R32G32_FLOAT;
		elements[0].AlignedByteOffset = offsetof(Vertex, position);
		elements[1].SemanticName = "COLOR";
		elements[1].Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		elements[1].AlignedByteOffset = offsetof(Vertex, color);
		PD3D11InputLayout inputLayout;
		device->CreateInputLayout(elements, ARRAYSIZE(elements), vertexCode->GetBufferPointer(), vertexCode->GetBufferSize(), &inputLayout.Get());
		NameObject(inputLayout, "inputLayout");

		D3D11_TEXTURE2D_DESC rovTestDesc{};
		rovTestDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
		rovTestDesc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
		rovTestDesc.ArraySize = 1;
		rovTestDesc.Width = canvasDesc.width;
		rovTestDesc.Height = canvasDesc.height;
		rovTestDesc.MipLevels = 1;
		rovTestDesc.SampleDesc.Count = 1;
		rovTestDesc.Usage = D3D11_USAGE_DEFAULT;
		PD3D11Texture2D rovTest;
		device->CreateTexture2D(&rovTestDesc, nullptr, &rovTest.Get());
		NameObject(rovTest, "rovTest");

		D3D11_SHADER_RESOURCE_VIEW_DESC rovTestSRVDesc{};
		rovTestSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		rovTestSRVDesc.Texture2D.MipLevels = 1;
		rovTestSRVDesc.Texture2D.MostDetailedMip = 0;
		PD3D11ShaderResourceView robTestSRV;
		device->CreateShaderResourceView(rovTest, &rovTestSRVDesc, &robTestSRV.Get());

		D3D11_UNORDERED_ACCESS_VIEW_DESC rovTestUAVDesc{};
		rovTestUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		rovTestUAVDesc.Texture2D.MipSlice = 0;
		PD3D11UnorderedAccessView rovTestUAV;
		device->CreateUnorderedAccessView(rovTest, &rovTestUAVDesc, &rovTestUAV.Get());

		while (UpdateSystemMessages() == SystemMessageResult::Continue)
		{
			userAnnotations->BeginEvent(L"Frame");
						
			FLOAT gray[] = {0.5f, 0.5f, 0.5f, 1.0f};			
			context->ClearUnorderedAccessViewFloat(rovTestUAV, gray);

			userAnnotations->BeginEvent(L"Render");
			D3D11_VIEWPORT viewport{};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = canvasDesc.width;
			viewport.Height = canvasDesc.height;
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;
			ID3D11UnorderedAccessView* uavs[] = { rovTestUAV };
			context->OMSetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, 0, 1, uavs, nullptr);
			context->RSSetViewports(1, &viewport);
			
			const UINT vertexStride = sizeof(Vertex);
			ID3D11Buffer* vertexStreams[] = { vertexBuffer };
			UINT vertexOffsets[] = { 0 };
			context->IASetInputLayout(inputLayout);
			context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context->IASetVertexBuffers(0, 1, vertexStreams, &vertexStride, vertexOffsets);
			context->VSSetShader(vertexShader, nullptr, 0);
			context->PSSetShader(pixelShader, nullptr, 0);
			userAnnotations->SetMarker(L"{Draw \"vertexBuffer\"}");
			context->Draw(3, 0);
			context->ClearState();
			userAnnotations->EndEvent();

			userAnnotations->BeginEvent(L"Finalize");
			ID3D11RenderTargetView* rtvs[] = { backBufferRTV };
			context->OMSetRenderTargets(1, rtvs, nullptr);
			context->RSSetViewports(1, &viewport);
			context->IASetInputLayout(nullptr);
			context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			context->VSSetShader(finalVertexShader, nullptr, 0);
			context->PSSetShader(finalPixelShader, nullptr, 0);
			context->PSSetShaderResources(0, 1, &robTestSRV.Get());
			context->Draw(3, 0);
			context->ClearState();
			userAnnotations->EndEvent();

			userAnnotations->EndEvent();
			swapChain->Present(1, 0);
		}
	}
	return true;

	RenderDevicePtr device = RenderDevice::Create(RenderDeviceDesc());
	if (!device)
		return false;

	VulkanCanvasListener canvasListener;
	CanvasDesc canvasDesc;
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	canvas->AddListener(&canvasListener);
	if (!canvas)
		return false;

	SwapChainDesc swapChainDesc(
		canvas->GetHandle(),
		canvasDesc.width,
		canvasDesc.height);
	SwapChainPtr swapChain = device->CreateSwapChain(swapChainDesc);
	if (!swapChain)
		return false;
	RenderContextPtr context = device->CreateRenderContext();
	if (!context)
		return false;

	SimplePass simplePass;
	if (!simplePass.Prepare(device, swapChain))
		return false;

	// loop
	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		context->Begin();

		simplePass.Render(context, swapChain);

		context->End();
		device->SubmitContext(context);
		swapChain->Present();
	}

	// end

	return true;
}
