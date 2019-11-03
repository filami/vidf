#include "pch.h"
#include "fractals.h"

namespace viqt
{



	Renderer::Renderer()
	{
		renderDevice = RenderDevice::Create(RenderDeviceDesc());
		shaderManager = std::make_shared<ShaderManager>(renderDevice);
		commandBuffer = std::make_shared<CommandBuffer>(renderDevice);
	}




	FractalsRenderer::FractalsRenderer(QWidget* parent, Renderer& _renderer)
		: Dx11Widget(parent, _renderer.GetRenderDevice())
		, renderer(_renderer)
	{
	}



	void FractalsRenderer::Render()
	{
		if (!valid)
			return;
		RenderDevicePtr renderDevice = renderer.GetRenderDevice();
		CommandBuffer& commandBuffer = *renderer.GetCommandBuffer().get();

		{
			VIDF_GPU_EVENT(renderDevice, Display);

			commandBuffer.BeginRenderPass(displayPass);

			commandBuffer.SetGraphicsPSO(displayPSO);
			commandBuffer.Draw(3, 0);

			commandBuffer.EndRenderPass();
		}

		GetSwapChain()->Present(false);

		update();
	}



	void FractalsRenderer::Resize(int _width, int _height)
	{
		RenderDevicePtr renderDevice = renderer.GetRenderDevice();
		width = _width;
		height = _height;

		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = width;
		viewport.Height = height;
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



	void FractalsRenderer::CompileShaders(const char* _shaderName)
	{
		shaderName = _shaderName;
		CompileShaders();
		PreparePipeline();
	}



	void FractalsRenderer::ReloadShaders()
	{
		renderer.GetShaderManager()->RecompileShaders();
	}



	void FractalsRenderer::CompileShaders()
	{
		if (shaderName.empty())
			return;
		ShaderManagerPtr shaderManager = renderer.GetShaderManager();
		mainVS = shaderManager->CompileShaderFile(shaderName.c_str(), "vsMain", ShaderType::VertexShader);
		mainPS = shaderManager->CompileShaderFile(shaderName.c_str(), "psMain", ShaderType::PixelShader);

		std::string outputName = shaderName + "(" + "psMain" + ") : ";
		auto reflection = mainPS->GetReflection();
		ID3D11ShaderReflectionConstantBuffer* viewCBRefl = reflection->GetConstantBufferByName("viewCB");
		if (!viewCBRefl)
		{
			std::cout << outputName << "viewCB not found." << std::endl;
			return;
		}
		else
		{
			D3D11_SHADER_BUFFER_DESC viewCBReflDesc;
			viewCBRefl->GetDesc(&viewCBReflDesc);
			for (uint i = 0; i < viewCBReflDesc.Variables; ++i)
			{
				ID3D11ShaderReflectionVariable* varRefl = viewCBRefl->GetVariableByIndex(i);
				ID3D11ShaderReflectionType* typeRefl = varRefl->GetType();
				D3D11_SHADER_VARIABLE_DESC varReflDesc;
				D3D11_SHADER_TYPE_DESC varTypeReflDesc;
				varRefl->GetDesc(&varReflDesc);
				typeRefl->GetDesc(&varTypeReflDesc);

				varTypeReflDesc = varTypeReflDesc;
			}
		}

		valid = true;
	}



	void FractalsRenderer::PreparePipeline()
	{
		if (!valid)
			return;
		RenderDevicePtr renderDevice = renderer.GetRenderDevice();
		ShaderManager& shaderManager = *renderer.GetShaderManager().get();

		D3D11_VIEWPORT viewport{};
		viewport.TopLeftX = 0;
		viewport.TopLeftY = 0;
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;

		RenderPassDesc displayPassDesc;
		displayPassDesc.viewport = viewport;
		displayPassDesc.rtvs[0] = GetSwapChain()->GetBackBufferRTV();
		displayPass = RenderPass::Create(renderDevice, displayPassDesc);

		GraphicsPSODesc displayPSODesc;
		displayPSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
		displayPSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
		displayPSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		displayPSODesc.vertexShader = mainVS;
		displayPSODesc.pixelShader = mainPS;
		displayPSO = GraphicsPSO::Create(renderDevice, displayPSODesc);
	}



	//////////////////////////////////////////////////////////



	class LayoutStack
	{
	public:
		LayoutStack(QWidget* _parent)
			: parent(_parent) { }

		friend class Layout;
		QWidget* parent;
		std::stack<QBoxLayout*> layouts;
	};



	class Layout
	{
	public:
		Layout(LayoutStack& _stack, QBoxLayout* _layout)
			: stack(_stack)
			, layout(_layout)
		{
			if (stack.layouts.empty())
				stack.parent->setLayout(_layout);
			else
				stack.layouts.top()->addLayout(layout);
			stack.layouts.push(layout);
		}
		~Layout()
		{
			stack.layouts.pop();
		}
		QBoxLayout* operator->() { return layout; }

		LayoutStack& stack;
		QBoxLayout*  layout;
	};



	//////////////////////////////////////////////////////////



	FractalsFrame::FractalsFrame()
	{
		QWidget* mainWidget = new QWidget();
		LayoutStack layoutStack{ mainWidget };
		{
			Layout layout{ layoutStack, new QBoxLayout(QBoxLayout::LeftToRight) };
			mainWidget->setAttribute(Qt::WA_NoSystemBackground, true);

			fractalsRenderer = new FractalsRenderer(this, renderer);
			layout->addWidget(fractalsRenderer);

			{
				Layout layout{ layoutStack, new QBoxLayout(QBoxLayout::TopToBottom) };

				QPushButton* openFileButton = new QPushButton(tr("Open File..."));
				layout->addWidget(openFileButton);
				connect(openFileButton, &QPushButton::clicked, this, &FractalsFrame::OnOpenFile);

				QPushButton* reloadButton = new QPushButton(tr("Reload Shaders"));
				layout->addWidget(reloadButton);
				connect(reloadButton, &QPushButton::clicked, this, &FractalsFrame::OnReload);

				propertyTree = new QPropertyTree();
				propertyTree->setUndoEnabled(true, false);
				propertyTree->setMaximumWidth(380);
				propertyTree->expandAll();
				layout->addWidget(propertyTree);
			}
		}
		setCentralWidget(mainWidget);
		setWindowTitle(tr("Fractals"));
	}



	void FractalsFrame::OnOpenFile()
	{
		QString path = QFileDialog::getOpenFileName(this, tr("Open..."), QString(), tr("Shaders(*.hlsl)"));
		if (!path.isEmpty())
			fractalsRenderer->CompileShaders(path.toUtf8().data());
	}



	void FractalsFrame::OnReload()
	{
		fractalsRenderer->ReloadShaders();
	}



	int Fractals(int argc, char** argv)
	{
		QApplication app{ argc, argv };

		FractalsFrame fractalsFrame;
		fractalsFrame.showMaximized();

		return app.exec();
	}


}
