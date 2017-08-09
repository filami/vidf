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


/*
float4 psMain(Output input) : SV_Target
{
	float pixelIn = testROV[input.hPosition.xy];
	pixelIn += 0.2;
	return 1.0;
}
*/

void psMain(Output input)
{
	rovTestROV[input.hPosition.xy] = input.color;
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
