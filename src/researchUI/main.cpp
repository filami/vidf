#include "pch.h"


int NotepadTest(int argc, char** argv);
int LensesTest(int argc, char** argv);
int AssetBrowserTest(int argc, char** argv);

#include "fractals/fractals.h"

using namespace vidf;
using namespace dx11;
using namespace viqt;



typedef uint8 Color8[3];


// 445 040222-02 
const Color8 paletteColors[] =
{ { 140, 109, 204 },{ 117, 101, 203 },{ 109, 100, 208 },{ 102, 99, 213 },
{ 98, 93, 210 },{ 95, 88, 208 },{ 101, 89, 205 },{ 107, 90, 203 },
{ 133, 104, 206 },{ 150, 116, 217 },{ 167, 128, 229 },{ 184, 136, 240 },
{ 202, 144, 252 },{ 216, 146, 252 },{ 230, 149, 253 },{ 235, 149, 253 },
{ 240, 150, 254 },{ 247, 153, 254 },{ 240, 150, 254 },{ 233, 148, 254 },
{ 218, 141, 252 },{ 204, 134, 250 },{ 195, 128, 244 },{ 186, 122, 239 },
{ 151, 98, 207 },{ 132, 86, 188 },{ 114, 74, 170 },{ 96, 65, 156 },
{ 79, 57, 143 },{ 69, 52, 135 },{ 59, 47, 127 },{ 43, 39, 110 },
{ 29, 31, 91 },{ 8, 14, 53 },{ 5, 9, 42 },{ 2, 4, 32 },
{ 2, 4, 34 },{ 2, 5, 36 },{ 4, 8, 43 },{ 6, 12, 51 },
{ 16, 28, 82 },{ 20, 35, 97 },{ 24, 43, 112 },{ 23, 42, 112 },
{ 22, 42, 113 },{ 21, 42, 113 },{ 21, 42, 114 },{ 21, 42, 114 },
{ 21, 42, 114 },{ 24, 45, 118 },{ 21, 41, 108 },{ 19, 37, 99 },
{ 14, 29, 82 },{ 10, 21, 65 },{ 8, 17, 57 },{ 6, 13, 49 },
{ 1, 3, 26 },{ 0, 2, 24 },{ 0, 1, 22 },{ 0, 3, 27 },
{ 1, 6, 33 },{ 3, 9, 41 },{ 5, 13, 49 },{ 11, 21, 67 },
{ 17, 29, 85 },{ 29, 43, 118 },{ 30, 47, 126 },{ 32, 51, 134 },
{ 32, 52, 136 },{ 33, 53, 138 },{ 33, 54, 139 },{ 32, 54, 139 },
{ 35, 55, 142 },{ 37, 58, 149 },{ 39, 61, 157 },{ 45, 62, 160 },
{ 51, 63, 164 },{ 54, 62, 164 },{ 57, 62, 164 },{ 63, 60, 163 },
{ 71, 58, 161 },{ 90, 65, 176 },{ 108, 75, 193 },{ 127, 85, 211 },
{ 138, 89, 219 },{ 149, 94, 228 },{ 168, 103, 241 },{ 186, 108, 251 },
{ 208, 114, 254 },{ 214, 116, 254 },{ 221, 118, 254 },{ 222, 119, 254 },
{ 224, 120, 254 },{ 223, 121, 254 },{ 219, 121, 254 },{ 208, 119, 251 },
{ 191, 116, 242 },{ 156, 99, 212 },{ 137, 88, 193 },{ 119, 78, 175 },
{ 110, 73, 166 },{ 102, 68, 158 },{ 83, 60, 144 },{ 65, 52, 128 },
{ 32, 35, 93 },{ 21, 26, 74 },{ 10, 17, 55 },{ 7, 13, 47 },
{ 5, 10, 40 },{ 2, 4, 29 },{ 1, 1, 23 },{ 0, 0, 21 },
{ 0, 0, 20 },{ 0, 0, 20 },{ 0, 0, 21 },{ 0, 0, 22 },
{ 1, 0, 24 },{ 1, 2, 28 },{ 3, 5, 35 },{ 7, 12, 52 },
{ 24, 28, 89 },{ 35, 35, 107 },{ 47, 43, 126 },{ 52, 46, 133 },
{ 58, 49, 140 },{ 68, 53, 150 },{ 76, 60, 167 },{ 83, 67, 184 },
{ 87, 74, 200 },{ 88, 80, 212 },{ 88, 79, 211 },{ 88, 79, 210 },
{ 85, 76, 200 },{ 84, 73, 194 },{ 83, 73, 194 },{ 84, 75, 201 },
{ 82, 77, 205 },{ 74, 75, 200 },{ 67, 74, 195 },{ 62, 70, 186 },
{ 57, 67, 178 },{ 49, 62, 164 },{ 45, 58, 155 },{ 44, 55, 150 },
{ 47, 52, 145 },{ 47, 49, 137 },{ 45, 46, 129 },{ 43, 43, 122 },
{ 34, 37, 105 },{ 23, 29, 86 },{ 13, 21, 66 },{ 7, 13, 47 },
{ 1, 3, 21 },{ 1, 2, 18 },{ 1, 1, 16 },{ 1, 1, 14 },
{ 0, 0, 12 },{ 1, 0, 12 },{ 1, 1, 14 },{ 1, 1, 16 },
{ 2, 4, 22 },{ 8, 15, 46 },{ 10, 18, 54 },{ 12, 22, 63 },
{ 18, 29, 81 },{ 24, 37, 99 },{ 27, 43, 114 },{ 30, 48, 125 },
{ 29, 49, 128 },{ 28, 49, 128 },{ 27, 49, 129 },{ 28, 49, 129 },
{ 29, 48, 129 },{ 29, 47, 127 },{ 29, 47, 124 },{ 26, 41, 110 },
{ 22, 35, 96 },{ 15, 27, 78 },{ 10, 20, 61 },{ 5, 12, 46 },
{ 2, 5, 34 },{ 0, 2, 28 },{ 0, 0, 23 },{ 0, 0, 22 },
{ 0, 0, 21 },{ 0, 0, 21 },{ 0, 0, 22 },{ 0, 0, 23 },
{ 0, 0, 24 },{ 0, 0, 26 },{ 0, 0, 27 },{ 0, 1, 28 },
{ 0, 1, 29 },{ 0, 1, 30 },{ 0, 2, 30 },{ 0, 2, 30 },
{ 0, 2, 30 },{ 0, 2, 31 },{ 0, 2, 32 },{ 0, 3, 35 },
{ 1, 6, 40 },{ 3, 10, 51 },{ 9, 18, 67 },{ 18, 26, 85 },
{ 30, 35, 103 },{ 43, 44, 120 },{ 54, 51, 135 },{ 64, 56, 149 },
{ 68, 61, 161 },{ 69, 67, 174 },{ 69, 71, 183 },{ 70, 73, 188 },
{ 71, 76, 192 },{ 73, 79, 194 },{ 79, 82, 200 },{ 81, 83, 202 },
{ 81, 85, 206 },{ 76, 85, 210 },{ 71, 83, 209 },{ 65, 82, 206 },
{ 60, 78, 199 },{ 59, 78, 198 },{ 60, 79, 200 },{ 63, 82, 205 },
{ 66, 86, 215 },{ 66, 87, 215 },{ 64, 85, 212 },{ 60, 78, 198 },
{ 55, 71, 184 },{ 51, 64, 169 },{ 47, 59, 158 },{ 44, 58, 156 },
{ 42, 59, 155 },{ 43, 61, 158 },{ 47, 62, 159 },{ 55, 63, 159 },
{ 68, 63, 158 },{ 83, 64, 159 },{ 96, 67, 162 },{ 106, 73, 173 },
{ 116, 81, 190 },{ 125, 91, 208 },{ 134, 100, 225 },{ 149, 107, 239 },
{ 168, 113, 251 },{ 187, 119, 254 },{ 202, 122, 254 },{ 211, 126, 254 },
{ 216, 131, 254 },{ 216, 138, 254 },{ 200, 135, 252 },{ 186, 131, 250 },
{ 175, 129, 247 },{ 163, 122, 237 },{ 151, 114, 224 },{ 139, 105, 211 }
};



class AverageBrightnessPass
{
private:
	struct CBuffer
	{
		Vector2f invSize;
		Vector2f _;
	};

	struct Pass
	{
		ConstantBuffer cbuffer;
		RenderPassPtr renderPass;
		GraphicsPSOPtr pso;
		PD3D11ShaderResourceView inputSRV;
		RenderTarget output;
	};

public:
	void Prepare(RenderDevicePtr renderDevice, ShaderManager& shaderManager, uint width, uint height)
	{
		vsShader = shaderManager.CompileShaderFile("data/shaders/flame/display.hlsl", "vsFullscreen", ShaderType::VertexShader);
		psShader = shaderManager.CompileShaderFile("data/shaders/flame/display.hlsl", "psAverage", ShaderType::PixelShader);
		psShaderColor = shaderManager.CompileShaderFile("data/shaders/flame/display.hlsl", "psAverageColor", ShaderType::PixelShader);

		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		// samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = 0;
		renderDevice->GetDevice()->CreateSamplerState(&samplerDesc, &sampler.Get());

		bool first = true;
		Vector2i outputSize{ width, height };
		do
		{
			Vector2i inputSize = outputSize;
			outputSize.x = Max(1, outputSize.x / 4);
			outputSize.y = Max(1, outputSize.y / 4);

			CBuffer cbData;
			cbData.invSize = Vector2f(1.0f / inputSize.x, 1.0f / inputSize.y);
			ConstantBufferDesc cbDesc(sizeof(CBuffer), "cabCB");
			ConstantBuffer cbuffer = ConstantBuffer::Create(renderDevice, cbDesc);
			cbuffer.Update(renderDevice->GetContext(), cbData);

			RenderTargetDesc outputDesc{ DXGI_FORMAT_R32_FLOAT, uint(outputSize.x), uint(outputSize.y), "cabOutput" };
			RenderTarget output = RenderTarget::Create(renderDevice, outputDesc);

			GraphicsPSODesc psoDesc;
			psoDesc.rasterizer.CullMode = D3D11_CULL_NONE;
			psoDesc.rasterizer.FillMode = D3D11_FILL_SOLID;
			psoDesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			psoDesc.vertexShader = vsShader;
			psoDesc.pixelShader = first ? psShaderColor : psShader;
			GraphicsPSOPtr pso = GraphicsPSO::Create(renderDevice, psoDesc);

			D3D11_VIEWPORT viewport{};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = uint(outputSize.x);
			viewport.Height = uint(outputSize.y);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			RenderPassDesc renderPassDesc;
			renderPassDesc.viewport = viewport;
			renderPassDesc.rtvs[0] = output.rtv;
			RenderPassPtr renderPass = RenderPass::Create(renderDevice, renderPassDesc);

			Pass pass;
			pass.cbuffer = cbuffer;
			pass.output = output;
			pass.pso = pso;
			pass.renderPass = renderPass;
			if (!passes.empty())
				pass.inputSRV = passes.back().output.srv;
			passes.push_back(pass);

			first = false;
		} while (!(outputSize.x == 1 && outputSize.y == 1));
	}

	void Draw(CommandBuffer& commandBuffer, PD3D11ShaderResourceView inputSRV)
	{
		VIDF_GPU_EVENT(commandBuffer.GetRenderDevice(), ComputeAverageBrightness);

		passes[0].inputSRV = inputSRV;

		for (auto& pass : passes)
		{
			commandBuffer.BeginRenderPass(pass.renderPass);
			commandBuffer.SetGraphicsPSO(pass.pso);
			commandBuffer.SetConstantBuffer(0, pass.cbuffer.buffer);
			commandBuffer.GetContext()->PSSetSamplers(0, 1, &sampler.Get());
			commandBuffer.SetSRV(0, pass.inputSRV);
			commandBuffer.Draw(3, 0);
			commandBuffer.EndRenderPass();
		}
	}

	PD3D11ShaderResourceView GetOutputSRV() const { return passes.back().output.srv; }

private:
	std::vector<Pass> passes;
	ShaderPtr vsShader;
	ShaderPtr psShader;
	ShaderPtr psShaderColor;
	PD3D11SamplerState sampler;
};



const uint texWidth = 1500;
const uint texHeight = 1500;
const uint numThreads = 64;
const uint dispatchSize = 1024 * 2;



struct Camera
{
	void serialize(yasli::Archive& ar)
	{
		ar(focalDist, "focalDist", "Focal Distance");
		ar(cocSize, "cocSize", "Circle Of Confusion");
	}

	float focalDist = 3.0;
	float cocSize = 0.045;
};



struct Colors
{
	void serialize(yasli::Archive& ar)
	{
		ar(brightness, "brightness", "Brightness");
		ar(saturation, "saturation", "Saturation");
		ar(vibrance, "vibrance", "Vibrance");
		ar(gamma, "gamma", "Gamma");
	}

	float brightness = 5.0;
	float saturation = 0.95;
	float vibrance = 1.6;
	float gamma = 3.0;
};


bool serialize(yasli::Archive& ar, Vector2f& point, const char* name, const char* label)
{
	return true;
}



class Variation : public yasli::RefCounter
{
public:
	virtual void serialize(yasli::Archive& ar)
	{
		ar(enabled, "enabled", "^^");
		ar(weigth, "weigth", "^>42>");
	}

private:
	float weigth = 1.0f;
	bool enabled = true;
};


class VarLinear : public Variation
{
};

YASLI_CLASS(Variation, VarLinear, "Linear")

class VarJuliaN : public Variation
{
public:
	void serialize(yasli::Archive& ar) override
	{
		Variation::serialize(ar);
		ar(power, "power", "Power");
		ar(dist, "dist", "Distance");
	}

private:
	float power = 3.0f;
	float dist = 1.0f;
};

YASLI_CLASS(Variation, VarJuliaN, "JuliaN")



struct Transform
{
	void serialize(yasli::Archive& ar)
	{
		ar(enabled, "enabled", "^^");
		ar(weigth, "weigth", "Weigth");
	//	ar(offset, "offset", "Offset");
	//	ar(xAxis, "xAxis", "xAxis");
	//	ar(yAxis, "yAxis", "yAxis");
		ar(variations, "variations", "Variations");
	}

	Vector2f offset{ 0.0f, 0.0f };
	Vector2f xAxis{ 1.0f, 0.0f };
	Vector2f yAxis{ 0.0f, 1.0f };
	float weigth = 1.0f;
	std::vector<yasli::SharedPtr<Variation>> variations;
	bool enabled = true;
};


class FlameDocument
{
public:
	FlameDocument()
	{
		transforms.emplace_back();
	}

	void serialize(yasli::Archive& ar)
	{
		ar(camera, "camera", "Camera");
		ar(colors, "colors", "Colors");
		ar(transforms, "transforms", "Transforms");
	}

	Camera camera;
	Colors colors;
	std::vector<Transform> transforms;
};



class FlamesRenderer : public Dx11Widget
{
private:
	struct ViewConsts
	{
		Matrix44f projViewTM;
		Vector3f cameraPos;
		float time;
		Vector2i viewport;
		uint rngOffset;
		float aspectRatio;
	};

	struct DispConsts
	{
		Vector4f disp;
	};

	struct Sample
	{
		Vector2f point;
		uint rngSeed;
	};

public:
	FlamesRenderer(QWidget* parent, Renderer& _renderer)
		: Dx11Widget(parent, _renderer.GetRenderDevice())
		, renderer(_renderer)
	{ }

protected:
	void Render() override
	{
		RenderDevicePtr renderDevice = renderer.GetRenderDevice();
		CommandBuffer& commandBuffer = *renderer.GetCommandBuffer().get();

		Vector3f camPos = Vector3f(1.0f, 1.0f, 1.2f);
		Vector3f camTarget = Vector3f(0.0f, 0.0f, -1.0f);
		camPos = camTarget + (camPos - camTarget) * 1.25f;

		Matrix44f projTM = PerspectiveFovLH(1.4f, texWidth / float(texHeight), 0.01f, 100.0f);
		Matrix44f viewTM = LookAtLH(camPos, camTarget, Vector3f(0.0f, 0.0f, 1.0f));
		viewConsts.projViewTM = Mul(viewTM, projTM);
		viewConsts.cameraPos = camPos;
		viewConsts.rngOffset += numThreads * dispatchSize;
		viewCB.Update(renderDevice->GetContext(), viewConsts);

		const float zoom = 2.0f;
		DispConsts disp;
		disp.disp.x = disp.disp.y = 0.0f;
		disp.disp.z = disp.disp.w = zoom;
		dispCB.Update(renderDevice->GetContext(), disp);
		
		const bool clearHistogram = false;
		if (clearHistogram)
		{
			VIDF_GPU_EVENT(renderDevice, ClearHistorgram);
			const float zero[] = { 0, 0, 0, 0 };
			commandBuffer.GetContext()->ClearUnorderedAccessViewFloat(historgram.uav, zero);
			viewConsts.rngOffset = 0;
		}

		{
			VIDF_GPU_EVENT(renderDevice, FlameCS);

			for (uint i = 0; i < samples.size(); ++i)
			{
				samples[i].point.x = sampleDist(rand);
				samples[i].point.y = sampleDist(rand);
				samples[i].rngSeed = rand();
			}
			samplesBuffer.Update(renderDevice->GetContext(), samples.data(), samples.size() * sizeof(samples[0]));

			commandBuffer.BeginComputePass(flameCSPass);

			commandBuffer.SetCompute(flameCS);
			commandBuffer.SetConstantBuffer(0, viewCB.buffer);
			commandBuffer.SetSRV(0, palette.srv);
			commandBuffer.SetSRV(1, samplesBuffer.srv);
			commandBuffer.GetContext()->CSSetSamplers(0, 1, &paletteSS.Get());
			commandBuffer.Dispatch(Vector3i(int(dispatchSize), 1, 1));

			commandBuffer.EndComputePass();
		}

		averageBrightnessPass.Draw(commandBuffer, historgram.srv);

		{
			VIDF_GPU_EVENT(renderDevice, Finalize);

			commandBuffer.BeginRenderPass(finalizePass);

			commandBuffer.SetGraphicsPSO(finalizePSO);
			commandBuffer.SetConstantBuffer(0, dispCB.buffer);
			commandBuffer.SetSRV(0, historgram.srv);
			commandBuffer.SetSRV(1, averageBrightnessPass.GetOutputSRV());
			commandBuffer.Draw(3, 0);

			commandBuffer.EndRenderPass();
		}

		{
			VIDF_GPU_EVENT(renderDevice, Display);

			commandBuffer.BeginRenderPass(displayPass);

			commandBuffer.SetGraphicsPSO(displayPSO);
			commandBuffer.SetConstantBuffer(0, dispCB.buffer);
			commandBuffer.SetSRV(0, finalBuffer.srv);
			commandBuffer.Draw(3, 0);

			commandBuffer.EndRenderPass();
		}

		GetSwapChain()->Present(false);

		update();
	}

	void Resize(int width, int heigth) override
	{
		RenderDevicePtr renderDevice = renderer.GetRenderDevice();

		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = width;
		viewport.Height = heigth;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		RenderPassDesc displayPassDesc;
		displayPassDesc.viewport = viewport;
		displayPassDesc.rtvs[0] = GetSwapChain()->GetBackBufferRTV();
		displayPass = RenderPass::Create(renderDevice, displayPassDesc);

		if (!outputRTV)
		{
			outputRTV = GetSwapChain()->GetBackBufferRTV();
			CompileShaders();
			PreparePipeline();
		}
	}

private:
	void CompileShaders()
	{
		ShaderManagerPtr shaderManager = renderer.GetShaderManager();
		flameCS = shaderManager->CompileShaderFile("data/shaders/flame/flameCS.hlsl", "csFlame", ShaderType::ComputeShader);
		fullscreenVS = shaderManager->CompileShaderFile("data/shaders/flame/display.hlsl", "vsFullscreen", ShaderType::VertexShader);
		finalizePS = shaderManager->CompileShaderFile("data/shaders/flame/display.hlsl", "psFinalize", ShaderType::PixelShader);
		displayPS = shaderManager->CompileShaderFile("data/shaders/flame/display.hlsl", "psDisplay", ShaderType::PixelShader);
	}

	void PreparePipeline()
	{
		RenderDevicePtr renderDevice = renderer.GetRenderDevice();
		ShaderManager& shaderManager = *renderer.GetShaderManager().get();

		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = texWidth;
		viewport.Height = texHeight;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		RenderTargetDesc finalBufferDesc{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, texWidth, texHeight, "FinalBuffer" };
		finalBuffer = RenderTarget::Create(renderDevice, finalBufferDesc);

		RenderPassDesc finalizePassDesc;
		finalizePassDesc.viewport = viewport;
		finalizePassDesc.rtvs[0] = finalBuffer.rtv;
		finalizePass = RenderPass::Create(renderDevice, finalizePassDesc);

		GraphicsPSODesc finalizePSODesc;
		finalizePSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
		finalizePSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
		finalizePSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		finalizePSODesc.vertexShader = fullscreenVS;
		finalizePSODesc.pixelShader = finalizePS;
		finalizePSO = GraphicsPSO::Create(renderDevice, finalizePSODesc);

		GraphicsPSODesc displayPSODesc = finalizePSODesc;
		displayPSODesc.pixelShader = displayPS;
		displayPSO = GraphicsPSO::Create(renderDevice, displayPSODesc);

		averageBrightnessPass.Prepare(renderDevice, shaderManager, texWidth, texHeight);

		ConstantBufferDesc viewCBDesc(sizeof(ViewConsts), "viewCB");
		viewCB = ConstantBuffer::Create(renderDevice, viewCBDesc);

		ConstantBufferDesc dispCBDesc(sizeof(DispConsts), "viewCB");
		dispCB = ConstantBuffer::Create(renderDevice, dispCBDesc);

		samples.resize(dispatchSize * numThreads);
		StructuredBufferDesc samplesBufferDesc(sizeof(Sample), dispatchSize * numThreads, "SamplesBuffer", true);
		samplesBuffer = StructuredBuffer::Create(renderDevice, samplesBufferDesc);

		std::vector<uint32> paletteData;
		paletteData.resize(sizeof(paletteColors) / sizeof(paletteColors[0]));
		for (uint i = 0; i < paletteData.size(); ++i)
			paletteData[i] = (paletteColors[i][2] << 16) | (paletteColors[i][1] << 8) | (paletteColors[i][0]);
		Texture2DDesc paletteDesc{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, uint(paletteData.size()), 1, "Palette" };
		paletteDesc.dataSize = paletteData.size() * sizeof(uint32);
		paletteDesc.dataPtr = paletteData.data();
		palette = Texture2D::Create(renderDevice, paletteDesc);

		D3D11_SAMPLER_DESC paletteSSDesc{};
		paletteSSDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		paletteSSDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		paletteSSDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		paletteSSDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		paletteSSDesc.MinLOD = 0;
		paletteSSDesc.MaxLOD = 0;
		renderDevice->GetDevice()->CreateSamplerState(&paletteSSDesc, &paletteSS.Get());

		RWTexture2DDesc historgramDesc{ DXGI_FORMAT_R32G32B32A32_FLOAT, texWidth, texHeight, "Historgram" };
		historgram = RWTexture2D::Create(renderDevice, historgramDesc);

		RenderPassDesc flameCSPassDesc;
		flameCSPassDesc.uavs[0] = historgram.uav;
		flameCSPass = RenderPass::Create(renderDevice, flameCSPassDesc);

		viewConsts.viewport = Vector2i(texWidth, texHeight);
		viewConsts.time = 0;
		viewConsts.aspectRatio = texWidth / float(texHeight);
		viewConsts.rngOffset = 0;
	}

private:
	Renderer& renderer;
	PD3D11RenderTargetView outputRTV;
	AverageBrightnessPass averageBrightnessPass;
	ShaderPtr flameCS;
	ShaderPtr fullscreenVS;
	ShaderPtr finalizePS;
	ShaderPtr displayPS;
	RenderPassPtr  displayPass;
	RenderPassPtr  finalizePass;
	RenderPassPtr  flameCSPass;
	GraphicsPSOPtr finalizePSO;
	GraphicsPSOPtr displayPSO;
	RenderTarget   finalBuffer;
	ConstantBuffer viewCB;
	ConstantBuffer dispCB;
	Texture2D      palette;
	RWTexture2D    historgram;
	StructuredBuffer samplesBuffer;
	PD3D11SamplerState paletteSS;
	ViewConsts     viewConsts;
	std::mt19937   rand;
	std::uniform_real_distribution<> sampleDist{ -1.0f, 1.0f };
	std::vector<Sample> samples;
};



class FlamesFrame : public QMainWindow
{
public:
	FlamesFrame()
	{
		QWidget* mainWidget = new QWidget();
		QBoxLayout* layout = new QBoxLayout(QBoxLayout::LeftToRight);
		mainWidget->setLayout(layout);
		mainWidget->setAttribute(Qt::WA_NoSystemBackground, true);

		flamesRenderer = new FlamesRenderer(this, renderer);
		layout->addWidget(flamesRenderer);

		propertyTree = new QPropertyTree();
		propertyTree->attach(yasli::Serializer(document));
		propertyTree->setUndoEnabled(true, false);
		propertyTree->setMaximumWidth(380);
		propertyTree->expandAll();
		layout->addWidget(propertyTree);

		setCentralWidget(mainWidget);

		setWindowTitle(tr("Flames"));
	}

private:
	FlamesRenderer* flamesRenderer;
	QPropertyTree*  propertyTree;
	Renderer        renderer;
	FlameDocument   document;
};



int Flames(int argc, char** argv)
{
	QApplication app{ argc, argv };

	FlamesFrame flamesFrame;
	flamesFrame.showMaximized();

	return app.exec();
}



int main(int argc, char** argv)
{
	vidf::SetCurrentDirectory("../../");
//	return NotepadTest(argc, argv);
//	return LensesTest(argc, argv);
	return AssetBrowserTest(argc, argv);
//	return Flames(argc, argv);
//	return viqt::Fractals(argc, argv);
}

/*
#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/FlowView>

#include <QtWidgets/QApplication>

#include <nodes/DataModelRegistry>

#include "../ext/nodeeditor/examples/example2/TextSourceDataModel.hpp"
#include "../ext/nodeeditor/examples/example2/TextDisplayDataModel.hpp"

using QtNodes::DataModelRegistry;
using QtNodes::FlowView;
using QtNodes::FlowScene;

static std::shared_ptr<DataModelRegistry>
registerDataModels()
{
	auto ret = std::make_shared<DataModelRegistry>();

	ret->registerModel<TextSourceDataModel>();

	ret->registerModel<TextDisplayDataModel>();

	return ret;
}


int
main(int argc, char *argv[])
{
	QApplication app(argc, argv);

	FlowScene scene(registerDataModels());

	FlowView view(&scene);

	view.setWindowTitle("Node-based flow editor");
	view.resize(800, 600);
	view.show();

	return app.exec();
}
*/
