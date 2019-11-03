#pragma once

#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"
#include "vidf/rendererdx11/wikidraw.h"
#include "voxelmodelrenderer.h"

namespace vi::retro::render
{

	/*
	class Dx11CanvasListener : public CanvasListener
	{
	public:
		Dx11CanvasListener(ShaderManager& _shaderManager)
			: shaderManager(_shaderManager) {}

		virtual void Close()
		{
			PostQuitMessage();
		}
		virtual void KeyDown(KeyCode keyCode)
		{
			if (keyCode == KeyCode::Escape)
				PostQuitMessage();
			else if (keyCode == KeyCode::F5)
				shaderManager.RecompileShaders();
		}
		ShaderManager& shaderManager;
	};
	*/




	class WorldRender
	{
	private:
		struct ViewConsts
		{
			Matrix44f projTM;
			Matrix44f viewTM;
			Matrix44f projViewTM;
			Matrix44f invProjViewTM;

			Vector2f  viewportSize;
			Vector2f  invViewportSize;

			Vector3f  viewPosition;
			float     _;
		};

	public:
		WorldRender(CanvasPtr canvas);

		void SetView(const Matrix44f& _projTM, const Matrix44f& _viewTM, Vector3f _viewPosition);

		void Draw();

		RenderDevicePtr GetRenderDevice() const { return renderDevice; }
		WikiDraw&       GetWikiDraw() { return wikiDraw; }
		VoxelModelRenderer& GetVoxelModelRenderer() { return voxelModelRenderer; }

	private:
		RenderDevicePtr renderDevice;
		ShaderManager   shaderManager;
		CommandBuffer   commandBuffer;
		WikiDraw        wikiDraw;
	//	Dx11CanvasListener canvasListener{ shaderManager };

		SwapChainPtr       swapChain;
		VoxelModelRenderer voxelModelRenderer;

		uint          width;
		uint          height;
		Matrix44f     projTM;
		Matrix44f     viewTM;
		Vector3f      viewPosition;
		GPUBuffer     depthStencil;
		RenderPassPtr preZRenderPass;
		RenderPassPtr renderPass;
		GPUBuffer     viewCB;
	};



}
