#include "pch.h"
#include "time.h"



namespace vidf
{


	Time::Time()
		:	ticks(0)
	{
	}



	Time::Time(int64 _ticks)
		:	ticks(_ticks)
	{
	}



	TimeCounter::TimeCounter()
		:	lastTime(GetTime())
	{
	}


	Time TimeCounter::GetElapsed()
	{
		Time current = GetTime();
		Time elapsed = current - lastTime;
		lastTime = current;
		return elapsed;
	}


}
