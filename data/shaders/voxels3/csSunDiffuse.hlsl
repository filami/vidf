#include "raytracecommon.hlsli"
#include "raytracevoxel.hlsli"


cbuffer viewCB : register(b0)
{
	View view;
};
Texture3D<uint>   sparceBricksSRV : register(t0);
Texture3D<uint>   voxelBricksSRV  : register(t1);
Texture2D<float>  hitTSRV         : register(t2);
Texture2D<float3> normalsSRV      : register(t3);
Texture2D<float3> rayDirSRV       : register(t4);

RWTexture2D<float4> sunDiffuseUAV : register(u0);



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


static const float3 lightDir = normalize(float3(2, 4, 10));



[numthreads(8, 8, 1)]
void csMain(uint3 groundId : SV_GroupID, uint3 threadId : SV_GroupThreadID)
{
	const uint2 pixel = (groundId.xy * uint2(8, 8) + threadId.xy);
	
	uint rngState = WangHash(uint(pixel.x)*1280 + uint(pixel.y) + uint(view.frameId)*1280*720);
	NormalBasis normalBasis = NormalBasisFromNormal(normalsSRV[pixel * 2]);
	
	Ray ray;
	ray.orig = view.viewPosition + rayDirSRV[pixel * 2].xyz * hitTSRV[pixel * 2];
	ray.dir = SampleCone(lightDir, RandUFloat2(rngState), 0.025);
	
	Fragment fragment;
	fragment.normal = 0;
	fragment.steps = 0;
	fragment.distance = 1024 * 1024;
	fragment.isect = false;
	RaytraceRootV2(fragment, sparceBricksSRV, voxelBricksSRV, ray);
	
	const float shadow = (1 - fragment.isect);
	const float3 light = max(dot(normalBasis.normal, lightDir), 0) * float3(1.0, 0.95, 0.8) * 0.2;
		
	sunDiffuseUAV[pixel] = float4(light * shadow, 0);
}
