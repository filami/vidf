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

	void Log(LogLevel logLevel, const char* _text);
	void Log(LogLevel logLevel, const string& _text)
	{
		Log(logLevel, _text.c_str());
	}

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



#define VI_INFO(outputStr)    { vidf::Logger::Get()->Log(LogLevel::Info,    outputStr); }
#define VI_WARNING(outputStr) { vidf::Logger::Get()->Log(LogLevel::Warning, outputStr); }
#define VI_ERROR(outputStr)   { vidf::Logger::Get()->Log(LogLevel::Error,   outputStr); }
#define VI_FATAL(outputStr)   { vidf::Logger::Get()->Log(LogLevel::Fatal,   outputStr); }



}
