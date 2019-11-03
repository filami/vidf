#include "variables.hlsl"

struct Sample
{
	float2 sPoint;
	uint rngSeed;
};

Texture2D paletteSRV : register(t0);
StructuredBuffer<Sample> samplesSRV : register(t1);
SamplerState paletteSS : register(s0);
RWTexture2D<float4> histogramUAV : register(u0);


cbuffer viewCB : register(b0)
{
	struct
	{
		float4x4 projViewTM;
		float3   cameraPos;
		float    time;
		uint2    viewport;
		int      rngOffset;
		float    aspectRatio;
	} view;
}



#include "flame01.hlsl"
// #include "flame02.hlsl"
// #include "flame03.hlsl"
// #include "flame04.hlsl"



void AddPoint(const float3 p, const float3 color)
{
	const float4 hPosition = mul(view.projViewTM, float4(p, 1));
	const float2 screenCoord = hPosition.xy / hPosition.w * float2(1, -1);

	const float dist = distance(p, view.cameraPos);
	const float coc = (-1.0/dist + 1.0/focalDist) * cocSize;
	
	const float u = RandUNorm();
	const float v = RandUNorm() * 2 * pi;
	const float2 disc = sqrt(u) * float2(cos(v), sin(v));
	const float2 finalP = screenCoord + disc * coc * float2(1, view.aspectRatio);

	const uint2 c = (finalP * 0.5 + 0.5) * view.viewport;
	if (hPosition.z > 0.0 && c.x < view.viewport.x && c.y < view.viewport.y)
		histogramUAV[c] += float4(color, 1);
}




[numthreads(64, 1, 1)]
void csFlame(uint3 threadId : SV_DispatchThreadID)
{
	float3 p = float3(samplesSRV[threadId.x].sPoint.xy, 0.0);
	rngState = samplesSRV[threadId.x].rngSeed;
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
