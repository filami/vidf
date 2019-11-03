
cbuffer viewCB : register(b0)
{
	struct
	{
		float4x4 projTM;
		float4x4 viewTM;
		float2 viewportSize;
		float2 invViewportSize;
		float3 viewPosition;
		uint   numLights;
	} view;
};

Texture2D    diffuseSRV : register(t0);
SamplerState diffuseSS  : register(s0);



float3 LinearToSRGB(float3 color)
{
	return color < 0.0031308 ? 12.92 * color : 1.055 * pow(abs(color), 1.0 / 2.4) - 0.055;
}



struct VSInput
{
	float3 position : POSITION;
	float3 normal   : NORMAL;
	float2 texCoord : TEXCOORD;
};



struct VSOutput
{
	float4 hPosition : SV_POSITION;
	float2 texCoord  : TEXCOORD;
};



VSOutput VSMain(VSInput input)
{
	VSOutput output;

	output.hPosition = mul(view.projTM, mul(view.viewTM, float4(input.position, 1.0)));
	output.texCoord = input.texCoord;

	return output;
}



float4 PSMain(VSOutput input) : SV_TARGET
{
	const float3 diffuse = diffuseSRV.Sample(diffuseSS, input.texCoord).rgb;
	return float4(LinearToSRGB(diffuse), 1.0);
}
