#include "pch.h"
#include "../time.h"


namespace vidf
{


	namespace
	{


		class TimeContext
		{
		public:
			TimeContext();
			int64 GetTicksPerSecond() const {return systemFrequency;}
			int64 GetTicks() const;
			float AsFloat(int64 ticks) const;
			int AsMiliseconds(int64 ticks) const;
			int64 FromFloat(float value) const;

		private:
			int64 systemFrequency;
			int64 systemCounter;
			int64 startSystemTime;
		};


		TimeContext::TimeContext()
		{
			LARGE_INTEGER freq;
			BOOL res = QueryPerformanceFrequency(&freq);
			systemFrequency = freq.QuadPart;

			LARGE_INTEGER counter;
			res = QueryPerformanceCounter(&counter);
			systemCounter = counter.QuadPart;

			FILETIME fileTime;
			GetSystemTimeAsFileTime(&fileTime);	
			LARGE_INTEGER convert;
			convert.LowPart = fileTime.dwLowDateTime;
			convert.HighPart = fileTime.dwHighDateTime;
			startSystemTime = convert.QuadPart;
		}


		int64 TimeContext::GetTicks() const
		{
			LARGE_INTEGER counter;
			QueryPerformanceCounter(&counter);
			int64 ticks = counter.QuadPart - systemCounter;
			return ticks;
		}


		float TimeContext::AsFloat(int64 ticks) const
		{
			float time = (float)((double)ticks / (double)systemFrequency);
			return time;
		}


		int TimeContext::AsMiliseconds(int64 ticks) const
		{
			int time = int(ticks / (systemFrequency/1000));
			return time;
		}


		int64 TimeContext::FromFloat(float value) const
		{
			return static_cast<int64>(systemFrequency * value);
		}



		const TimeContext& GetTimeContext()
		{
			static TimeContext timeContext;
			return timeContext;
		}

	}



	Time GetTime()
	{
		return Time(GetTimeContext().GetTicks());
	}



	Time::Time(float asFloat)
		:	ticks(GetTimeContext().FromFloat(asFloat))
	{
	}



	int64 Time::GetPlatformTicksPerSecond()
	{
		return GetTimeContext().GetTicksPerSecond();
	}



	float Time::AsFloat() const
	{
		return GetTimeContext().AsFloat(ticks);
	}



	int Time::AsMiliseconds() const
	{
		return GetTimeContext().AsMiliseconds(ticks);
	}


}
