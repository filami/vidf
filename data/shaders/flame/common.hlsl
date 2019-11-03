#ifndef COMMON_HLSL
#define COMMON_HLSL


static const float pi = acos(-1.0);
static const float degToRad = pi / 180.0;


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



float sqr(float x)
{
	return x * x;
}



float2x3 MakeTransform(const float2 translation, const float angle, const float2 scale)
{
	const float2 sc = float2(sin(angle)*scale.x, cos(angle)*scale.y);
	float2x3 output;
	output[0] = float3(sc.y, -sc.x, translation.x);
	output[1] = float3(sc.x,  sc.y, translation.y);
	return output;
}


#endif
