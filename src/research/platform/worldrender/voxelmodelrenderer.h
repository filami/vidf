#pragma once

#include "voxel.h"

namespace vi::retro::render
{

	using namespace std;
	using namespace vidf;


	class VoxelModelRenderer
	{
	private:

	public:
		struct Handle
		{
			uint id = -1;
			bool IsValid() const { return id != -1; }
		};

		struct ObjectCB
		{
			Matrix44f worldTM;
		};

		struct Model
		{
			shared_ptr<VoxelModel> model;
			ObjectCB  objectCBData;
			GPUBuffer objectCB;
			bool      valid = false;
			bool      updateCB = false;
			bool      flip = false;
		};

	public:
		VoxelModelRenderer(RenderDevicePtr _renderDevice, ShaderManager* shaderManager);

		Handle AddModel(shared_ptr<VoxelModel> model);
		void   RemoveModel(Handle handle);

		void SetModel(Handle handle, shared_ptr<VoxelModel> model);
		void SetModelWorldTM(Handle handle, Matrix44f objectTM);

		void Prepare(CommandBuffer* commandBuffer);
		void DrawPreZ(CommandBuffer* commandBuffer);
		void Draw(CommandBuffer* commandBuffer);

	private:
		RenderDevicePtr renderDevice;
		vector<Model>   models;
		deque<uint>     freeSlots;

		GraphicsPSOPtr voxelPreZPSO;
		GraphicsPSOPtr voxelPSO;
		GraphicsPSOPtr voxelOutlinePSO;
		vector<uint>   toUpdateCB;
		vector<uint>   toDraw;
	};



}
