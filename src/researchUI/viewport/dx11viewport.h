#pragma once


#include "vidf/rendererdx11/renderdevice.h"


namespace viqt
{

	using namespace vidf;
	using namespace dx11;



	class Dx11Widget : public QWidget
	{
	public:
		Dx11Widget(RenderDevicePtr _renderDevice);
		Dx11Widget(QWidget* parent, RenderDevicePtr _renderDevice);

		QPaintEngine* paintEngine() const override;

		RenderDevicePtr GetRenderDevice() { return renderDevice; }
		SwapChainPtr    GetSwapChain() { return swapChain; }

	protected:
		virtual void Render();
		virtual void Resize(int w, int h);

	private:
		void paintEvent(QPaintEvent* e) override final;
		void resizeEvent(QResizeEvent* e) override final;
		void MakeSwapChain();

	private:
		RenderDevicePtr renderDevice;
		SwapChainPtr    swapChain;
	};



}
