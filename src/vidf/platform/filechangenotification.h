#pragma once
#include "taskmanager.h"


namespace vidf
{


	class FileChangeNotificationSystem;



	class FileChangeNotification
	{
	public:
		virtual void Notify(const wchar_t* fileName) = 0;
	};



	class FileChangeNotificationSystem
	{
	private:
		typedef std::set<std::wstring> Queue;

	public:
		void StartNotificationListener();
		void DispatchNotifications();

		void AddFileNotification(const wchar_t* fileName, FileChangeNotification* notification);
		void RemoveFileNotification(const wchar_t* fileName);
		void PushNotification(const wchar_t* filename);

	private:
		std::map<std::wstring, FileChangeNotification*> notifications;
		Monitor<Queue> queue;
	};



}
