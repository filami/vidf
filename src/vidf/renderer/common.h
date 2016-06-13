#pragma once



#define VK_VERIFY_RETURN(vkInst)        \
{                                       \
	VkResult result = vkInst;           \
	if (result != VK_SUCCESS)           \
		return false;                   \
}



#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)                        \
{                                                                       \
    vk##entrypoint = (PFN_vk##entrypoint) vkGetInstanceProcAddr(inst, "vk"#entrypoint); \
    if (vk##entrypoint == NULL)                                         \
	{																    \
         __debugbreak();                                                \
    }                                                                   \
}



#define GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
    vk##entrypoint = (PFN_vk##entrypoint) vkGetDeviceProcAddr(dev, "vk"#entrypoint);   \
    if (vk##entrypoint == NULL)                                         \
	{																    \
        __debugbreak();                                                 \
    }                                                                   \
}


namespace vidf
{

	struct SwapChainDesc;
	class RenderDevice;
	class SwapChain;
	class RenderContext;
	typedef std::shared_ptr<RenderDevice> RenderDevicePtr;
	typedef std::shared_ptr<SwapChain> SwapChainPtr;
	typedef std::shared_ptr<RenderContext> RenderContextPtr;

}
