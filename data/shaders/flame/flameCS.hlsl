Texture2D paletteSRV : register(t0);
SamplerState paletteSS : register(s0);
RWTexture2D<float4> histogramUAV : register(u0);


cbuffer viewCB : register(b0)
{
	struct
	{
		uint2    viewport;
		int      rngOffset;
		row_major float2x3 cameraTM;
	} view;
}



static uint rngState;


uint WangHash(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}


float RandUNorm()
{
	rngState = WangHash(rngState);
	return float(rngState) * (1.0 / 4294967296.0);
}



float RandSNorm()
{
	rngState = WangHash(rngState);
	return float(rngState) * (1.0 / 4294967296.0) * 2 - 1;
}



void AddPoint(const float2 p, const float3 color)
{
	const uint2 c = mul(view.cameraTM, float3(p, 1)) * view.viewport;
	if (c.x < view.viewport.x && c.y < view.viewport.y)
		histogramUAV[c] += float4(color, 1);
}


float2 Horseshoe(const float2 p, const float r)
{
	return float2((1 / r) * (p.y - p.x) * (p.x + p.y), 2 * p.x * p.y);
}


void Function3(inout float2 p)
{
	const float r = length(p);
	p =
		0.5 * p +
		0.2 * Horseshoe(p, r);
}


[numthreads(64, 1, 1)]
void csFlame(uint3 threadId : SV_DispatchThreadID)
{
	rngState = threadId.x + view.rngOffset;
	
	float2 p = float2(RandSNorm(), RandSNorm());
	const float2 ip = p;
	float c = RandUNorm();

	float3x2 tm0;
	tm0[0] = float2( 0.5,  0.0);
	tm0[1] = float2( 0.0,  0.5);
	tm0[2] = float2(-0.5, -0.5);

	float3x2 tm1;
	tm1[0] = float2( 0.5, 0.0);
	tm1[1] = float2( 0.0, 0.5);
	tm1[2] = float2( 0.5, -0.5);

	float3x2 tm2;
	tm2[0] = float2( 0.5,  0.0);
	tm2[1] = float2( 0.0,  0.5);
	tm2[2] = float2(-0.5,  0.5);

	float3x2 tm3;
	tm3[0] = float2(1.0, 0.0);
	tm3[1] = float2(0.0, 1.0);
	tm3[2] = float2(0.0, 0.0);

	const uint numIterations = 60;
	for (uint i = 0; i < numIterations; ++i)
	{
		const float pi = acos(-1.0);

		const float f = RandUNorm();
		float ci;

		/*
		if (f < 0.25)
		{
			p = mul(float3(p, 1), tm0);
		}
		else if (f < 0.5)
		{
			p = mul(float3(p, 1), tm1);
		}
		else if (f < 0.75)
		{
			p = mul(float3(p, 1), tm2);
		}
		else
		{
			float2 np = mul(float3(p, 1), tm3);
			Function3(np);
			p = np;
		}
		*/
		
		
		if (f < 0.1)
		{
			const float2 np = p;

			const float u = RandUNorm() * 2 * pi;
			p =
				np * 0.65 +
				RandUNorm() * float2(cos(u), sin(u)) * 0.01;
			ci = 0.05;
		}
		else if (f < 0.65)
		{
			// const float2 np = p * 0.75 + float2(-0.25, 0.25);
			const float2 np = p * 0.75 + float2(-0.25, 0.25);
						
			const float r = length(np);
			const float theta = atan2(np.x, np.y);
			const float phi = atan2(np.y, np.x);

			const float p1 = 4.0;
			const float p2 = -1;
			const uint p3 = abs(p1)*RandUNorm();
			const float t = (phi + 2 * pi * p3) / p1;
			p = pow(r, p2 / p1) * float2(cos(t), sin(t)) * 0.65;
			ci = 0.5;
		}
		else if (f < 0.85)
		{
			const float2 np = p*float2(-6, 0.5) + float2(-0.2, 0.5);

			const float pi = acos(-1.0);
			const float r = length(np);
			const float theta = atan2(np.x, np.y);
			const float phi = atan2(np.y, np.x);

			const float p1 = -8.0;
			const float p2 = 2.0;
			const uint p3 = abs(p1)*RandUNorm();
			const float t = (sign(RandSNorm()) * phi + 2 * pi * p3) / p1;
			p = pow(r, p2 / p1) * float2(cos(t), sin(t)) * 0.75;
			ci = 0.9;
		}
		else
		{
			const float2 np = p * 1.5;

			const float r = length(np);

			p = (2 / (1 + r)) * np.yx * 0.5;
			ci = 0.4;
		}

		c = (c + ci) * 0.5;

		/*
		// Depth of Field works like marvel!!
		const float u = RandUNorm();
		const float v = RandUNorm() * 2 * pi;
		const float2 disc = sqrt(u) * float2(cos(v), sin(v));
		const float2 finalP = p + disc * 0.05 * p.y;
		*/
		const float2 finalP = p;

		if (i >= 20)
			AddPoint(finalP, paletteSRV.SampleLevel(paletteSS, float2(c, 0.5), 0).rgb);
	}
}
