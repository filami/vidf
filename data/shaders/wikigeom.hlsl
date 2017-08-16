
cbuffer cBuffer : register(b0)
{
	float4x4 projViewTM;
	float4x4 worldTM;
};



struct Input
{
	float3 position : POSITION;
	float4 color : COLOR;
};



struct Output
{
	float4 hPosition : SV_Position;
	float4 color : COLOR;
};



Output vsMain(Input input)
{
	Output output;
	output.hPosition = mul(projViewTM, mul(worldTM, float4(input.position, 1.0)));
	output.color = input.color;
	return output;
}



float4 psMain(Output input) : SV_Target
{
	return input.color;
}
