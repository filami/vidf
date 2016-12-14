#include "pch.h"

#include <vidf/proto/protodx11.h>
#include "proto/renderresource.h"
#include "proto/mesh.h"
#include "proto/texture.h"
#include "proto/rendertarget.h"
#include "proto/shader.h"
#include "proto/buffer.h"


void ManyParticles()
{
	using namespace vidf;
	using namespace proto;
	using namespace vproto;

	const int width = 1280;
	const int height = 720;
	ProtoDX11Desc protoDx11Desc(width, height);
	protoDx11Desc.caption = "Space Renderer";
	proto::ProtoDX11Window window;
	window.Initialize(protoDx11Desc);

	const float fov = Degrees2Radians(85.0f);
	proto::OrbitalCamera camera(window.GetCanvas(), proto::CameraListener_Full);
	camera.SetPerspective(fov, 1.0f, 10000.0f);
	camera.SetCamera(Vector3f(zero), Quaternionf(zero), 10.0f);

	ID3D11Device* device = window.GetRenderDevice()->GetDevice();
	ID3D11DeviceContext* context = window.GetRenderDevice()->GetContext();
	RenderTarget frameBuffer = CreateFrameBuffer(window.GetSwapChain());

	ID3D11UnorderedAccessView* frameBufferUAV;
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory(&uavDesc, sizeof(uavDesc));
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	device->CreateUnorderedAccessView(frameBuffer.texture, &uavDesc, &frameBufferUAV);

	const unsigned int numParticles = 8 * 1024 * 1024;
	// const unsigned int numParticles = 512 * 1024;
	Shader shader = Shader(device, "manyparticles/particles.hlsl");
	// RWBuffer sceneBuffer = CreateRWTexture2D(device, width, height, DXGI_FORMAT_R16G16B16A16_FLOAT);
	RWBuffer sceneBufferRed = CreateRWTexture2D(device, width, height, DXGI_FORMAT_R16_FLOAT);
	RWBuffer sceneBufferGreen = CreateRWTexture2D(device, width, height, DXGI_FORMAT_R16_FLOAT);
	RWBuffer sceneBufferBlue = CreateRWTexture2D(device, width, height, DXGI_FORMAT_R16_FLOAT);
	RWBuffer particleBuffer = CreateRWBuffer(device, sizeof(float)*(3+3+2), numParticles);

	TimeCounter counter;
	Time scenetime;

	bool pressed = false;
	bool reInit = true;

	bool recompileShaders = false;
	while (window.Update())
	{
		Time deltaTime = counter.GetElapsed();
		scenetime += deltaTime;

		if (GetAsyncKeyState(VK_F5) & 0x8000)
		{
			if (!pressed)
				recompileShaders = true;
			pressed = true;
		}
		else
			pressed = false;

		FLOAT black[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		if (recompileShaders)
		{
			scenetime = Time();
			shader.Recompile();
			context->ClearUnorderedAccessViewFloat(particleBuffer.GetUnorderedAccessView(), black);
			recompileShaders = false;
			reInit = true;
		}

		context->ClearUnorderedAccessViewFloat(sceneBufferRed.GetUnorderedAccessView(), black);
		context->ClearUnorderedAccessViewFloat(sceneBufferGreen.GetUnorderedAccessView(), black);
		context->ClearUnorderedAccessViewFloat(sceneBufferBlue.GetUnorderedAccessView(), black);

		Vector4f viewportSize = Vector4f(float(width), float(height), 0.0f, 0.0f);
		Vector4f invViewportSize = Vector4f(1.0f/float(width), 1.0f/float(height), 0.0f, 0.0f);
		Matrix44f projectionTM = camera.PerspectiveMatrix();
		Matrix44f viewTM = camera.ViewMatrix();
		Matrix44f viewProjectionTM = Mul(viewTM, projectionTM);
		shader.GetShader()->GetVariableByName("pv_viewProjectionTM")->AsMatrix()->SetMatrix((float*)&viewProjectionTM);
		shader.GetShader()->GetVariableByName("pv_viewportSize")->AsVector()->SetFloatVector(&viewportSize.x);
		shader.GetShader()->GetVariableByName("pv_invViewportSize")->AsVector()->SetFloatVector(&invViewportSize.x);
		shader.GetShader()->GetVariableByName("pv_sceneTime")->AsScalar()->SetFloat(scenetime.AsFloat());
		shader.GetShader()->GetVariableByName("pv_deltaTime")->AsScalar()->SetFloat(deltaTime.AsFloat());
		shader.GetShader()->GetVariableByName("particlesUAV")->AsUnorderedAccessView()->SetUnorderedAccessView(particleBuffer.GetUnorderedAccessView());
		shader.GetShader()->GetVariableByName("frameBufferRedUAV")->AsUnorderedAccessView()->SetUnorderedAccessView(sceneBufferRed.GetUnorderedAccessView());
		shader.GetShader()->GetVariableByName("frameBufferGreenUAV")->AsUnorderedAccessView()->SetUnorderedAccessView(sceneBufferGreen.GetUnorderedAccessView());
		shader.GetShader()->GetVariableByName("frameBufferBlueUAV")->AsUnorderedAccessView()->SetUnorderedAccessView(sceneBufferBlue.GetUnorderedAccessView());

		ID3DX11EffectTechnique* tech = shader.GetShader()->GetTechniqueByName("TestTech");
		if (reInit)
		{
			tech->GetPassByIndex(0)->Apply(0, context);
			context->Dispatch(numParticles/1024, 1, 1);
			reInit = false;
		}
		tech->GetPassByIndex(1)->Apply(0, context);
		context->Dispatch(numParticles/1024, 1, 1);
		tech->GetPassByIndex(2)->Apply(0, context);
		context->Dispatch(numParticles/1024, 1, 1);
		CleanRenderTargets(context);

		D3D11_VIEWPORT vp;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = FLOAT(width);
		vp.Height = FLOAT(height);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		context->RSSetViewports(1, &vp);

		SetRenderTarget(context, frameBuffer);
		shader.GetShader()->GetVariableByName("frameBufferRedSRV")->AsShaderResource()->SetResource(sceneBufferRed.GetShaderView());
		shader.GetShader()->GetVariableByName("frameBufferGreenSRV")->AsShaderResource()->SetResource(sceneBufferGreen.GetShaderView());
		shader.GetShader()->GetVariableByName("frameBufferBlueSRV")->AsShaderResource()->SetResource(sceneBufferBlue.GetShaderView());
		tech->GetPassByIndex(3)->Apply(0, context);
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		context->Draw(3, 0);
		CleanRenderTargets(context);

		window.Swap();
	}
}
