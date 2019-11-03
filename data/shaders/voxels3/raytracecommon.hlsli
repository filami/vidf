#ifndef RAYTRACECOMMON_H
#define RAYTRACECOMMON_H


static const float PI = acos(-1);


struct View
{
	float4x4 projViewTM;
	float4x4 invProjViewTM;
	float4x4 prevProjViewTM;
	float4x4 prevInvProjViewTM;
	float2   viewportSize;
	float2   invViewportSize;
	float3   viewPosition;
	uint     _;
	float3   prevViewPosition;
	uint     frameId;
	uint2    rayRate;
	uint2    __;
};



struct Ray
{
	float3 orig;
	float3 dir;
};



struct NormalBasis
{
	float3 xx;
	float3 yy;
	float3 normal;
};



float3 Transform(NormalBasis basis, float3 v)
{
	return basis.xx * v.x + basis.yy * v.y + basis.normal * v.z;
}



NormalBasis NormalBasisFromNormal(float3 normal)
{
	NormalBasis basis;
	basis.xx = cross(normal, float3(0.0f, 0.0f, 1.0f));
	if (abs(dot(basis.xx, basis.xx)) < (1.0f / 1024.0f))
		basis.xx = float3(0.0f, 1.0f, 0.0f);
	else
		basis.xx = normalize(basis.xx);
	basis.yy = normalize(cross(basis.xx, normal));
	basis.normal = normal;
	return basis;
}



float3 SampleHemisphere(float2 sample)
{
	const float z = sample.x;
	const float r = sqrt(max(0.0f, 1.0f - z * z));
	const float phi = 2 * PI * sample.y;
	return float3(r * cos(phi), r * sin(phi), z);
}



float3 SampleCone(float3 dir, float2 sample, float angleTan)
{
	float3 xx = cross(dir, float3(0, 0, 1));
	if (abs(dot(xx, xx)) < 0.99)
		xx = float3(0.0, 1.0, 0.0);
	else
		xx = normalize(xx);
	float3 yy = normalize(cross(xx, dir));
	float u = sample.x * PI * 2.0f;
	float r = sqrt(sample.y) * angleTan;
	float3 d;
	d.x = cos(u) * r;
	d.y = sin(u) * r;
	d.z = sqrt(1.0f - d.x * d.x - d.y * d.y);
	return normalize(d.x * xx + d.y * yy + d.z * dir);
}



float3 SampleCosineHemisphere(float2 sample)
{
	float u = sample.x * PI * 2.0f;
	float r = sqrt(sample.y);
	float3 d;
	d.x = cos(u) * r;
	d.y = sin(u) * r;
	d.z = sqrt(1.0f - d.x * d.x - d.y * d.y);
	return d;
}



float2 BoxRayIntersect(in float3 rayOrig, in float3 rayDir, in float3 minBox, in float3 maxBox)
{
	const float3 invRayDir = 1.0 / rayDir;
	const float3 t0 = (minBox - rayOrig) * invRayDir;
	const float3 t1 = (maxBox - rayOrig) * invRayDir;
	const float3 near = min(t0, t1);
	const float3 far = max(t0, t1);	
	const float2 output = float2(max(max(near.x, near.y), near.z), min(min(far.x, far.y), far.z));
	return output;
}



Ray PrimaryRay(in float4x4 invProjViewTM, in float3 viewPosition, in float2 viewportSize, in float2 pixel)
{
	Ray ray;
	ray.orig = viewPosition;
	const float2 coord = ((pixel / viewportSize) * 2 - 1) * float2(1, -1);	
	const float4 m = mul(invProjViewTM, float4(coord, 1, 1));
	ray.dir = normalize(m.xyz / m.w - viewPosition);
	return ray;
}



float4 NormalPointPlane(const float3 normal, const float3 pos)
{
	float4 result;
	result.xyz = normal;
	result.w = dot(normal, pos);
	return result;
}



float PlaneDist(float4 plane, float3 pos)
{
	return dot(plane.xyz, pos) - plane.w;
}



float sqr(float x)
{
	return x * x;
}



float Schlick(float cost, float n1, float n2)
{
	const float r0 = sqr((n1 - n2)/(n1 + n2));
	return r0 + (1 - r0) * pow(1 - cost, 5.0f);
}



float3 SphericalDirection(float sinTheta, float cosTheta, float phi)
{
	return float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}



bool SameHemisphere(float3 w, float3 wp)
{
	return w.z * wp.z > 0;
}



float3 SamplePowerHemisphere(float2 sample, float power)
{
	const float logSample = log(1.0f - sample.x);
	const float tan2Theta = -sqr(1 / power) * logSample;
	const float phi = sample.y * 2 * PI;

	const float cosTheta = 1 / sqrt(1 + tan2Theta);
	const float sinTheta = sqrt(max(0, 1 - cosTheta * cosTheta));

	const float3 wh = SphericalDirection(sinTheta, cosTheta, phi);
	if (!SameHemisphere(float3(0, 0, 1), wh))
		return -wh;
	return wh;
}


#endif