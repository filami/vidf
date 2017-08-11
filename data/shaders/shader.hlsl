
RasterizerOrderedTexture2D<float3> rovTestROV : register(u0);
Texture2D<float3> rovTestSRV : register(t0);

struct Input
{
	float2 position : POSITION;
	float4 color : COLOR;
};


struct Output
{
	float4 hPosition : SV_Position;
	float3 color : COLOR;
};


Output vsMain(Input input)
{
	Output output;
	output.hPosition = float4(input.position, 0.5, 1.0);
	output.color = input.color.rgb;
	return output;
}



void psMain(Output input, uint coverage : SV_Coverage)
{
	float aa = countbits(coverage) / 16.0;
	float3 outColor = rovTestROV[input.hPosition.xy];
	outColor = lerp(outColor, input.color, aa * 0.75);
	rovTestROV[input.hPosition.xy] = outColor;
}



float4 vsFinalMain(uint vertexId : SV_VertexId) : SV_Position
{
	float2 vertices[] =
	{
		float2(-1, -1),
		float2(-1,  3),
		float2( 3, -1),
	};
	return float4(vertices[vertexId], 0.5, 1.0);
}

float4 psFinalMain(float4 coord : SV_Position) : SV_Target
{
	return float4(rovTestSRV[coord.xy], 1.0);
}
