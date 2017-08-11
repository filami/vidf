#pragma once

#include "common.h"


#define VK_FNEXT(ENTRYPOINT, NULLFN) PFN_##ENTRYPOINT ENTRYPOINT = (PFN_##ENTRYPOINT)NULLFN

#define VK_FUNCTIONS(x) \
	x(vkCreateInstance, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyInstance, vidf::VkNullPtrBreakpoint) \
	x(vkEnumeratePhysicalDevices, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceFeatures, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceFormatProperties, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceImageFormatProperties, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceProperties, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceQueueFamilyProperties, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceMemoryProperties, vidf::VkNullPtrBreakpoint) \
	x(vkGetInstanceProcAddr, vidf::VkNullPtrBreakpoint) \
	x(vkGetDeviceProcAddr, vidf::VkNullPtrBreakpoint) \
	x(vkCreateDevice, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyDevice, vidf::VkNullPtrBreakpoint) \
	x(vkEnumerateInstanceExtensionProperties, vidf::VkNullPtrBreakpoint) \
	x(vkEnumerateDeviceExtensionProperties, vidf::VkNullPtrBreakpoint) \
	x(vkEnumerateInstanceLayerProperties, vidf::VkNullPtrBreakpoint) \
	x(vkEnumerateDeviceLayerProperties, vidf::VkNullPtrBreakpoint) \
	x(vkGetDeviceQueue, vidf::VkNullPtrBreakpoint) \
	x(vkQueueSubmit, vidf::VkNullPtrBreakpoint) \
	x(vkQueueWaitIdle, vidf::VkNullPtrBreakpoint) \
	x(vkDeviceWaitIdle, vidf::VkNullPtrBreakpoint) \
	x(vkAllocateMemory, vidf::VkNullPtrBreakpoint) \
	x(vkFreeMemory, vidf::VkNullPtrBreakpoint) \
	x(vkMapMemory, vidf::VkNullPtrBreakpoint) \
	x(vkUnmapMemory, vidf::VkNullPtrBreakpoint) \
	x(vkFlushMappedMemoryRanges, vidf::VkNullPtrBreakpoint) \
	x(vkInvalidateMappedMemoryRanges, vidf::VkNullPtrBreakpoint) \
	x(vkGetDeviceMemoryCommitment, vidf::VkNullPtrBreakpoint) \
	x(vkBindBufferMemory, vidf::VkNullPtrBreakpoint) \
	x(vkBindImageMemory, vidf::VkNullPtrBreakpoint) \
	x(vkGetBufferMemoryRequirements, vidf::VkNullPtrBreakpoint) \
	x(vkGetImageMemoryRequirements, vidf::VkNullPtrBreakpoint) \
	x(vkGetImageSparseMemoryRequirements, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceSparseImageFormatProperties, vidf::VkNullPtrBreakpoint) \
	x(vkQueueBindSparse, vidf::VkNullPtrBreakpoint) \
	x(vkCreateFence, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyFence, vidf::VkNullPtrBreakpoint) \
	x(vkResetFences, vidf::VkNullPtrBreakpoint) \
	x(vkGetFenceStatus, vidf::VkNullPtrBreakpoint) \
	x(vkWaitForFences, vidf::VkNullPtrBreakpoint) \
	x(vkCreateSemaphore, vidf::VkNullPtrBreakpoint) \
	x(vkDestroySemaphore, vidf::VkNullPtrBreakpoint) \
	x(vkCreateEvent, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyEvent, vidf::VkNullPtrBreakpoint) \
	x(vkGetEventStatus, vidf::VkNullPtrBreakpoint) \
	x(vkSetEvent, vidf::VkNullPtrBreakpoint) \
	x(vkResetEvent, vidf::VkNullPtrBreakpoint) \
	x(vkCreateQueryPool, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyQueryPool, vidf::VkNullPtrBreakpoint) \
	x(vkGetQueryPoolResults, vidf::VkNullPtrBreakpoint) \
	x(vkCreateBuffer, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyBuffer, vidf::VkNullPtrBreakpoint) \
	x(vkCreateBufferView, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyBufferView, vidf::VkNullPtrBreakpoint) \
	x(vkCreateImage, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyImage, vidf::VkNullPtrBreakpoint) \
	x(vkGetImageSubresourceLayout, vidf::VkNullPtrBreakpoint) \
	x(vkCreateImageView, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyImageView, vidf::VkNullPtrBreakpoint) \
	x(vkCreateShaderModule, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyShaderModule, vidf::VkNullPtrBreakpoint) \
	x(vkCreatePipelineCache, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyPipelineCache, vidf::VkNullPtrBreakpoint) \
	x(vkGetPipelineCacheData, vidf::VkNullPtrBreakpoint) \
	x(vkMergePipelineCaches, vidf::VkNullPtrBreakpoint) \
	x(vkCreateGraphicsPipelines, vidf::VkNullPtrBreakpoint) \
	x(vkCreateComputePipelines, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyPipeline, vidf::VkNullPtrBreakpoint) \
	x(vkCreatePipelineLayout, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyPipelineLayout, vidf::VkNullPtrBreakpoint) \
	x(vkCreateSampler, vidf::VkNullPtrBreakpoint) \
	x(vkDestroySampler, vidf::VkNullPtrBreakpoint) \
	x(vkCreateDescriptorSetLayout, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyDescriptorSetLayout, vidf::VkNullPtrBreakpoint) \
	x(vkCreateDescriptorPool, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyDescriptorPool, vidf::VkNullPtrBreakpoint) \
	x(vkResetDescriptorPool, vidf::VkNullPtrBreakpoint) \
	x(vkAllocateDescriptorSets, vidf::VkNullPtrBreakpoint) \
	x(vkFreeDescriptorSets, vidf::VkNullPtrBreakpoint) \
	x(vkUpdateDescriptorSets, vidf::VkNullPtrBreakpoint) \
	x(vkCreateFramebuffer, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyFramebuffer, vidf::VkNullPtrBreakpoint) \
	x(vkCreateRenderPass, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyRenderPass, vidf::VkNullPtrBreakpoint) \
	x(vkGetRenderAreaGranularity, vidf::VkNullPtrBreakpoint) \
	x(vkCreateCommandPool, vidf::VkNullPtrBreakpoint) \
	x(vkDestroyCommandPool, vidf::VkNullPtrBreakpoint) \
	x(vkResetCommandPool, vidf::VkNullPtrBreakpoint) \
	x(vkAllocateCommandBuffers, vidf::VkNullPtrBreakpoint) \
	x(vkFreeCommandBuffers, vidf::VkNullPtrBreakpoint) \
	x(vkBeginCommandBuffer, vidf::VkNullPtrBreakpoint) \
	x(vkEndCommandBuffer, vidf::VkNullPtrBreakpoint) \
	x(vkResetCommandBuffer, vidf::VkNullPtrBreakpoint) \
	x(vkCmdBindPipeline, vidf::VkNullPtrBreakpoint) \
	x(vkCmdSetViewport, vidf::VkNullPtrBreakpoint) \
	x(vkCmdSetScissor, vidf::VkNullPtrBreakpoint) \
	x(vkCmdSetLineWidth, vidf::VkNullPtrBreakpoint) \
	x(vkCmdSetDepthBias, vidf::VkNullPtrBreakpoint) \
	x(vkCmdSetBlendConstants, vidf::VkNullPtrBreakpoint) \
	x(vkCmdSetDepthBounds, vidf::VkNullPtrBreakpoint) \
	x(vkCmdSetStencilCompareMask, vidf::VkNullPtrBreakpoint) \
	x(vkCmdSetStencilWriteMask, vidf::VkNullPtrBreakpoint) \
	x(vkCmdSetStencilReference, vidf::VkNullPtrBreakpoint) \
	x(vkCmdBindDescriptorSets, vidf::VkNullPtrBreakpoint) \
	x(vkCmdBindIndexBuffer, vidf::VkNullPtrBreakpoint) \
	x(vkCmdBindVertexBuffers, vidf::VkNullPtrBreakpoint) \
	x(vkCmdDraw, vidf::VkNullPtrBreakpoint) \
	x(vkCmdDrawIndexed, vidf::VkNullPtrBreakpoint) \
	x(vkCmdDrawIndirect, vidf::VkNullPtrBreakpoint) \
	x(vkCmdDrawIndexedIndirect, vidf::VkNullPtrBreakpoint) \
	x(vkCmdDispatch, vidf::VkNullPtrBreakpoint) \
	x(vkCmdDispatchIndirect, vidf::VkNullPtrBreakpoint) \
	x(vkCmdCopyBuffer, vidf::VkNullPtrBreakpoint) \
	x(vkCmdCopyImage, vidf::VkNullPtrBreakpoint) \
	x(vkCmdBlitImage, vidf::VkNullPtrBreakpoint) \
	x(vkCmdCopyBufferToImage, vidf::VkNullPtrBreakpoint) \
	x(vkCmdCopyImageToBuffer, vidf::VkNullPtrBreakpoint) \
	x(vkCmdUpdateBuffer, vidf::VkNullPtrBreakpoint) \
	x(vkCmdFillBuffer, vidf::VkNullPtrBreakpoint) \
	x(vkCmdClearColorImage, vidf::VkNullPtrBreakpoint) \
	x(vkCmdClearDepthStencilImage, vidf::VkNullPtrBreakpoint) \
	x(vkCmdClearAttachments, vidf::VkNullPtrBreakpoint) \
	x(vkCmdResolveImage, vidf::VkNullPtrBreakpoint) \
	x(vkCmdSetEvent, vidf::VkNullPtrBreakpoint) \
	x(vkCmdResetEvent, vidf::VkNullPtrBreakpoint) \
	x(vkCmdWaitEvents, vidf::VkNullPtrBreakpoint) \
	x(vkCmdPipelineBarrier, vidf::VkNullPtrBreakpoint) \
	x(vkCmdBeginQuery, vidf::VkNullPtrBreakpoint) \
	x(vkCmdEndQuery, vidf::VkNullPtrBreakpoint) \
	x(vkCmdResetQueryPool, vidf::VkNullPtrBreakpoint) \
	x(vkCmdWriteTimestamp, vidf::VkNullPtrBreakpoint) \
	x(vkCmdCopyQueryPoolResults, vidf::VkNullPtrBreakpoint) \
	x(vkCmdPushConstants, vidf::VkNullPtrBreakpoint) \
	x(vkCmdBeginRenderPass, vidf::VkNullPtrBreakpoint) \
	x(vkCmdNextSubpass, vidf::VkNullPtrBreakpoint) \
	x(vkCmdEndRenderPass, vidf::VkNullPtrBreakpoint) \
	x(vkCmdExecuteCommands, vidf::VkNullPtrBreakpoint) \

#define VK_FUNCTIONS_WIN32(x) \
	x(vkDestroySurfaceKHR, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceSurfaceSupportKHR, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceSurfaceCapabilitiesKHR, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceSurfaceFormatsKHR, vidf::VkNullPtrBreakpoint) \
	x(vkGetPhysicalDeviceSurfacePresentModesKHR, vidf::VkNullPtrBreakpoint) \
	x(vkCreateWin32SurfaceKHR, vidf::VkNullPtrBreakpoint) \
	x(vkCreateSwapchainKHR, vidf::VkNullPtrBreakpoint) \
	x(vkGetSwapchainImagesKHR, vidf::VkNullPtrBreakpoint) \
	x(vkAcquireNextImageKHR, vidf::VkNullPtrBreakpoint) \
	x(vkQueuePresentKHR, vidf::VkNullPtrBreakpoint) \

#define VK_DECLARE_FUNCTIONS(ENTRYPOINT, NULLFN) extern PFN_##ENTRYPOINT ENTRYPOINT;
VK_FUNCTIONS(VK_DECLARE_FUNCTIONS);
VK_FUNCTIONS_WIN32(VK_DECLARE_FUNCTIONS);

namespace vidf
{
	bool BindToVulkanLib();

	VkResult VkNullPtrSuccess();
	VkResult VkNullPtrBreakpoint();


	struct VkExtDebugMarker
	{
		VK_FNEXT(vkDebugMarkerSetObjectTagEXT,  VkNullPtrSuccess);
		VK_FNEXT(vkDebugMarkerSetObjectNameEXT, VkNullPtrSuccess);
		VK_FNEXT(vkCmdDebugMarkerBeginEXT,      VkNullPtrSuccess);
		VK_FNEXT(vkCmdDebugMarkerEndEXT,        VkNullPtrSuccess);
		VK_FNEXT(vkCmdDebugMarkerInsertEXT,     VkNullPtrSuccess);
		operator bool() const { return vkDebugMarkerSetObjectTagEXT != (PFN_vkDebugMarkerSetObjectTagEXT)VkNullPtrSuccess; };
	};
	const VkExtDebugMarker& GetVkExtDebugMarker();
	const VkExtDebugMarker& GetVkExtDebugMarker(RenderDevicePtr device);


}
