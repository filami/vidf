#pragma once

namespace vidf
{


	class RenderContext
	{
	public:
		~RenderContext();

		void Begin();
		void End();

	private:
		RenderContext(VkDevice _device, VkCommandPool _commandPool, VkQueue _queue, uint32_t _queueIndex);

		bool Initialize();

	public:
	// private:
		friend class RenderDevice;
		VkDevice device;
		VkCommandPool commandPool;
		VkCommandBuffer drawCmdBuffer;
		VkQueue queue;
		uint32_t queueIndex;
		bool open = false;
	};



}
