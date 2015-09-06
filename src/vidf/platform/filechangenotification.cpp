#include "pch.h"
#include "filesystemutils.h"
#include "filechangenotification.h"



namespace vidf
{


	void FileChangeNotificationSystem::DispatchNotifications()
	{
		queue([&](Queue& queue)
		{
			for (auto fileName : queue)
			{
				auto it = notifications.find(fileName);
				if (it != notifications.end())
					it->second->Notify(fileName.c_str());
			}
			queue.clear();
		});
	}



	void FileChangeNotificationSystem::AddFileNotification(const wchar_t* fileName, FileChangeNotification* notification)
	{
		notifications[StandardPath(fileName)] = notification;
	}



	void FileChangeNotificationSystem::RemoveFileNotification(const wchar_t* fileName)
	{
		auto it = notifications.find(StandardPath(fileName));
		if (it != notifications.end())
			notifications.erase(it);
	}



	void FileChangeNotificationSystem::PushNotification(const wchar_t* filename)
	{
		queue([=](Queue& queue){queue.insert(StandardPath(filename));});
	}
	

}
