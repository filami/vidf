

cbuffer viewCB
{
	float3x2 viewTM;
};


struct VSOutput
{
	float4 hPosition : SV_Position;
	float2 tc0 : TEXCOORD0;
};



VSOutput vsMain(uint vertexId : SV_VertexId)
{
	float2 vertices[] =
	{
		float2(-1, -1),
		float2(-1,  3),
		float2(3, -1),
	};
	VSOutput output;
	output.hPosition = float4(vertices[vertexId], 0.5, 1.0);
	output.tc0 = vertices[vertexId] * float2(0.5, 0.5) + 0.5;
	return output;
}



float2 cMul(float2 a, float2 b)
{
	return float2(
		a.x * b.x - a.y * b.y,
		a.x * b.y + a.y * b.x);
}


float cNorm(float2 a)
{
	return sqrt(a.x * a.x + a.y * a.y);
}



float4 psMain(VSOutput input) : SV_Target
{
	float2 px = (input.tc0 * 2 - 1) / 5.0 + float2(-1.5, 0.0);
//	float2 px = mul(viewTM, input.tc0 * 2 - 1).xy;
		
	int i = 0;
	float2 z = 0;
	for (; cNorm(z) < 16.0 && i < 100; ++i)
		z = cMul(z, z) + px;
		
	float mu = i + 1 - log(log(cNorm(z))) / log(2.0);

	return float4(mu.xxx / 100.0, 1);
}
