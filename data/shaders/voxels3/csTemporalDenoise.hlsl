#include "raytracecommon.hlsli"

cbuffer viewCB : register(b0)
{
	View view;
};
Texture2D<float>  hitTSRV       : register(t0);
Texture2D<float3> normalsSRV    : register(t1);
Texture2D<float3> rayDirSRV     : register(t2);
Texture2D<float3> lightSRV      : register(t3);
Texture2D<float3> outputPrevSRV : register(t4);
Texture2D<float3> rayDirPrevSRV : register(t5);
Texture2D<float>  hitTPrevSRV   : register(t6);

RWTexture2D<float4> outputUAV : register(u0);

[numthreads(8, 8, 1)]
void csMain(uint3 groundId : SV_GroupID, uint3 threadId : SV_GroupThreadID)
{
	const int2 pixel = (groundId.xy * uint2(8, 8) + threadId.xy);

	const float3 position = view.viewPosition + rayDirSRV[pixel].xyz * hitTSRV[pixel];
	float4 hPrevPosition = mul(view.prevProjViewTM, float4(position, 1));
	hPrevPosition /= hPrevPosition.w;
	const float2 prevPx = uint2((hPrevPosition.xy * float2(1, -1) * 0.5 + 0.5) * view.viewportSize + 0.5);

	const float3 light = lightSRV[pixel];
	const float3 prevSample = outputPrevSRV[prevPx];
	
	bool reuse = true;
	if (prevPx.x <= 0 || prevPx.y <= 0)
		reuse = false;
	if (prevPx.x >= view.viewportSize.x || prevPx.y >= view.viewportSize.y)
		reuse = false;

	const float3 prevPosition = view.prevViewPosition + rayDirPrevSRV[prevPx].xyz * hitTPrevSRV[prevPx];
	if (abs(distance(prevPosition, position)) > 1)
		reuse = false;
		
	if (reuse)
	{
		const float count = outputUAV[pixel].a;
	//	outputUAV[pixel] = float4(lerp(prevSample, light, 1 / (count + 1)), count + 1.0);
		outputUAV[pixel] = float4(lerp(prevSample, light, 1 / 64.0), 1.0);
	}
	else
	{
		outputUAV[pixel] = float4(light, 1.0);
	}
}
