#pragma once

#include "common.h"
#include "renderdevice.h"

namespace vidf { namespace dx11 {



	class GpuTimer
	{
	public:
		typedef uint TimeStampId;

	public:
		GpuTimer(RenderDevicePtr _renderDevice);

		void Begin();
		void End();
		
		TimeStampId MakeNewTimeStamp();
		uint64 GetTimeStamp(TimeStampId timeStampId) const { return timeStamps[timeStampId]; }
		uint64 GetFrequency() const { return frequency; }
		void PushTimeStamp(TimeStampId timeStampId);

	private:
		friend class GpuSection;
		RenderDevicePtr renderDevice;
		std::vector<uint64> timeStamps;
		std::vector<PD3D11Query> queries;
		PD3D11Query frequencyQuery;
		uint64 frequency = 1;
		bool quering = false;
	};


	class GpuSection
	{
	public:
		GpuSection(GpuTimer* _timer);

		void Begin() const { timer->PushTimeStamp(beginId); }
		void End() const { timer->PushTimeStamp(endId); }
		float AsFloat() const;

	private:
		GpuTimer* timer;
		GpuTimer::TimeStampId beginId;
		GpuTimer::TimeStampId endId;
	};


	class GpuAutoSection
	{
	public:
		GpuAutoSection(GpuSection& _section)
			: section(_section)
		{
			section.Begin();
		}
		~GpuAutoSection()
		{
			section.End();
		}

	private:
		GpuSection& section;
	};


} }
