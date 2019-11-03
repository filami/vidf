
cbuffer viewCB : register(b0)
{
	struct
	{
		float4x4 projViewTM;
		float4x4 invProjViewTM;
		float2 viewportSize;
		float2 invViewportSize;
		float3 viewPosition;
		uint   frameIdx;
	} view;
};


struct GBufferOut
{
	float4 gBuffer0;
	float4 gBuffer1;
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
	float2 tc       : TEXCOORD;
};



struct VSOutput
{
	float4 hPosition : SV_POSITION;
	float2 tc        : TEXCOORD0;
	float3 normal    : NORMAL;
};



VSOutput VSMain(VSInput input)
{
	VSOutput output;

	output.hPosition = mul(view.projViewTM, float4(input.position, 1.0));
	output.tc = input.tc;
	output.normal = input.normal;

	return output;
}



GBufferOut PSMain(VSOutput input) : SV_TARGET
{
	const float3 diffuse = diffuseSRV.Sample(diffuseSS, input.tc).rgb;
	const float3 normal = normalize(input.normal);
	
	GBufferOut output;
	output.gBuffer0 = float4(diffuse, 0);
	output.gBuffer1 = float4(normal, 0);
	return output;
}
