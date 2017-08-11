#pragma once



#define VK_VERIFY_RETURN(vkInst)        \
{                                       \
	VkResult result = vkInst;           \
	if (result != VK_SUCCESS)           \
		return false;                   \
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
