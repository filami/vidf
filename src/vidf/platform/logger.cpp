#include "pch.h"
#include "logger.h"



namespace vidf
{



	Logger* Logger::Get()
	{
		static Logger logger;
		return &logger;
	}



	void Logger::Log(LogLevel logLevel, const wchar_t* _text)
	{
		std::unique_lock<std::mutex> loc(mtx);

		static wstring str;
		str.clear();
				
		for (; *_text != 0; ++_text)
			str.push_back(*_text);
		CommitLine(logLevel, str);

		if (logLevel == LogLevel::Fatal)
			Terminate();
	}



	void Logger::CommitLine(LogLevel logLevel, const wstring& _text)
	{
		Line line;
		line.begin = text.size();
		line.end = line.begin + _text.size();
		text.insert(text.end(), _text.begin(), _text.end());
		lines.push_back(line);

		wcout << _text;
		OutputDebugStringW(_text.c_str());
	}



	void Logger::Terminate()
	{
		__debugbreak();
		exit(-1);
	}


}
