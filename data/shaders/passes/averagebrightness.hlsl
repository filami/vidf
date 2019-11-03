Texture2D srv0 : register(t0);
SamplerState sampler0 : register(s0);


cbuffer cb : register(b0)
{
	float2 invSize;
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
			const float2 tc = input.tc0 + float2(x, y) * invSize;
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
			const float2 tc = input.tc0 + float2(x, y) * invSize;
			const float value = srv0.Sample(sampler0, tc).r;
			output = max(output, value);
		}
	}
	return output;
}
