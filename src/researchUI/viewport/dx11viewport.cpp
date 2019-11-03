#include "pch.h"
#include "dx11viewport.h"



namespace viqt
{


	
	Dx11Widget::Dx11Widget(RenderDevicePtr _renderDevice)
		: Dx11Widget(nullptr, _renderDevice)
	{
	}



	Dx11Widget::Dx11Widget(QWidget* parent, RenderDevicePtr _renderDevice)
		: QWidget(parent)
		, renderDevice(_renderDevice)
	{
		setAttribute(Qt::WA_PaintOnScreen, true);
		setAttribute(Qt::WA_NativeWindow, true);
		MakeSwapChain();
	}



	QPaintEngine* Dx11Widget::paintEngine() const
	{
		return nullptr;
	}



	void Dx11Widget::paintEvent(QPaintEvent* e)
	{
		Render();
	}



	void Dx11Widget::Render()
	{
	}



	void Dx11Widget::Resize(int w, int h)
	{
	}



	void Dx11Widget::resizeEvent(QResizeEvent* e)
	{
		const QSize _size = size();
		MakeSwapChain();
		Resize(_size.width(), _size.height());
	}



	void Dx11Widget::MakeSwapChain()
	{
		const QSize _size = size();
		SwapChainDesc swapChainDesc{};
		swapChainDesc.width = _size.width();
		swapChainDesc.height = _size.height();
		swapChainDesc.windowHandle = (void*)winId();
		swapChain = renderDevice->CreateSwapChain(swapChainDesc);
	}


}
