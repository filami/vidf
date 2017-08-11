#pragma once

#include "common.h"

namespace vidf { namespace dx11
{



	class Event
	{
	public:
		Event(RenderDevicePtr _device, LPCWSTR name)
			: device(_device)
		{
			device->GetUserAnnotations()->BeginEvent(name);
		}

		~Event()
		{
			device->GetUserAnnotations()->EndEvent();
		}

	private:
		RenderDevicePtr device;
	};


#	define VIDF_GPU_EVENT(DEVICE,NAME) Event __event ## NAME (DEVICE,L#NAME)



} }
