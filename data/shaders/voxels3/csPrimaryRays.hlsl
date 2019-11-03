
#include "raytracecommon.hlsli"
#include "raytracevoxel.hlsli"

cbuffer viewCB : register(b0)
{
	View view;
};
Texture3D<uint> sparceBricksSRV : register(t0);
Texture3D<uint> voxelBricksSRV  : register(t1);

RWTexture2D<float>  hitTUAV    : register(u0);
RWTexture2D<float4> normalsUAV : register(u1);
RWTexture2D<float4> rayDirUAV  : register(u2);



uint WangHash(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}

float RandUNorm(inout uint rngState)
{
	rngState = WangHash(rngState);
	return float(rngState) * (1.0 / 4294967296.0);
}

float2 RandUFloat2(inout uint rngState)
{
	return float2(RandUNorm(rngState), RandUNorm(rngState));
}



[numthreads(8, 8, 1)]
void csMain(uint3 groundId : SV_GroupID, uint3 threadId : SV_GroupThreadID)
{
	const uint2 pixel = groundId.xy * uint2(8, 8) + threadId.xy;
	
	Ray primary = PrimaryRay(view.invProjViewTM, view.viewPosition, view.viewportSize, pixel);
	
	Fragment fragment;
	fragment.normal = 0;
	fragment.steps = 0;
	fragment.distance = 1024 * 1024;
	fragment.isect = false;
	RaytraceRootV2(fragment, sparceBricksSRV, voxelBricksSRV, primary);

	hitTUAV[pixel] = fragment.distance;
	normalsUAV[pixel] = float4(fragment.normal, 0);
	rayDirUAV[pixel] = float4(primary.dir, fragment.index);
}
