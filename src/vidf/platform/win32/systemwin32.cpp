#include "pch.h"
#include "../system.h"

namespace vidf
{


	void PostQuitMessage()
	{
		::PostQuitMessage(0);
	}



	SystemMessageResult UpdateSystemMessages()
	{
		SystemMessageResult result = SystemMessageResult::Continue;

		MSG msg;
		while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE)){
			if(msg.message == WM_QUIT)
				result = SystemMessageResult::Quit;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		return result;
	}



	void SetCurrentDirectory(const char* currentDir)
	{
		::SetCurrentDirectoryA(currentDir);
	}



	void MakeFolder(const char* folderName)
	{
	}



}
