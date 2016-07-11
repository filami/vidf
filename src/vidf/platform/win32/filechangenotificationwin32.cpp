#include "pch.h"
#include "../filechangenotification.h"



namespace
{


	DWORD __cdecl DirectoryChangeNotificationThread(LPVOID param)
	{
		vidf::FileChangeNotificationSystem* fileNotification = reinterpret_cast<vidf::FileChangeNotificationSystem*>(param);
		DWORD notificationFilter = FILE_NOTIFY_CHANGE_LAST_WRITE;

		WCHAR cCurrentPath[FILENAME_MAX];
		wchar_t filePath[FILENAME_MAX];
		::GetCurrentDirectoryW(FILENAME_MAX, cCurrentPath);

		HANDLE directory = ::CreateFileW(cCurrentPath, GENERIC_READ | FILE_LIST_DIRECTORY, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
		HANDLE notify = FindFirstChangeNotificationW(cCurrentPath, TRUE, notificationFilter);

		while (true)
		{
			WaitForSingleObject(notify, INFINITE);

			BYTE  fni[32 * 1024];
			DWORD offset = 0;
			DWORD bytesret;
			PFILE_NOTIFY_INFORMATION pNotify;

			ReadDirectoryChangesW(directory, fni, sizeof(fni), TRUE, notificationFilter, &bytesret, NULL, NULL);
			do
			{
				pNotify = (PFILE_NOTIFY_INFORMATION)&fni[offset];
				offset += pNotify->NextEntryOffset;

				if (pNotify->Action == FILE_ACTION_MODIFIED)
				{
					int i = 0;
					const size_t count = pNotify->FileNameLength / sizeof(wchar_t);
					for (const WCHAR* wFilePath = pNotify->FileName; i < count;)
						filePath[i++] = *(wFilePath++);
					filePath[i] = 0;
					fileNotification->PushNotification(filePath);
				}

			} while (pNotify->NextEntryOffset != 0);
			FindNextChangeNotification(notify);
		}

		return 0;
	}
	

}



namespace vidf
{



	void FileChangeNotificationSystem::StartNotificationListener()
	{
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&DirectoryChangeNotificationThread, this, 0, 0);
	}



}
