#include "flameCS.hlsl"

Texture2D srv0 : register(t0);
Texture2D srv1 : register(t1);
SamplerState sampler0 : register(s0);



cbuffer cabCB : register(b0)
{
	struct
	{
		float2 invSize;
	} cab;
}



struct VSOutput
{
	float4 hPosition : SV_Position;
	float2 tc0 : TEXCOORD0;
};


VSOutput vsFullscreen(uint vertexId : SV_VertexId)
{
	float2 vertices[] =
	{
		float2(-1, -1),
		float2(-1,  3),
		float2(3, -1),
	};
	VSOutput output;
	output.hPosition = float4(vertices[vertexId], 0.5, 1.0);
	output.tc0 = vertices[vertexId] * 0.5 + 0.5;
	return output;
}



float4 psAverageColor(VSOutput input) : SV_Target
{
	const uint numSamples = 4;
	float output = 0.0;
	[unroll]
	for (uint y = 0; y < numSamples; ++y)
	{
		[unroll]
		for (uint x = 0; x < numSamples; ++x)
		{
			const float2 tc = input.tc0 + float2(x, y) * cab.invSize;
			float value = dot(srv0.Sample(sampler0, tc).rgb, 1.0 / 3.0);
			output = max(output, value);
		}
	}
	return output;
}



float4 psAverage(VSOutput input) : SV_Target
{
	const uint numSamples = 4;
	float output = 0.0;
	[unroll]
	for (uint y = 0; y < numSamples; ++y)
	{
		[unroll]
		for (uint x = 0; x < numSamples; ++x)
		{
			const float2 tc = input.tc0 + float2(x, y) * cab.invSize;
			const float value = srv0.Sample(sampler0, tc).r;
			output = max(output, value);
		}
	}
	return output;
}



float3 rgb2hsv(float3 c)
{
	float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
	float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}



float3 hsv2rgb(float3 c)
{
	float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}




float4 psFinalize(VSOutput input) : SV_Target
{
	const float4 histogram = srv0[uint2(input.hPosition.xy)];
	const float maxValue = srv1[uint2(0, 0)].r;
	
	float3 color = histogram.rgb / maxValue;
	
	color = saturate(1-exp(-color * brightness));

	float3 hsv = saturate(rgb2hsv(color));
	hsv.y = pow(hsv.y * saturation, 1.0 / vibrance);
	hsv.z = pow(hsv.z, 1.0 / gamma);
	color = hsv2rgb(hsv);
	
	return float4(color, 1.0);
}



cbuffer dispCB : register(b0)
{
	struct
	{
		float4 disp;
	} disp;
}



float4 psDisplay(VSOutput input) : SV_Target
{
	const float3 final = srv0[uint2(disp.disp.xy + input.hPosition.xy * disp.disp.zw)].rgb;
	return float4(final, 1.0);
}
