#include "pch.h"
#include "../system.h"

namespace vidf
{



	SystemMessageResult UpdateSystemMessages()
	{
		SystemMessageResult result = SMR_Continue;

		MSG msg;
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)){
			if(msg.message == WM_QUIT)
				result = SMR_PostQuit;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return result;
	}



	void SetCurrentDirectory(const char* currentDir)
	{
		::SetCurrentDirectoryA(currentDir);
	}



}
