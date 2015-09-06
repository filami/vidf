#pragma once


namespace vidf
{



	template<typename T>
	class Monitor
	{
	public:
		Monitor() {}
		explicit Monitor(T _t) : t(_t) {}

		template<typename F>
		auto operator()(F f) const -> decltype(f(t))
		{
			std::lock_guard<std::mutex> hold(m);
			return f(t);
		}

	private:
		mutable T t;
		mutable std::mutex m;
	};
	


	class TaskManager
	{
	public:
		typedef std::function<void()> Task;
		typedef std::deque<Task> Tasks;

	public:
		TaskManager(int numThreads)
			:	threads(numThreads)
			,	done(false)
		{
		/*	int threadId = 0;
			for (auto& thread : threads)
			{
				thread = std::thread([=,this]{TaskLoop(this, threadId);});
				++threadId;
			}*/
		}

		~TaskManager()
		{
		/*	WaitForAll();
			done = true;
			newTaskNotify.notify_all();
			for (auto& thread : threads)
			{
				thread.join();
			}*/
		}

		void AddTask(Task task)
		{
			tasks([&task](Tasks& _tasks){_tasks.push_back(task);});
			newTaskNotify.notify_one();
		}

		void WaitForAll()
		{
			while (HasTasksLeft())
			{
				std::unique_lock<std::mutex> removedTaskLoc(removedTaskMutex);
				removedTaskNotify.wait(removedTaskLoc);
			}
		}

	private:
		bool HasTasksLeft() const
		{
			bool hasTasksLeft = tasks([](Tasks& _tasks)
			{
				bool hasTasks = !_tasks.empty();
				return hasTasks;
			});
			return hasTasksLeft;
		}

		Task PopTask()
		{
			return tasks([this](Tasks& _tasks)
			{
				Task task = _tasks.front();
				_tasks.pop_front();
				removedTaskNotify.notify_all();
				return task;
			});
		}

		static void TaskLoop(TaskManager* taskManager)
		{
			while (!taskManager->done)
			{
				if (taskManager->HasTasksLeft())
				{
					Task task = taskManager->PopTask();
					task();
				}
				else
				{
					std::unique_lock<std::mutex> newTaskLoc(taskManager->newTaskMutex);
					taskManager->newTaskNotify.wait(newTaskLoc);
				}
			}
		}

		std::vector<std::thread> threads;
		Monitor<Tasks> tasks;

		std::mutex newTaskMutex;
		std::condition_variable newTaskNotify;

		std::mutex removedTaskMutex;
		std::condition_variable removedTaskNotify;

		bool done;
	};



}
