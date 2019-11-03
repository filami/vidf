

Texture2D<float4> inputSRV    : register(t0);
Texture2D<float4> gBuffer0SRV : register(t1);
Texture2D<float4> gBuffer1SRV : register(t2);
Texture2D<float>  depthSRV    : register(t3);

cbuffer cb : register(b0)
{
	uint filterStep;
	float2 depthFactors;
	float _;
	int width;
	int height;
	uint2 __;
	float3 gaussian;
	float ___;
};

RWTexture2D<float4> outputUAV : register(u0);



float LinearDepth(float hDepth, float2 depthFactors)
{
//	return rcp(depthFactors.x * hDepth + depthFactors.y) * 10000.0;
	return hDepth;
}



[numthreads(8, 8, 1)]
void csMain(uint3 threadId : SV_DispatchThreadID)
{
	const int2 pixel = int2(threadId.xy);
	
	const float  cw = gaussian[1] * gaussian[1];
	const float3 cColor = inputSRV[pixel].rgb;
	const float3 cNormal = gBuffer1SRV[pixel].xyz;
	const float  cVar = gBuffer1SRV[pixel].w;
	const float  cDepth = LinearDepth(depthSRV[pixel], depthFactors);
	const float  cLum = dot(cColor, rcp(3));
	float3 colorSum = cColor * cw;
	float wSum = cw;
	float varSum = cVar * cw * cw;
		
	float sigmalL = 0;
	float sum = 0;
	int y;
	for (y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{			
			const int2 coord = pixel + int2(x, y);
			if (coord.x < 0 || coord.y < 0 || coord.x >= width || coord.y >= height)
				continue;
			const float wg = gaussian[x+1] * gaussian[y+1];
			sigmalL += inputSRV[coord].w * wg;
			sum += wg;
		}
	}
	sigmalL /= sum;
	
	for (y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			if (x == 0 && y == 0)
				continue;
			
			const int2 coord = pixel + int2(x, y) * filterStep;
			if (coord.x < 0 || coord.y < 0 || coord.x >= width || coord.y >= height)
				continue;
			
			const float3 kColor = inputSRV[coord].rgb;
			const float  kVar = inputSRV[coord].a;
			const float3 kNormal = gBuffer1SRV[coord].xyz;
			const float  kDepth = LinearDepth(depthSRV[coord], depthFactors);
			const float  kLum = dot(kColor, rcp(3));
			
			const float diffZ = abs(kDepth - cDepth) * 256.0;
			const float diffL = abs(kLum - cLum);
			
			const float wg = gaussian[x+1] * gaussian[y+1];
			const float wn = pow(max(0, dot(cNormal, kNormal)), 128);
			/*
			const float wz = exp(-diffZ / float(filterStep));
			const float wl = min(diffL * diffL * sigmalL * 512.0, 1.0);
			const float w = wg * wn * wz * wl;
			*/
			const float wz = -diffZ / float(filterStep);
			// const float wl = -min(diffL * diffL * sigmalL * 512.0, 1.0);
			// const float wl = -min(diffL * diffL * sigmalL * 2.0, 1.0);
			const float wl = -filterStep;
			// const float w = wg * wn * exp(wz + wl);
			const float w = wg * wn /* exp(wz) */;
			
			colorSum += kColor * w;
			varSum += kVar * w * w;
			wSum += w;
		}
	}
	
	colorSum /= wSum;
	varSum /= wSum * wSum;
	
	outputUAV[pixel] = float4(colorSum, varSum);
//	outputUAV[pixel] = inputSRV[pixel];
}
