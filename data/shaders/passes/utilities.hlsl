
Texture2D srv0 : register(t0);
SamplerState sampler0 : register(s0);


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
	output.tc0 = vertices[vertexId] * float2(0.5, -0.5) + 0.5;
	return output;
}


float4 psReduceHalfSize(float4 pixelPos : SV_Position) : SV_Target
{
	const uint numSamples = 2;
	float4 output = 0.0;
	
	[unroll]
	for (uint y = 0; y < numSamples; ++y)
	{
		[unroll]
		for (uint x = 0; x < numSamples; ++x)
		{
			output += srv0[pixelPos.xy * numSamples + uint2(x, y)];
		}
	}
	
	return output / (numSamples * numSamples);
}



float4 psReduceQuarterSize(float4 pixelPos : SV_Position) : SV_Target
{
	const uint numSamples = 4;
	float4 output = 0.0;
	
	[unroll]
	for (uint y = 0; y < numSamples; ++y)
	{
		[unroll]
		for (uint x = 0; x < numSamples; ++x)
		{
			output += srv0[pixelPos.xy * numSamples + uint2(x, y)];
		}
	}
	
	return output / (numSamples * numSamples);
}



cbuffer gaussianBlurPassCB : register(b0)
{
	struct
	{
		float2 invSize;
		float2 direction;
	} gaussianBlurPass;
}


float4 psGaussianBlur7(VSOutput input) : SV_Target
{
	float4 output = 0.0;
	
	[unroll]
	for (int i = -3; i <= 3; ++i)
	{
		const float2 tc = input.tc0 + float2(i, i) * gaussianBlurPass.invSize * gaussianBlurPass.direction;
		output.rgb += srv0.Sample(sampler0, tc).rgb;
	}
	
	return output / 7.0;
}


float4 psGaussianBlur15(VSOutput input) : SV_Target
{
	float4 output = 0.0;
	
	[unroll]
	for (int i = -7; i <= 7; ++i)
	{
		const float2 tc = input.tc0 + float2(i, i) * gaussianBlurPass.invSize * gaussianBlurPass.direction;
		output.rgb += srv0.Sample(sampler0, tc).rgb;
	}
	
	return output / 15.0;
}
