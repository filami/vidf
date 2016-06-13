#include "pch.h"
#include "protogl.h"


namespace vidf { namespace proto {


	namespace
	{



		class ProtoCanvasListener : public CanvasListener
		{
		public:
			virtual void Close()
			{
				PostQuitMessage(0);
			}
		};



		std::vector<Recti> monitorRects;



		BOOL CALLBACK MyInfoEnumProc(HMONITOR /*hMonitor*/, HDC /*hdcMonitor*/, LPRECT lprcMonitor, LPARAM /*dwData*/)
		{
			Recti monitorRect;
			monitorRect.min = Vector2i(lprcMonitor->left, lprcMonitor->top);
			monitorRect.max = Vector2i(lprcMonitor->right, lprcMonitor->bottom);
			monitorRects.push_back(monitorRect);
			return true;
		}


	}



	ProtoGLDevice::ProtoGLDevice()
	{
	}



	ProtoGLDevice::~ProtoGLDevice()
	{
		if (hglrc)
			wglDeleteContext(hglrc);
	}



	bool ProtoGLDevice::Initialize(void* windowHandle)
	{
		hdc = GetDC((HWND)windowHandle);
		PIXELFORMATDESCRIPTOR pfd;
		memset(&pfd,0,sizeof(PIXELFORMATDESCRIPTOR));
		pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER; 
		pfd.dwLayerMask = PFD_MAIN_PLANE;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 24;
		pfd.cStencilBits = 8;
		int pixelFormat = ChoosePixelFormat(hdc, &pfd);
		if(!pixelFormat)
			return false;
		SetPixelFormat(hdc, pixelFormat, &pfd);

		hglrc = wglCreateContext(hdc);
		wglMakeCurrent(hdc, hglrc);

		return true;
	}



	void ProtoGLDevice::Swap() const
	{
		SwapBuffers(hdc);
	}




	ProtoGL::ProtoGL()
		:	canvasListener(0)
	{
	}



	ProtoGL::~ProtoGL()
	{
		if (canvas && canvasListener)
			canvas->RemoveListener(canvasListener);
	}



	bool ProtoGL::Initialize(const ProtoGLDesc& desc)
	{
		CanvasDesc canvasDesc;
		canvasDesc.caption = desc.caption;
		canvasDesc.width = desc.width;
		canvasDesc.height = desc.height;
		canvasDesc.simpleFrame = desc.fullscreen;

		canvas = Canvas::Create(canvasDesc);
		if (!canvas)
			return false;
		canvasListener = new ProtoCanvasListener();
		canvas->AddListener(canvasListener);

		if (desc.fullscreen)
		{
			const int displayIdx = 0;

			EnumDisplayMonitors(NULL, NULL, MyInfoEnumProc, 0);  

			DEVMODE dmScreenSettings;
    		memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
    		dmScreenSettings.dmSize=sizeof(dmScreenSettings);
			dmScreenSettings.dmPelsWidth = desc.width;
			dmScreenSettings.dmPelsHeight = desc.height;
    		dmScreenSettings.dmBitsPerPel = 32;
    		dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|DM_PELSHEIGHT;

			DISPLAY_DEVICE displayDevice = {0};
			displayDevice.cb = sizeof(displayDevice);
			EnumDisplayDevices(0, displayIdx, &displayDevice, EDD_GET_DEVICE_INTERFACE_NAME);

			ChangeDisplaySettingsEx(displayDevice.DeviceName, &dmScreenSettings, 0, CDS_FULLSCREEN, 0);

			HWND hwnd = reinterpret_cast<HWND>(canvas->GetHandle());
			Recti monitorRect = monitorRects[displayIdx];
			SetWindowPos(
				hwnd, 0,
				monitorRect.min.x, monitorRect.min.y,
				desc.width, desc.height,
				0);
		}

		if (!device.Initialize(canvas->GetHandle()))
			return false;

		glClearColor(desc.color.R(), desc.color.G(), desc.color.B(), desc.color.A());
		aspectRatio = AspectRatio(desc.width, desc.height);

		protoGLDesc = desc;
		return true;
	}



	bool ProtoGL::Update()
	{
		MSG msg;
		HWND focusWindow = GetFocus();
		HWND windowHandle = static_cast<HWND>(canvas->GetHandle());
		if (focusWindow == windowHandle)
		{
			while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					return false;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{
			while (GetMessage(&msg, 0, 0, 0))
			{
				if (msg.message == WM_QUIT)
					return false;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
				focusWindow = GetFocus();
				if (focusWindow == windowHandle)
					return true;
			}
			return false;
		}
		return true;
	}



	void ProtoGL::Swap() const
	{
		device.Swap();
	}


} }
