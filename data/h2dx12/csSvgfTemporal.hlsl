
Texture2D<float4> inputSRV    : register(t0);
Texture2D<float4> gBuffer0SRV : register(t1);
Texture2D<float4> gBuffer1SRV : register(t2);
Texture2D<float>  depthSRV    : register(t3);
uint clearTemporal : register(b0);

RWTexture2D<float4> outputUAV : register(u0);
RWTexture2D<float3> momentsUAV : register(u1);



[numthreads(8, 8, 1)]
void csMain(uint3 threadId : SV_DispatchThreadID)
{
	const int2 pixel = int2(threadId.xy);
	
	const float speed = 128;
	const float3 prevOutput = outputUAV[pixel].rgb;
	const float3 currOutput = inputSRV[pixel].rgb;
	const float histLenPrev = momentsUAV[pixel].z;
	const float histLen = clearTemporal ? 1.0 : min(histLenPrev + 1.0f, speed);
	const float alpha = max(rcp(speed), rcp(histLen));
	const float3 output = lerp(prevOutput, currOutput, alpha);
	
	const float2 momPrev = momentsUAV[pixel].xy;
	const float currLum = dot(currOutput, rcp(3));
	const float2 momCurr = float2(currLum, currLum * currLum);
	/* const */ float2 mom = lerp(momPrev, momCurr, alpha);
	/* const */ float var = sqrt(max(mom.y - mom.x * mom.x, 0));
		
	if (clearTemporal)
	{
		const float3 cNormal = gBuffer1SRV[pixel].xyz;
		float3 sumColor = currOutput;
		float maxLum = dot(currOutput, rcp(3));
		float2 sumMom = float2(maxLum, maxLum*maxLum);
		float sumW = 1.0;
		for (int y = -3; y <= 3; ++y)
		{
			for (int x = -3; x <= 3; ++x)
			{
				if (x == 0 && y == 0)
					continue;				
				const int2 coord = pixel + int2(x, y);
			//	if (coord.x < 0 || coord.y < 0 || coord.x >= width || coord.y >= height)
			//		continue;
					
				const float depth = depthSRV[coord];
				const float3 qColor = inputSRV[coord];
				const float3 qNormal = gBuffer1SRV[coord].xyz;
				const float  qLum = dot(qColor, rcp(3));
				
				const float w = pow(max(0, dot(cNormal, qNormal)), 128);
				sumColor += qColor * w;
				sumMom += float2(qLum * w, qLum * qLum * w * w);
				sumW += w;
			}
		}
		sumColor /= sumW;
		sumMom /= float2(sumW, sumW * sumW);
		mom = sumMom;
		var = maxLum * 8;
	}
        
	outputUAV[pixel] = float4(output, var);
	momentsUAV[pixel] = float3(mom, histLen);
}
