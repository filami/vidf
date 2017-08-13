
struct OIT
{
	half4 fragments[8];
	float depth[8];
	uint  numFrags;
};

RasterizerOrderedStructuredBuffer<OIT> rovTestROV : register(u0);
StructuredBuffer<OIT> rovTestSRV : register(t0);
// RWTexture2D<float3> rovTestROV : register(u0);
// Texture2D<float3> rovTestSRV : register(t0);


cbuffer viewCB : register(b0)
{
	struct
	{
		float4x4 projTM;
		float4x4 viewTM;
		float2 viewportSize;
		float2 invViewportSize;
	} view;
};


struct Input
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD;
};


struct Output
{
	float4 hPosition : SV_Position;
	float3 color : COLOR;
};


Output vsMain(Input input)
{
	Output output;
	output.hPosition = mul(view.projTM, mul(view.viewTM, float4(input.position, 1.0)));
	output.color = input.normal.xyz * 0.5 + 0.5;
	return output;
}



void psOITClear(float4 coord : SV_Position)
{
	const uint idx = coord.x + view.viewportSize.x * coord.y;
	for (int i = 0; i < 8; i++)
		rovTestROV[idx].depth[i] = 1.0;
	rovTestROV[idx].numFrags = 0;
}



void psMain(Output input /*, uint coverage : SV_Coverage*/)
{
	const float aa = /* countbits(coverage) / 16.0 */ 1.0;
	const float4 output = float4(input.color * 0.75 * aa, 1.0 - 0.75 * aa);

	const uint pxIdx = input.hPosition.x + view.viewportSize.x * input.hPosition.y;

	half4 fragment = half4(output);
	half depth = input.hPosition.z;
	for (uint i = 0; i < rovTestROV[pxIdx].numFrags + 1; i++)
	{
		if (input.hPosition.z < rovTestROV[pxIdx].depth[i])
		{
			half4 tempFragment = fragment;
			half tempDepth = depth;
			fragment = rovTestROV[pxIdx].fragments[i];
			depth = rovTestROV[pxIdx].depth[i];
			rovTestROV[pxIdx].fragments[i] = tempFragment;
			rovTestROV[pxIdx].depth[i] = tempDepth;
		}
	}

	rovTestROV[pxIdx].numFrags += 1;
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
	const uint idx = coord.x + view.viewportSize.x * coord.y;

	float3 output = 0.15;

	for (int i = rovTestSRV[idx].numFrags - 1; i >= 0; --i)
		output = output * rovTestSRV[idx].fragments[i].a + rovTestSRV[idx].fragments[i].rgb;

	return float4(output, 1.0);
}
