#pragma once


namespace vidf
{



	enum class LogLevel
	{
		Info,
		Warning,
		Error,
		Fatal,
	};



	class LogListener
	{
	public:
		LogListener();
		virtual ~LogListener();
		virtual void Log(LogLevel logLevel, const char* _text) = 0;
	};


	class Logger
	{
	private:
		struct Line
		{
			uint begin;
			uint end;
		};

	public:
		static Logger* Get();

		template<typename ... Args>
		void Log(LogLevel logLevel, const char* _text, Args ... args)
		{
		//	wchar_t buffer[256];
		//	Format(buffer, _text, args...);
		//	Log(logLevel, buffer);
			Log(logLevel, _text);
		}

		void Log(LogLevel logLevel, const char* _text);

	private:
		void CommitLine(LogLevel logLevel, const string& _text);
		void Terminate();

	private:
		friend class LogListener;
		deque<Line> lines;
		deque<char> text;
		deque<LogListener*> listeners;
		std::mutex  mtx;
	};



#define VI_INFO(outputStr, ...)    { vidf::Logger::Get()->Log(LogLevel::Info,    outputStr, __VA_ARGS__); }
#define VI_WARNING(outputStr, ...) { vidf::Logger::Get()->Log(LogLevel::Warning, outputStr, __VA_ARGS__); }
#define VI_ERROR(outputStr, ...)   { vidf::Logger::Get()->Log(LogLevel::Error,   outputStr, __VA_ARGS__); }
#define VI_FATAL(outputStr, ...)   { vidf::Logger::Get()->Log(LogLevel::Fatal,   outputStr, __VA_ARGS__); }



}
