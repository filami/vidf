#include "pch.h"
#include "canvas.h"


namespace vidf
{



	void Canvas::AddListener(CanvasListener* listener)
	{
		listeners.push_back(listener);
	}



	void Canvas::RemoveListener(CanvasListener* listener)
	{
		std::vector<CanvasListener*>::iterator it =
			std::find(listeners.begin(), listeners.end(), listener);
		if (it == listeners.end())
			return;
		listeners.erase(it);
	}



	void CanvasListener::Close() {}
	void CanvasListener::KeyDown(KeyCode keyCode) {keyCode;}
	void CanvasListener::KeyUp(KeyCode keyCode) {keyCode;}
	void CanvasListener::LeftMouseDown(Vector2i point) {point;}
	void CanvasListener::LeftMouseUp(Vector2i point) {point;}
	void CanvasListener::MiddleMouseDown(Vector2i point) {point;}
	void CanvasListener::MiddleMouseUp(Vector2i point) {point;}
	void CanvasListener::RightMouseDown(Vector2i point) {point;}
	void CanvasListener::RightMouseUp(Vector2i point) {point;}
	void CanvasListener::MouseMove(Vector2i point) {point;}
	void CanvasListener::MouseWheel(float delta) {delta;}


}
