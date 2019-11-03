#pragma once

#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/resources.h"

namespace vidf
{


	using namespace dx11;



	enum class AverageBrightnessMode
	{
		Maximum,
	};



	struct AverageBrightnessDesc
	{
		GPUBuffer inputBuffer;
		AverageBrightnessMode mode = AverageBrightnessMode::Maximum;
		DXGI_FORMAT format = DXGI_FORMAT_R16_FLOAT;
	};



	class AverageBrightness
	{
	private:
		struct CBuffer
		{
			Vector2f invSize;
			Vector2f _;
		};

		struct Pass
		{
			GPUBuffer cbuffer;
			GPUBuffer output;
			RenderPassPtr  renderPass;
			GraphicsPSOPtr pso;
			PD3D11ShaderResourceView inputSRV;
		};

	public:
		void Prepare(RenderDevicePtr renderDevice, ShaderManager& shaderManager, const AverageBrightnessDesc& desc);
		void Draw(CommandBuffer& commandBuffer);

		PD3D11ShaderResourceView GetOutputSRV() const { return passes.back().output.srv; }

	private:
		std::vector<Pass> passes;
		ShaderPtr vsShader;
		ShaderPtr psShader;
		ShaderPtr psShaderColor;
		PD3D11SamplerState sampler;
	};



}
