#include "pch.h"
#include "vidf/renderervulkan/renderdevice.h"
#include "vidf/renderervulkan/rendercontext.h"
#include "vidf/renderervulkan/swapchain.h"
#include "vidf/renderervulkan/renderpass.h"


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



bool TestVulkan()
{
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
