#ifndef VARIABLES_HLSL
#define VARIABLES_HLSL

#include "common.hlsl"


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

#endif
