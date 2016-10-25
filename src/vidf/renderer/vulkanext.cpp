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


#define VK_INITIALIZE_FUNCTIONS(ENTRYPOINT, NULLFN) PFN_##ENTRYPOINT ENTRYPOINT = (PFN_##ENTRYPOINT)NULLFN;
VK_FUNCTIONS(VK_INITIALIZE_FUNCTIONS);
VK_FUNCTIONS_WIN32(VK_INITIALIZE_FUNCTIONS);


namespace vidf
{

	bool BindToVulkanLib()
	{
		static bool done = false;
		static bool success = false;
		if (done)
			return success;
		HMODULE vulkanLib = LoadLibraryA("vulkan-1.dll");
		if (!vulkanLib)
		{
			success = false;
			return success;
		}
#		define VK_LOAD_FUNCTIONS(ENTRYPOINT, NULLFN) ENTRYPOINT = (PFN_##ENTRYPOINT)GetProcAddress(vulkanLib, #ENTRYPOINT);
		VK_FUNCTIONS(VK_LOAD_FUNCTIONS);
		VK_FUNCTIONS_WIN32(VK_LOAD_FUNCTIONS);
		done = true;
		success = true;
		return success;
	}



	VkResult VkNullPtrSuccess()
	{
		return VK_SUCCESS;
	}



	VkResult VkNullPtrBreakpoint()
	{
	//	__debugbreak();
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
