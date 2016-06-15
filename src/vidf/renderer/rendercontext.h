#pragma once

namespace vidf
{


	class RenderContext
	{
	public:
		~RenderContext();

		bool Begin();
		bool End();

		VkCommandBuffer GetDrawCommandBuffer() { return drawCmdBuffer; }

	private:
		friend class RenderDevice;
		RenderContext(RenderDevicePtr _device);

		bool Initialize();

	private:
		RenderDevicePtr device;
		VkCommandBuffer drawCmdBuffer;
		bool open = false;
	};



}
