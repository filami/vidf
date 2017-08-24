


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



cbuffer cascadeShadowCB : register(b1)
{
	struct
	{
		float4x4 projTM;
		float4x4 invProjTM;
		float3 lightDir;
		float2 texelSize;
	} shadow;
};



float4 vsMain(float3 position : POSITION) : SV_Position
{
	return mul(shadow.projTM, float4(position, 1.0));
}
