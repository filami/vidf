Texture2D srv : register(t0);
SamplerState ss : register(s0);

cbuffer cBuffer : register(b0)
{
	float4x4 projViewTM;
	float4x4 worldTM;
};



struct Input
{
	float3 position : POSITION;
	float2 tc : TEXCOORD;
	float4 color : COLOR;
};



struct Output
{
	float4 hPosition : SV_Position;
	float2 tc : TEXCOORD;
	float4 color : COLOR;
};



Output vsMain(Input input)
{
	Output output;
	output.hPosition = mul(projViewTM, mul(worldTM, float4(input.position, 1.0)));
	output.tc = input.tc;
	output.color = input.color;
	return output;
}



float4 psMain(Output input) : SV_Target
{
	return srv.Sample(ss, input.tc) * input.color;
}
