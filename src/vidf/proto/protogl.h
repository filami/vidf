#pragma once

#include "vidf/common/color.h"
#include "vidf/common/aspectratio.h"
#include "vidf/platform/canvas.h"


namespace vidf { namespace proto {



	struct ProtoGLDesc
	{
		ProtoGLDesc() : caption("ProtoGL"), width(800), height(600), color(0.2f, 0.0f, 0.0f, 1.0f), fullscreen(false) {}
		ProtoGLDesc(int _width, int _height) : caption("ProtoGL"), width(_width), height(_height), color(0.2f, 0.0f, 0.0f, 1.0f), fullscreen(false) {}

		std::string caption;
		int width;
		int height;
		Color color;
		bool fullscreen;
	};



	class ProtoGLDevice
	{
	public:
		ProtoGLDevice();
		~ProtoGLDevice();

		bool Initialize(void* windowHandle);
		void Swap() const;

		HDC GetDeviceContextHandle() const {return hdc;}

	private:
		HDC hdc;
		HGLRC hglrc;
	};



	class ProtoGL
	{
	public:
		ProtoGL();
		~ProtoGL();

		bool Initialize(const ProtoGLDesc& desc);
		bool Update();
		void Swap() const;
		const ProtoGLDesc& GetProtoGLDesc() const {return protoGLDesc;}
		const AspectRatio& GetAspectRatio() const {return aspectRatio;}

		Canvas_ptr GetCanvas() const {return canvas;}

	private:
		ProtoGLDevice device;
		ProtoGLDesc protoGLDesc;
		Canvas_ptr canvas;
		AspectRatio aspectRatio;
		CanvasListener* canvasListener;
	};



} }
