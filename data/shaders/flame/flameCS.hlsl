Texture2D paletteSRV : register(t0);
SamplerState paletteSS : register(s0);
RWTexture2D<float4> histogramUAV : register(u0);


cbuffer viewCB : register(b0)
{
	struct
	{
		float4x4 projViewTM;
		float3   cameraPos;
		uint2    viewport;
		int      rngOffset;
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



static const float pi = acos(-1.0);


void AddPoint(const float3 p, const float3 color)
{
	const float4 hPosition = mul(view.projViewTM, float4(p, 1));
	const float2 screenCoord = hPosition.xy / hPosition.w;

	const float focalDist = 1.4;
	const float cocSize = 0.035;
	const float dist = distance(p, view.cameraPos);
	const float coc = (-1.0/dist + 1.0/focalDist) * cocSize;
		
	// Depth of Field works like marvel!!
	const float u = RandUNorm();
	const float v = RandUNorm() * 2 * pi;
	const float2 disc = sqrt(u) * float2(cos(v), sin(v));
	const float2 finalP = screenCoord + disc * coc;

	const uint2 c = (finalP * 0.5 + 0.5) * view.viewport;
	if (hPosition.z > 0.0 && c.x < view.viewport.x && c.y < view.viewport.y)
		histogramUAV[c] += float4(color, 1);
}


void varLinear(inout float3 fp, const float3 ft, const float weigth)
{
	fp += ft * weigth;
}



void varJuliaN(inout float3 fp, const float3 ft, const float phi, const float r, const float weigth, const float power, const float dist)
{
	const uint p3 = abs(power) * RandUNorm();
	const float t = (phi + 2 * pi * p3) / power;
	fp.xy += pow(r, dist / power) * float2(cos(t), sin(t)) * weigth;
	fp.z += ft.z * weigth;
}



void varJuliaScope(inout float3 fp, const float3 ft, const float phi, const float r, const float weigth, const float power, const float dist)
{
	const uint p3 = abs(power) * RandUNorm();
	const float t = (sign(RandSNorm()) * phi + 2 * pi * p3) / power;
	fp.xy += pow(r, dist / power) * float2(cos(t), sin(t)) * weigth;
	fp.z += ft.z * weigth;
}



void varFishEye(inout float3 fp, const float3 ft, const float r, const float weigth)
{
	fp.xy += (2 / (1 + r)) * ft.yx * weigth;
	fp.z += ft.z * weigth;
}


void varEyeFish(inout float3 fp, const float3 ft, const float r, const float weigth)
{
	fp.xy += (2 / (1 + r)) * ft.xy * weigth;
	fp.z += ft.z * weigth;
}


void varBlur(inout float3 fp, const float3 ft, const float weigth)
{
	const float u = RandUNorm() * 2 * pi;
	fp.xy += RandUNorm() * float2(cos(u), sin(u)) * weigth;
	fp.z += ft.z * weigth;
}


void varZTranslate(inout float3 fp, const float3 ft, const float weigth)
{
	fp.z += weigth;
}


void varZCone(inout float3 fp, const float3 ft, const float r, const float weigth)
{
	fp.z += r * weigth;
}


void varZScale(inout float3 fp, const float3 ft, const float weigth)
{
	fp.z += ft.z * weigth;
}


void varFlatten(inout float3 fp)
{
	fp.z = 0.0;
}


[numthreads(64, 1, 1)]
void csFlame(uint3 threadId : SV_DispatchThreadID)
{
	rngState = threadId.x + view.rngOffset;
	
	float3 p = float3(RandSNorm(), RandSNorm(), 0.0);
	const float3 ip = p;
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
		const float f = RandUNorm();
		float ci;	
		
		if (f < 0.1)
		{
			const float3 ft = float3(p.xy*0.75, p.z);
			const float r = length(ft.xy);
			p = 0.0;
			
			varFlatten(p);
			varLinear(p, ft, 1.0);
			varBlur(p, ft, 0.015);
			ci = 0.05;
		}
		else if (f < 0.65)
		{
			const float3 ft = float3(p.xy * 0.75 + float2(-0.25, 0.25), p.z);
			const float r = length(ft.xy);
			const float phi = atan2(ft.y, ft.x);
			p = 0.0;
			
			// varZTranslate(p, ft, 0.05);
			// varZCone(p, ft, r, 0.1);
			varJuliaN(p, ft, phi, r, 0.65, 4.0, -1.0);
			ci = 0.5;
		}
		else if (f < 0.85)
		{
			const float3 ft = float3(p.xy * float2(-6, 0.5) + float2(-0.2, 0.5), p.z);
			const float r = length(ft.xy);
			const float phi = atan2(ft.y, ft.x);
			p = 0.0;

			// varZCone(p, ft, r, 0.005);
			varJuliaScope(p, ft, phi, r, 0.75, -8.0, 2.0);
			ci = 0.9;
		}
		else
		{
			const float3 ft = float3(p.xy * 1.5, p.z);
			const float r = length(ft.xy);
			p = 0.0;

			// varZCone(p, ft, r, -0.05);
			varZTranslate(p, ft, -0.05);
			varZScale(p, ft, 0.25);
			varLinear(p, ft, 0.5);
			varEyeFish(p, ft, r, 0.5);
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
		const float3 finalP = p;

		if (i >= 20)
			AddPoint(finalP, paletteSRV.SampleLevel(paletteSS, float2(c, 0.5), 0).rgb);
	}
}
