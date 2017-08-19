
struct OIT
{
	half4 fragments[8];
	float depth[8];
	uint  numFrags;
};

RasterizerOrderedStructuredBuffer<OIT> rovTestROV : register(u0);

Texture2D solidSRV : register(t0);
StructuredBuffer<OIT> rovTestSRV : register(t1);

Texture2D diffuseSRV : register(t0);
SamplerState diffuseSS : register(s0);


cbuffer viewCB : register(b0)
{
	struct
	{
		float4x4 projTM;
		float4x4 viewTM;
		float2 viewportSize;
		float2 invViewportSize;
		float3 viewPosition;
		float _;
	} view;
};


struct Input
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD0;
};


struct Output
{
	float4 hPosition : SV_Position;
	float3 wPosition : TEXCOORD0;
	float3 wNormal : TEXCOORD1;
	float2 texCoord : TEXCOORD2;
};


Output vsMain(Input input)
{
	Output output;
	output.hPosition = mul(view.projTM, mul(view.viewTM, float4(input.position, 1.0)));
	output.wPosition = input.position;
	output.wNormal = input.normal.xyz;
	output.texCoord = input.texCoord;
	return output;
}



void psOITClear(float4 coord : SV_Position)
{
	const uint idx = coord.x + view.viewportSize.x * coord.y;
	for (int i = 0; i < 8; i++)
	{
		rovTestROV[idx].fragments[i] = half4(0, 0, 0, 1);
		rovTestROV[idx].depth[i] = 1.0;
	}
	rovTestROV[idx].numFrags = 0;
}



float4 psMain(Output input) : SV_Target
{
	const float3 diffuseColor = diffuseSRV.Sample(diffuseSS, input.texCoord).rgb;
	return float4(diffuseColor, 1.0);
}



[earlydepthstencil]
void psMainOIT(Output input)
{
	const float2 tc = frac(input.texCoord);
	const float3 wNormal = normalize(input.wNormal);
	const float3 diffuseColor = diffuseSRV.Sample(diffuseSS, input.texCoord).rgb;

	const float3 diffuse = diffuseColor;
	// const float3 emissive = (1 - pow(max(0, dot(normalize(view.viewPosition - input.wPosition), wNormal)), 1.0 / 8.0)) * float3(1, 0.75, 0.25) * 0.2;
	const float3 emissive = 0;
	const float filter = (1 - pow(max(0, dot(normalize(view.viewPosition - input.wPosition), wNormal)), 1.0 / 4.0));
	
	const uint pxIdx = input.hPosition.x + view.viewportSize.x * input.hPosition.y;
	rovTestROV[pxIdx].numFrags = min(8, rovTestROV[pxIdx].numFrags + 1);
	half4 fragment = half4(diffuse * filter + emissive, 1.0 - filter);
	float depth = input.hPosition.z;
	for (uint i = 0; i < rovTestROV[pxIdx].numFrags; i++)
	{
		if (input.hPosition.z < rovTestROV[pxIdx].depth[i])
		{
			half4 tempFragment = fragment;
			float tempDepth = depth;
			fragment = rovTestROV[pxIdx].fragments[i];
			depth = rovTestROV[pxIdx].depth[i];
			rovTestROV[pxIdx].fragments[i] = tempFragment;
			rovTestROV[pxIdx].depth[i] = tempDepth;
		}
	}
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

	// float3 output = 0.15;
	float3 output = solidSRV[coord.xy].rgb;

	for (int i = rovTestSRV[idx].numFrags - 1; i >= 0; --i)
		output = output * rovTestSRV[idx].fragments[i].a + rovTestSRV[idx].fragments[i].rgb;

	return float4(1 - exp(-output * 1.5), 1.0);
}
