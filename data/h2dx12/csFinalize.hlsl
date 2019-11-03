
Texture2D<float4> gBuffer0 : register(t0);
Texture2D<float4> gBuffer1 : register(t1);

RWTexture2D<float4> outputUAV : register(u0);



float LinearToSRGB(float x)
{
    if (x <= 0.00031308)
        return 12.92 * x;
    else
        return 1.055 * pow(x,(1.0 / 2.4)) - 0.055;
}



float3 LinearToSRGB(float3 c)
{
    return float3(
		LinearToSRGB(c.x),
		LinearToSRGB(c.y),
		LinearToSRGB(c.z));
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



[numthreads(64, 1, 1)]
void csMain(uint3 threadId : SV_DispatchThreadID)
{
	const uint2  pixel = threadId.xy;

	const float3 color = gBuffer1[pixel].rgb;
	const float lum = dot(color, rcp(3));
	const float var = gBuffer1[pixel].a;
	float3 output = gBuffer1[pixel].rgb;
	
	output *= gBuffer0[pixel].xyz;
	output = 1 - exp(-output);
	
	// Heretic 2 was too saturated at time. Reduce saturation a bit and boost vibrance
	const float saturation = 0.65;
	const float vibrance = 1.5;
	float3 hsv = saturate(rgb2hsv(output));
	hsv.y = pow(hsv.y * saturation, 1.0 / vibrance);
	output = hsv2rgb(hsv);

	outputUAV[threadId.xy] = float4(LinearToSRGB(output), 1);
}
