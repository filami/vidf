#include "raytracecommon.hlsli"



cbuffer viewCB : register(b0)
{
	View view;
};
Texture2D<float>  hitTSRV         : register(t0);
Texture2D<float3> normalsSRV      : register(t1);
Texture2D<float4> rayDirSRV       : register(t2);
Texture2D<float>  aoHitTUAV       : register(t3);
Texture2D<float4> aoRayDirUAV     : register(t4);
Texture2D<float>  aoRayMaskUAV    : register(t5);
Texture2D<float3> aoSunDiffuseSRV : register(t6);

RWTexture2D<float4> lightUAV : register(u0);


static const bool temporalReuse = false;



[numthreads(8, 8, 1)]
void csMain(uint3 groundId : SV_GroupID, uint3 threadId : SV_GroupThreadID)
{
	const uint2 pixel = (groundId.xy * uint2(8, 8) + threadId.xy);
	
	const float3 normal = normalsSRV[pixel];
	const float3 position = view.viewPosition + rayDirSRV[pixel].xyz * hitTSRV[pixel];
	const float4 plane = NormalPointPlane(normal, position);
	
	float3 accum = 0;
	float count = 0;
	[unroll]
	for (int y = -5; y <= 5; ++y)
	{
		[unroll]
		for (int x = -5; x <= 5; ++x)
		{
			int2 coord = pixel + int2(x, y) * 2;
			int2 smpCoord = (coord >> 1) * 2;
		
			const float3 thatPosition = view.viewPosition + rayDirSRV[smpCoord].xyz * hitTSRV[smpCoord];
			float weigth = 1.0;
			weigth *= max(0, dot(normal, normalsSRV[smpCoord].xyz));
			weigth *= rcp(1 + abs(PlaneDist(plane, thatPosition)) * 8);
			weigth *= rayDirSRV[pixel].w == rayDirSRV[smpCoord].w;
			weigth *= rcp(1 + distance(thatPosition, position) * 2.0);
			
			accum.rgb += aoSunDiffuseSRV[coord >> 1] * weigth;
			count += weigth;
		}
	}
	float3 output = accum / max(1, count);
	
	output = aoSunDiffuseSRV[pixel / view.rayRate];

	lightUAV[pixel] = float4(output, 0);
}
