
Texture2D<float4> normalsSRV : register(t0);

RWTexture2D<float4> pixelUAV : register(u0);


float3 LinearToSRGBFast(float3 rgb)
{
	float3 s1 = sqrt(rgb);
	float3 s2 = sqrt(s1);
	float3 s3 = sqrt(s2);
	return 0.662002687 * s1 + 0.684122060 * s2 - 0.323583601 * s3 - 0.0225411470 * rgb;
}



float3 rgb2hsv(float3 c)
{
	float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
	float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}



float3 hsv2rgb(float3 c)
{
	float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}



[numthreads(128, 1, 1)]
void csMain(uint3 groundId : SV_GroupID, uint3 threadId : SV_GroupThreadID)
{
	const uint2 pixel = groundId.xy * uint2(128, 1) + threadId.xy;
	float3 color = normalsSRV[pixel].rgb;
	
	const float brightness = 1.5;
	const float saturation = 0.95;
	const float vibrance = 1.6;
	
	color = 1 - exp(-color * brightness);
	float3 hsv = saturate(rgb2hsv(color));
	hsv.y = pow(hsv.y * saturation, 1.0 / vibrance);
	color = hsv2rgb(hsv);
	
	pixelUAV[pixel] = float4(LinearToSRGBFast(color), 0);
}
