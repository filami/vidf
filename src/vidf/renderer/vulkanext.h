#pragma once

#include "common.h"


#define VK_FNEXT(ENTRYPOINT, NULLFN) PFN_##ENTRYPOINT ENTRYPOINT = (PFN_##ENTRYPOINT)NULLFN


namespace vidf
{

	VkResult VkNullPtrSuccess();
	VkResult VkNullPtrBreakpoint();


	struct VkExtDebugMarker
	{
		VK_FNEXT(vkDebugMarkerSetObjectTagEXT, VkNullPtrSuccess);
		VK_FNEXT(vkDebugMarkerSetObjectNameEXT, VkNullPtrSuccess);
		VK_FNEXT(vkCmdDebugMarkerBeginEXT, VkNullPtrSuccess);
		VK_FNEXT(vkCmdDebugMarkerEndEXT, VkNullPtrSuccess);
		VK_FNEXT(vkCmdDebugMarkerInsertEXT, VkNullPtrSuccess);
		operator bool() const { return vkDebugMarkerSetObjectTagEXT != (PFN_vkDebugMarkerSetObjectTagEXT)VkNullPtrSuccess; };
	};
	const VkExtDebugMarker& GetVkExtDebugMarker();
	const VkExtDebugMarker& GetVkExtDebugMarker(RenderDevicePtr device);


}
