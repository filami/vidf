#include "raytracecommon.hlsli"

cbuffer viewCB : register(b0)
{
	View view;
};
Texture2D<float4> sunDiffuseSRV : register(t0);
Texture2D<float>  hitTSRV       : register(t1);
Texture2D<float3> normalsSRV    : register(t2);
Texture2D<float4> rayDirSRV     : register(t3);

RWTexture2D<float4> lightUAV : register(u0);

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
	for (int y = -1; y <= 1; ++y)
	{
		[unroll]
		for (int x = -1; x <= 1; ++x)
		{
			int2 coord = pixel + int2(x, y) * 2;
			int2 smpCoord = (coord >> 1) * 2;
		
			const float3 thatPosition = view.viewPosition + rayDirSRV[smpCoord].xyz * hitTSRV[smpCoord];
			float weigth = 1.0;
			weigth *= max(0, dot(normal, normalsSRV[smpCoord].xyz));
			weigth *= rcp(1 + abs(PlaneDist(plane, thatPosition)) * 8);
			weigth *= rayDirSRV[pixel].w == rayDirSRV[smpCoord].w;
			
			accum.rgb += sunDiffuseSRV[coord >> 1].rgb * weigth;
			count += weigth;
		}
	}
	float3 output = accum / max(1, count);

	lightUAV[pixel] += float4(output, 0);
}
