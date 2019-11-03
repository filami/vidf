cbuffer viewCB : register(b0)
{
	struct
	{
		float3 camPosition;
		float3 camFront;
		float3 camRight;
		float3 camUp;
		uint2  viewportSz;
	} view;
}



struct VSOutput
{
	float4 hPosition : SV_Position;
	float2 tc0       : TEXCOORD0;
};



VSOutput vsFullscreen(uint vertexId : SV_VertexId)
{
	float2 vertices[] =
	{
		float2(-1, -1),
		float2(-1,  3),
		float2(3, -1),
	};
	VSOutput output;
	output.hPosition = float4(vertices[vertexId], 0.5, 1.0);
	output.tc0 = vertices[vertexId];
	return output;
}



struct Ray
{
	float3 orig;
	float3 dir;
};



void Sierpinski(inout float3 p, inout float d, in float scale)
{
	if (p.x + p.y < 0) { float x1 = -p.y; p.y = -p.x; p.x = x1; }
	if (p.x + p.z < 0) { float x1 = -p.z; p.z = -p.x; p.x = x1; }
	if (p.y + p.z < 0) { float y1 = -p.z; p.z = -p.y; p.y = y1; }
	p.x = scale * p.x - (scale - 1);
	p.y = scale * p.y - (scale - 1);
	p.z = scale * p.z - (scale - 1);
	d /= scale;
}



void Menger(inout float3 p, inout float d, in float3 C, in float scale)
{
	p.x = abs(p.x); p.y = abs(p.y); p.z = abs(p.z);
	if (p.x - p.y < 0) { float x1 = p.y; p.y = p.x; p.x = x1; }
	if (p.x - p.z < 0) { float x1 = p.z; p.z = p.x; p.x = x1; }
	if (p.y - p.z < 0) { float y1 = p.z; p.z = p.y; p.y = y1; }
	
	p.x = scale * p.x - C.x * (scale-1);
	p.y = scale * p.y - C.y * (scale-1);
	p.z = scale * p.z;
	
	if (p.z > 0.5 * C.z * (scale-1))
		p.z -= C.z * (scale-1);
	d /= scale;
}



void RotateX(inout float3 p, in float a)
{
	const float c = cos(a);
	const float s = sin(a);
	p = float3(
		p.x,
		p.y * c - p.z * s,
		p.y * s + p.z * c);
}


void RotateY(inout float3 p, in float a)
{
	const float c = cos(a);
	const float s = sin(a);
	p = float3(
		p.x * c - p.z * s,
		p.y,
		p.x * s + p.z * c);
}


void RotateZ(inout float3 p, in float a)
{
	const float c = cos(a);
	const float s = sin(a);
	p = float3(
		p.x * c - p.y * s,
		p.x * s + p.y * c,
		p.z);
}


float sdRoundBox(float3 p, float3 b, float r)
{
  float3 d = abs(p) - b;
  return length(max(d,0.0)) - r
         + min(max(d.x,max(d.y,d.z)),0.0); // remove this line for an only partially signed sdf 
}




float KIFS1(in float3 p)
{
#if 0
/*
	float d = 1;
	for (uint i = 0; i < 5; ++i)
	{
	//	Sierpinski(p, d, 1.8);
		RotateX(p, 0.2);
		Menger(p, d, float3(1, 1, 1), 3.0);
		RotateY(p, 0.1);
	}
	
	return (length(p) - 2) * d;
	*/	
	
	RotateX(p, -0.5);
	
	float d = 1;
	for (uint i = 0; i < 2; ++i)
	{
		RotateX(p, 3);
		Menger(p, d, float3(1, 1, 1), 3.0);
	//	RotateY(p, 0.1);
	}
	
//	return (max(abs(p.x), max(abs(p.y), abs(p.z))) - 1) * d;
	return (length(p) - 1) * d;
#endif

#if 1
	int i;
	float r, x, y, z;
	x = p.x;
	y = p.y;
	z = p.z;
	
	const int MI = 24;
	const float bailout = 4.0;
	const float scale = 1.25;
	const float CX = 1.0;
	const float CY = 1.0;
	const float CZ = 1.0;

	r=x*x+y*y+z*z;
	[loop]
	for(i=0;i<MI && r<bailout;i++){
		p = float3(x, y, z);
	//	RotateX(p, 0.5);
		x = p.x;
		y = p.y;
		z = p.z;
		
		x=abs(x);y=abs(y);z=abs(z);
		if(x-y<0){float x1=y;y=x;x=x1;}
		if(x-z<0){float x1=z;z=x;x=x1;}
		if(y-z<0){float y1=z;z=y;y=y1;}
		
		p = float3(x, y, z);
		RotateY(p, -1.25);
		x = p.x;
		y = p.y;
		z = p.z;		
		
		x=scale*x-CX*(scale-1);
		y=scale*y-CY*(scale-1);
		z=scale*z;
		if(z>0.5*CZ*(scale-1)) z-=CZ*(scale-1);
		
		r = x*x+y*y+z*z;
	}
	return (sqrt(r) - 1)*pow(scale, (-i));
#endif

#if 0
	p.x = frac(p.x);

	float3 p2 = p * 2 - 1;
	RotateZ(p2, 0.5);
	float d1 = sdRoundBox(p2, float3(3.5, 0.5, 1), 0.15);
	
	p2 = (p + float3(1, 0, 0)) * 2 - 1;
	RotateZ(p2, 0.5);
	float d2 = sdRoundBox(p2, float3(3.5, 0.5, 1), 0.15);
	
	p2 = (p - float3(1, 0, 0)) * 2 - 1;
	RotateZ(p2, 0.5);
	float d3 = sdRoundBox(p2, float3(3.5, 0.5, 1), 0.15);
	
//	return d1 / 2.0;
	return min(d1, min(d2, d3)) / 2.0;
#endif

#if 0
	p.x = frac(p.x);

	float3 p2 = p * 2 - 1;
	RotateZ(p2, 1.0);
	float d1 = sdRoundBox(p2, float3(1.5, 0.5, 1), 0.15) / 2.0;
	
	p2 = (p - float3(1, 0, 0)) * 2 - 1;
	RotateZ(p2, 1.0);
	float d2 = max(frac(1 - p.x), sdRoundBox(p2, float3(1.5, 0.5, 1), 0.15) / 2.0);
	
	p2 = (p + float3(1, 0, 0)) * 2 - 1;
	RotateZ(p2, 1.0);
	float d3 = max(frac(p.x), sdRoundBox(p2, float3(1.5, 0.5, 1), 0.15) / 2.0);
	
	return min(d1, min(d2, d3));
#endif
}



float KIFS2(in float3 p)
{
	float d = 1;
	
	p /= 4.0;
	d *= 4.0;
	p.y += 0.5;
	
	for (uint i = 0; i < 38; ++i)
	{
		RotateY(p, 3.1415 * 0.25);
		Sierpinski(p, d, 1.5);
	}
	
	return (length(p) - 1) * d;
}



float EstimateDistance(in float3 p)
{
	const float d1 = KIFS1(p);
	const float d2 = KIFS2(p);
	return min(d1, d2);
}



float3 EstimateNormal(in float3 p, in float eps)
{
	float2 veps = float2(eps, 0.0);
    float3 nor;
    nor.x = EstimateDistance(p + veps.xyy) - EstimateDistance(p - veps.xyy);
    nor.y = EstimateDistance(p + veps.yxy) - EstimateDistance(p - veps.yxy);
    nor.z = EstimateDistance(p + veps.yyx) - EstimateDistance(p - veps.yyx);
    return normalize(nor);
}



float4 psKaleidoscopeIFS(VSOutput input) : SV_Target
{
	Ray ray;
	ray.orig = view.camPosition;
	ray.dir = normalize(view.camFront + view.camRight * input.tc0.x + view.camUp * input.tc0.y);
	float tdist = 0;
	
	const uint maxSteps = 256;
	
	uint i = 0;
	[loop]
	for (; i < maxSteps; ++i)
	{
		const float dist = EstimateDistance(ray.orig);
		if (dist < tdist * 0.001)
			break;
		ray.orig += ray.dir * dist * 0.5;
		tdist += dist * 0.5;
	}
	float3 n = EstimateNormal(ray.orig, tdist * 0.001);
	
//	return i  == 256;
//	return float4(i.xxx / float(maxSteps), 1) * (i != maxSteps);
	return float4((n * 0.5 + 0.5) * (1 - (i / float(maxSteps))), 1);
}
