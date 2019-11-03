#include "pch.h"

#include "QPropertyTree/QPropertyTree.h"
#include "yasli/Enum.h"
#include "yasli/STL.h"
#include "yasli/decorators/Range.h"
#include "yasli/JSONIArchive.h"
#include "yasli/JSONOArchive.h"

#include "viewport/dx11viewport.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"


namespace viqt
{


	class Renderer
	{
	public:
		Renderer();

		RenderDevicePtr  GetRenderDevice()  { return renderDevice; }
		ShaderManagerPtr GetShaderManager() { return shaderManager; }
		CommandBufferPtr GetCommandBuffer() { return commandBuffer; }

	private:
		RenderDevicePtr  renderDevice;
		ShaderManagerPtr shaderManager;
		CommandBufferPtr commandBuffer;
	};



	class FractalsRenderer : public Dx11Widget
	{
	public:
		FractalsRenderer(QWidget* parent, Renderer& _renderer);

		void CompileShaders(const char* _shaderName);
		void ReloadShaders();

	protected:
		void Render() override;
		void Resize(int width, int heigth) override;

	private:
		void PreparePipeline();
		void CompileShaders();

	private:
		Renderer& renderer;

		PD3D11RenderTargetView outputRTV;
		ShaderPtr mainVS;
		ShaderPtr mainPS;
		RenderPassPtr  displayPass;
		GraphicsPSOPtr displayPSO;
		ConstantBuffer viewCB;

		std::string shaderName;
		uint width = 0;
		uint height = 0;
		bool valid = false;
	};



	class FractalsFrame : public QMainWindow
	{
		Q_OBJECT
	public:
		FractalsFrame();

	private slots:
		void OnOpenFile();
		void OnReload();

	private:
		FractalsRenderer* fractalsRenderer;
		QPropertyTree*    propertyTree;
		Renderer          renderer;
	};




	int Fractals(int argc, char** argv);


}
