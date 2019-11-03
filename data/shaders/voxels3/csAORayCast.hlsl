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
Texture2D<float4> rayDirSRV       : register(t4);
Texture3D<float4> haltonSRV       : register(t5);

RWTexture2D<float>  aoHitTUAV       : register(u0);
RWTexture2D<float4> aoRayDirUAV     : register(u1);
RWTexture2D<float>  aoRayMaskUAV    : register(u2);
RWTexture2D<float4> aoSunDiffuseUAV : register(u3);


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
	
//	uint rngState = WangHash(uint(pixel.x)*1280 + uint(pixel.y) + uint(view.frameId)*1280*720);
	uint3 sampleCoord = uint3(pixel % 64, view.frameId % 64);

	NormalBasis normalBasis = NormalBasisFromNormal(normalsSRV[pixel * view.rayRate]);
		
	Ray ray;

	float3 incomingLight = 0;
	
	Fragment fragment;
	
	if (rayDirSRV[pixel * view.rayRate].w >= 1 && rayDirSRV[pixel * view.rayRate].w <= 4)
	{
		incomingLight += float3(1.0, 2.0, 2.5) * 4;
	}
	
	{
		float2 sample = haltonSRV[sampleCoord].xy;
		ray.dir = Transform(normalBasis, SampleCosineHemisphere(sample));
		ray.orig = view.viewPosition + rayDirSRV[pixel * view.rayRate].xyz * hitTSRV[pixel * view.rayRate];
		/*
		const float3 normal = Transform(normalBasis, SamplePowerHemisphere(RandUFloat2(rngState), 20.0));
		const float fresnel = max(0, Schlick(dot(normal, -rayDirSRV[pixel * 2].xyz), 1.0, 1.55));
		if (RandUNorm(rngState) < fresnel)
			ray.dir = reflect(rayDirSRV[pixel * 2].xyz, normal);
		else
			ray.dir = Transform(normalBasis, SampleCosineHemisphere(RandUFloat2(rngState)));
		ray.orig = view.viewPosition + rayDirSRV[pixel * 2].xyz * hitTSRV[pixel * 2];
		*/
		fragment.normal = 0;
		fragment.steps = 0;
		fragment.distance = 1024 * 1024;
		fragment.isect = false;
		RaytraceRootV2(fragment, sparceBricksSRV, voxelBricksSRV, ray);
		const float shadow = (1 - fragment.isect);
		const float3 light = max(dot(normalBasis.normal, ray.dir), 0) * float3(0.75, 0.9, 1.0) * 0.2;
		incomingLight += light * shadow;
		
		if (fragment.isect && fragment.index >= 1 && fragment.index <= 4)
		{
			const float3 light = max(dot(normalBasis.normal, ray.dir), 0) * max(dot(fragment.normal, -ray.dir), 0);
			incomingLight += light * float3(1.0, 2.0, 2.5) * 4;
		}
	}
	
	aoHitTUAV[pixel] = fragment.distance;
	aoRayDirUAV[pixel] = float4(ray.dir, 0);
	aoRayMaskUAV[pixel] = fragment.isect ? 1 : 0;
	
	{
		// second bounce sun light
		const float3 normal = fragment.normal;
		ray.orig = ray.orig + ray.dir * fragment.distance;
		ray.dir = lightDir;
	
		fragment.normal = 0;
		fragment.steps = 0;
		fragment.distance = 1024 * 1024;
		fragment.isect = false;
		RaytraceRootV2(fragment, sparceBricksSRV, voxelBricksSRV, ray);
		const float shadow = (1 - fragment.isect);
		const float3 light = max(dot(normalBasis.normal, -ray.dir), 0) * max(dot(normal, lightDir), 0) * float3(1.0, 0.95, 0.8) * 0.2;
		incomingLight += light * shadow;
	}
	
	/*
	{
		// second bounce
		ray.orig = ray.orig + ray.dir * fragment.distance;
		ray.dir = Transform(normalBasis, SampleCosineHemisphere(RandUFloat2(rngState)));
		fragment.color = 0;
		fragment.normal = 0;
		fragment.steps = 0;
		fragment.distance = 1024 * 1024;
		fragment.isect = false;
		RaytraceRootV2(fragment, sparceBricksSRV, voxelBricksSRV, ray);
		const float shadow = (1 - fragment.isect);
		incomingLight += float3(0.75, 0.9, 1.0) * shadow;
	}
	*/
	aoSunDiffuseUAV[pixel] = float4(incomingLight, 0);
}
