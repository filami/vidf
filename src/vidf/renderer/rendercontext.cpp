#include "pch.h"
#include "common.h"
#include "rendercontext.h"

namespace vidf
{


	RenderContext::RenderContext(VkDevice _device, VkCommandPool _commandPool, VkQueue _queue, uint32_t _queueIndex)
		: device(_device)
		, commandPool(_commandPool)
		, queue(_queue)
		, queueIndex(_queueIndex)
	{
	}



	RenderContext::~RenderContext()
	{
	}



	void RenderContext::Begin()
	{
	}



	void RenderContext::End()
	{
	}




	bool RenderContext::Initialize()
	{
		VkCommandBufferAllocateInfo commandBufferAllocationInfo;
		ZeroStruct(commandBufferAllocationInfo);
		commandBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocationInfo.commandPool = commandPool;
		commandBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocationInfo.commandBufferCount = 1;
		VK_VERIFY_RETURN(vkAllocateCommandBuffers(device, &commandBufferAllocationInfo, &drawCmdBuffer));

		return true;
	}



}
