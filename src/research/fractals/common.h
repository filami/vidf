#pragma once

#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"
#include "vidf/proto/text.h"
#include "renderpasses/averagebrightness.h"



using namespace vidf;
using namespace dx11;
using namespace proto;



class Dx11CanvasListener : public CanvasListener
{
public:
	Dx11CanvasListener(ShaderManager& _shaderManager)
		: shaderManager(_shaderManager), restartRender(false) {}

	void Close() override
	{
		PostQuitMessage();
	}

	void KeyDown(KeyCode keyCode) override
	{
		if (keyCode == KeyCode::Escape)
			PostQuitMessage();
		else if (keyCode == KeyCode::F5)
		{
			shaderManager.RecompileShaders();
			restartRender = true;
		}
		else if (keyCode == KeyCode::F4)
		{
			display = !display;
		}
		else if (keyCode == KeyCode::F9)
		{
			savePicture = true;
		}
	}

	void MouseWheel(float delta) override
	{
		zoom *= exp(-delta * 0.2f);
	}

	ShaderManager& shaderManager;
	float zoom = 1.0f;
	bool restartRender;
	bool display = true;
	bool savePicture = false;
};
