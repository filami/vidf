#include "pch.h"
#include "logger.h"



namespace vidf
{



	LogListener::LogListener()
	{
		vidf::Logger::Get()->listeners.push_back(this);
	}



	LogListener::~LogListener()
	{
		auto& listeners = vidf::Logger::Get()->listeners;
		listeners.erase(find(listeners.begin(), listeners.end(), this));
	}



	Logger* Logger::Get()
	{
		static Logger logger;
		return &logger;
	}



	void Logger::Log(LogLevel logLevel, const char* _text)
	{
		std::unique_lock<std::mutex> loc(mtx);

		for (auto& listener : listeners)
			listener->Log(logLevel, _text);

		static string str;
		str.clear();
				
		for (; *_text != 0; ++_text)
			str.push_back(*_text);
		CommitLine(logLevel, str);

		if (logLevel == LogLevel::Fatal)
			Terminate();
	}



	void Logger::CommitLine(LogLevel logLevel, const string& _text)
	{
		Line line;
		line.begin = text.size();
		line.end = line.begin + _text.size();
		text.insert(text.end(), _text.begin(), _text.end());
		lines.push_back(line);

		cout << _text;
		OutputDebugStringA(_text.c_str());
	}



	void Logger::Terminate()
	{
		__debugbreak();
		exit(-1);
	}


}
