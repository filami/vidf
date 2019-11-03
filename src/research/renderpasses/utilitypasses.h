#pragma once

#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/resources.h"


namespace vidf
{


	using namespace dx11;



	class ReducePass
	{
	public:
		enum Mode
		{
			HalfResolution,
			QuarterResolution,
		};

	public:
		void Prepare(RenderDevicePtr renderDevice, ShaderManager& shaderManager, GPUBuffer& inputBuffer, Mode mode);
		void Draw(CommandBuffer& commandBuffer);
		GPUBuffer& GetOutput() { return output; }

	private:
		GPUBuffer input;
		GPUBuffer output;
		ShaderPtr vsShader;
		ShaderPtr psShader;
		RenderPassPtr renderPass;
		GraphicsPSOPtr pso;
	};



	class GaussianBlurPass
	{
	public:
		enum Mode
		{
			Gauss7Samples,
			Gauss15Samples,
		};

		struct CBuffer
		{
			Vector2f invSize;
			Vector2f direction;
		};

	private:

	public:
		void Prepare(RenderDevicePtr renderDevice, ShaderManager& shaderManager, GPUBuffer& inputBuffer, Mode mode);
		void Draw(CommandBuffer& commandBuffer);
		GPUBuffer& GetOutput() { return output; }

	private:
		GPUBuffer input;
		GPUBuffer tempBuffer;
		GPUBuffer output;
		GPUBuffer cbuffer0;
		GPUBuffer cbuffer1;
		PD3D11SamplerState sampler;
		ShaderPtr vsShader;
		ShaderPtr psShader;
		RenderPassPtr renderPass0;
		RenderPassPtr renderPass1;
		GraphicsPSOPtr pso;
	};


}
