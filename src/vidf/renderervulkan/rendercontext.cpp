#include "pch.h"
#include "common.h"
#include "rendercontext.h"
#include "renderdevice.h"

namespace vidf
{


	RenderContext::RenderContext(RenderDevicePtr _device)
		: device(_device)
	{
	}



	RenderContext::~RenderContext()
	{
	}



	bool RenderContext::Begin()
	{
		VkCommandBufferBeginInfo cmdBufferBeginInfo;
		ZeroStruct(cmdBufferBeginInfo);
		cmdBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		VK_VERIFY_RETURN(vkBeginCommandBuffer(drawCmdBuffer, &cmdBufferBeginInfo));
	}



	bool RenderContext::End()
	{
		VK_VERIFY_RETURN(vkEndCommandBuffer(drawCmdBuffer));
	}




	bool RenderContext::Initialize()
	{
		VkCommandBufferAllocateInfo commandBufferAllocationInfo;
		ZeroStruct(commandBufferAllocationInfo);
		commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocationInfo.commandPool = device->GetCommandPool();
		commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocationInfo.commandBufferCount = 1;
		VK_VERIFY_RETURN(
			vkAllocateCommandBuffers(
				device->GetDevice(), &commandBufferAllocationInfo,
				&drawCmdBuffer));

		return true;
	}



}
