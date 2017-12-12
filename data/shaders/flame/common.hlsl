#ifndef COMMON_HLSL
#define COMMON_HLSL


static const float pi = acos(-1.0);


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


#endif
