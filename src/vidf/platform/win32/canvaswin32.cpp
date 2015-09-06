#include "pch.h"
#include "platform/canvas.h"


namespace vidf
{


	namespace
	{

		const char* canvasClassName = "VIDFCanvasClass";



		KeyCode ToKeyCode(WPARAM virtualKey)
		{
			return static_cast<KeyCode>(virtualKey);
		}



		LRESULT CALLBACK CanvasProc(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
		{
			LONG_PTR userData = GetWindowLongPtr(window, GWLP_USERDATA);
			Canvas* canvas = reinterpret_cast<Canvas*>(userData);

			Vector2i point;
			point.x = LOWORD(lparam);
			point.y = HIWORD(lparam);

			switch (msg)
			{
				case WM_CREATE:
					{
						CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lparam);
						LONG_PTR userData = *static_cast<LONG_PTR*>(createStruct->lpCreateParams);
						SetWindowLongPtr(window, GWLP_USERDATA, userData);
					}
					break;

				case WM_CLOSE:
				case WM_DESTROY:
					{
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->Close();
					}
					break;
				case WM_KEYDOWN:
					{
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->KeyDown(ToKeyCode(wparam));
					}
					break;
				case WM_KEYUP:
					{
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->KeyUp(ToKeyCode(wparam));
					}
					break;

				case WM_LBUTTONDOWN:
					{
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->LeftMouseDown(point);
					}
					break;
				case WM_LBUTTONUP:
					{
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->LeftMouseUp(point);
					}
					break;
				case WM_MBUTTONDOWN:
					{
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->MiddleMouseDown(point);
					}
					break;
				case WM_MBUTTONUP:
					{
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->MiddleMouseUp(point);
					}
					break;
				case WM_RBUTTONDOWN:
					{
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->RightMouseDown(point);
					}
					break;
				case WM_RBUTTONUP:
					{
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->RightMouseUp(point);
					}
					break;
				case WM_MOUSEMOVE:
					{
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->MouseMove(point);
					}
					break;
				case WM_MOUSEWHEEL:
					{
						const float divisionSize = 120;
						short wheel = ((short)HIWORD(wparam));
						float delta = wheel / divisionSize;
						for (int i = 0; i < canvas->GetNumListeners(); ++i)
							canvas->GetListener(i)->MouseWheel(delta);
					}
					break;
			}

			return DefWindowProc(window, msg, wparam, lparam);
		}



		class CanvasClass
		{
		public:
			CanvasClass();
			~CanvasClass();

			bool IsValid() const {return valid;}

		private:
			bool valid;
		};



		CanvasClass::CanvasClass()
			:	valid(false)
		{
			HINSTANCE hInstance = GetModuleHandle(0);

			WNDCLASSA wnd = {0};
			wnd.hCursor = LoadCursor(NULL, IDC_ARROW);
			wnd.hIcon = LoadIcon(NULL, IDI_APPLICATION);
			wnd.hInstance = hInstance;
			wnd.lpfnWndProc = (WNDPROC)CanvasProc;
			wnd.lpszClassName = canvasClassName;
			wnd.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
			if (RegisterClassA(&wnd))
				valid = true;
		}



		CanvasClass::~CanvasClass()
		{
			HINSTANCE hInstance = GetModuleHandle(0);
			UnregisterClassA(canvasClassName, hInstance);
		}



		bool CreateCanvasClass()
		{
			static CanvasClass canvasClass;
			return canvasClass.IsValid();
		}



		void* CreateCanvasWindow(const CanvasDesc& desc, Canvas_ptr owner, bool simpleFrame)
		{
			HINSTANCE hInstance = GetModuleHandle(0);

			long style = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX;
			if (simpleFrame)
				style = WS_POPUP;
			LPARAM lparam = reinterpret_cast<LPARAM>(owner.get());
			HWND hwnd = CreateWindowA(
				canvasClassName, desc.caption.c_str(),
				style,
				0, 0,
				desc.width, desc.height,
				0, 0, hInstance, &lparam);

			if (!hwnd)
				return 0;

			UpdateWindow(hwnd);
			ShowWindow(hwnd, SW_SHOW);

			return static_cast<void*>(hwnd);
		}



		void ResizeCanvasWindow(const CanvasDesc& desc, void* handle)
		{
			HWND hwnd = static_cast<HWND>(handle);
			RECT windowRect;
			RECT clientRect;
			GetWindowRect(hwnd, &windowRect);
			GetClientRect(hwnd, &clientRect);
			SetWindowPos(
				hwnd, 0,
				windowRect.left, windowRect.top,
				windowRect.right + (desc.width-(clientRect.right-clientRect.left)),
				windowRect.bottom + (desc.height-(clientRect.bottom-clientRect.top)),
				0);
		}


	}



	Canvas_ptr Canvas::CreateCanvas(const CanvasDesc& desc)
	{
		if (!CreateCanvasClass())
			return Canvas_ptr();

		Canvas_ptr canvas(new Canvas(desc));

		void* handle = CreateCanvasWindow(desc, canvas, desc.simpleFrame);
		if (!handle)
			return Canvas_ptr();
		ResizeCanvasWindow(desc, handle);
		canvas->handle = handle;
		canvas->deviceContext = GetDC((HWND)handle);

		return canvas;
	}



	Canvas::Canvas(const CanvasDesc& desc)
		:	handle(0)
		,	deviceContext(0)
		,	canvasDesc(desc)
	{
	}



	Canvas::~Canvas()
	{
		HWND hwnd = static_cast<HWND>(handle);
		if (hwnd)
			DestroyWindow(hwnd);
	}


}

