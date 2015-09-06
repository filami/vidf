#pragma once

#include "vidf/common/vector2.h"
#include "system.h"


namespace vidf
{


	class Canvas;
	typedef std::shared_ptr<Canvas> Canvas_ptr;



	struct CanvasDesc
	{
		CanvasDesc(int _width=800, int _height=600, const std::string& _caption="vidf")
			:	width(_width)
			,	height(_height)
			,	caption(_caption)
			,	simpleFrame(false) {}

		std::string caption;
		int width;
		int height;
		bool simpleFrame;
	};




	class CanvasListener
	{
	public:
		virtual void Close();
		virtual void KeyDown(KeyCode keyCode);
		virtual void KeyUp(KeyCode keyCode);
		virtual void LeftMouseDown(Vector2i point);
		virtual void LeftMouseUp(Vector2i point);
		virtual void MiddleMouseDown(Vector2i point);
		virtual void MiddleMouseUp(Vector2i point);
		virtual void RightMouseDown(Vector2i point);
		virtual void RightMouseUp(Vector2i point);
		virtual void MouseMove(Vector2i point);
		virtual void MouseWheel(float delta);
	};



	class Canvas
	{
	public:
		~Canvas();

		static Canvas_ptr CreateCanvas(const CanvasDesc& desc);
		const CanvasDesc& GetCanvasDesc() const {return canvasDesc;}
		void* GetHandle() const {return handle;}
		void* GetDeviceContext() const {return deviceContext;}

		void AddListener(CanvasListener* listener);
		void RemoveListener(CanvasListener* listener);
		int GetNumListeners() const {return int(listeners.size());}
		CanvasListener* GetListener(int idx) const {return listeners[idx];}

	private:
		Canvas(const CanvasDesc& desc);

		std::vector<CanvasListener*> listeners;
		CanvasDesc canvasDesc;
		void* handle;
		void* deviceContext;
	};


}
