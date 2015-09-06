#pragma once

#include "vidf/common/types.h"


namespace vidf
{


	class Time
	{
	public:
		Time();
		explicit Time(int64 _ticks);
		explicit Time(float asFloat);

		static int64 GetPlatformTicksPerSecond();
		int64 GetTicks() const {return ticks;}
		float AsFloat() const;
		int AsMiliseconds() const;

	private:
		int64 ticks;
	};



	class TimeCounter
	{
	public:
		TimeCounter();

		Time GetElapsed();

	private:
		Time lastTime;
	};


	Time GetTime();



	inline Time operator+ (Time lhv, Time rhv)
	{
		Time res(lhv.GetTicks() + rhv.GetTicks());
		return res;
	}


	inline Time operator+= (Time& lhv, Time rhv)
	{
		lhv = Time(lhv.GetTicks() + rhv.GetTicks());
		return lhv;
	}


	inline Time operator- (Time lhv, Time rhv)
	{
		Time res(lhv.GetTicks() - rhv.GetTicks());
		return res;
	}


	inline Time operator-= (Time& lhv, Time rhv)
	{
		lhv = Time(lhv.GetTicks() - rhv.GetTicks());
		return lhv;
	}


	inline Time operator* (Time lhv, float fraction)
	{
		Time res(lhv.AsFloat() * fraction);
		return res;
	}


	inline bool operator< (Time lhv, Time rhv)
	{
		bool res = lhv.GetTicks() < rhv.GetTicks();
		return res;
	}


	inline bool operator> (Time lhv, Time rhv)
	{
		bool res = lhv.GetTicks() > rhv.GetTicks();
		return res;
	}

}
