#include "pch.h"
#include "vulkanext.h"
#include "renderdevice.h"

#define GET_DEVICE_PROC_ADDR(DEVICE, STRUCT, ENTRYPOINT)               \
{                                                                       \
    ext.ENTRYPOINT = (PFN_##ENTRYPOINT) vkGetDeviceProcAddr(DEVICE, #ENTRYPOINT);   \
    if (ext.ENTRYPOINT == nullptr)                                      \
	{																    \
		ext = STRUCT();                                                 \
        return ext;                                                     \
    }                                                                   \
}


namespace vidf
{


	VkResult VkNullPtrSuccess()
	{
		return VK_SUCCESS;
	}

	VkResult VkNullPtrBreakpoint()
	{
		__debugbreak();
		return VK_ERROR_FEATURE_NOT_PRESENT;
	}


	const VkExtDebugMarker& GetVkExtDebugMarker()
	{
		static VkExtDebugMarker ext;
		return ext;
	}

	const VkExtDebugMarker& GetVkExtDebugMarker(RenderDevicePtr device)
	{
		static bool done = false;
		VkExtDebugMarker& ext = const_cast<VkExtDebugMarker&>(GetVkExtDebugMarker()); // safe
		if (!done)
		{
			done = true;
			VkDevice _device = device->GetDevice();
			GET_DEVICE_PROC_ADDR(_device, VkExtDebugMarker, vkDebugMarkerSetObjectTagEXT);
			GET_DEVICE_PROC_ADDR(_device, VkExtDebugMarker, vkDebugMarkerSetObjectNameEXT);
			GET_DEVICE_PROC_ADDR(_device, VkExtDebugMarker, vkCmdDebugMarkerBeginEXT);
			GET_DEVICE_PROC_ADDR(_device, VkExtDebugMarker, vkCmdDebugMarkerEndEXT);
			GET_DEVICE_PROC_ADDR(_device, VkExtDebugMarker, vkCmdDebugMarkerInsertEXT);
		}
		return ext;
	}


}
