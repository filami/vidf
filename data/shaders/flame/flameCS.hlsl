#include "variables.hlsl"
#include "flame01.hlsl"

Texture2D paletteSRV : register(t0);
SamplerState paletteSS : register(s0);
RWTexture2D<float4> histogramUAV : register(u0);


cbuffer viewCB : register(b0)
{
	struct
	{
		float4x4 projViewTM;
		float3   cameraPos;
		uint2    viewport;
		int      rngOffset;
	} view;
}



void AddPoint(const float3 p, const float3 color)
{
	const float4 hPosition = mul(view.projViewTM, float4(p, 1));
	const float2 screenCoord = hPosition.xy / hPosition.w;

	const float focalDist = 1.4;
	const float cocSize = 0.035;
	const float dist = distance(p, view.cameraPos);
	const float coc = (-1.0/dist + 1.0/focalDist) * cocSize;
	
	const float u = RandUNorm();
	const float v = RandUNorm() * 2 * pi;
	const float2 disc = sqrt(u) * float2(cos(v), sin(v));
	const float2 finalP = screenCoord + disc * coc;

	const uint2 c = (finalP * 0.5 + 0.5) * view.viewport;
	if (hPosition.z > 0.0 && c.x < view.viewport.x && c.y < view.viewport.y)
		histogramUAV[c] += float4(color, 1);
}





[numthreads(64, 1, 1)]
void csFlame(uint3 threadId : SV_DispatchThreadID)
{
	rngState = threadId.x + view.rngOffset;
	
	float3 p = float3(RandSNorm(), RandSNorm(), 0.0);
	const float3 ip = p;
	float c = RandUNorm();

	const uint fuse = 15;
	const uint numIterations = 60;

	uint i;
	[loop]
	for (i = 0; i < fuse; ++i)
	{
		float ci;
		Iterate(p, ci);
	}

	[loop]
	for (i = 0; i < numIterations - fuse; ++i)
	{
		float ci;		
		Iterate(p, ci);
		c = (c + ci) * 0.5;
		AddPoint(p, paletteSRV.SampleLevel(paletteSS, float2(c, 0.5), 0).rgb);
	}
}
