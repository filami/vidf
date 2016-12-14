//               https://github.com/ashima/webgl-noise

float4 mod289(float4 x) 
{
	return x - floor(x * (1.0 / 289.0)) * 289.0; 
}

float mod289(float x) 
{
	return x - floor(x * (1.0 / 289.0)) * 289.0; 
}

float4 permute(float4 x) 
{
	return mod289(((x*34.0)+1.0)*x);
}

float permute(float x) 
{
	return mod289(((x*34.0)+1.0)*x);
}

float4 taylorInvSqrt(float4 r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

float taylorInvSqrt(float r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

float fract(float x) { return x - floor(x); }

float4 grad4(float j, float4 ip)
{
	const float4 ones = float4(1.0, 1.0, 1.0, -1.0);
	float4 p,s;

	p.xyz = floor( frac(j * ip.xyz) * 7.0) * ip.z - 1.0;
	p.w = 1.5 - dot( abs(p.xyz), ones.xyz );
	p.xyz -= sign(p.xyz) * (p.w < 0);

	return p;
}
  
// (sqrt(5) - 1)/4 = F4, used once below
#define F4 0.309016994374947451

// this returns the value of the simplex noise 
// function in the w component and the 3D gradient 
// in the xyz components
float4 SNoiseGrad(float4 v)
{
	const float4  C = float4(
		0.138196601125011,	// (5 - sqrt(5))/20  G4
		0.276393202250021,	// 2 * G4
		0.414589803375032,	// 3 * G4
		-0.447213595499958);// -1 + 4 * G4

	// First corner
	float4 i = floor(v + dot(v, 0.309016994374947451));
	float4 x0 = v -   i + dot(i, C.xxxx);

	// Other corners

	// Rank sorting originally contributed by Bill Licea-Kane, AMD (formerly ATI)
	float4 i0;
	float3 isX = step( x0.yzw, x0.xxx );
	float3 isYZ = step( x0.zww, x0.yyz );
	//  i0.x = dot( isX, float3( 1.0 ) );
	i0.x = isX.x + isX.y + isX.z;
	i0.yzw = 1.0 - isX;
	//  i0.y += dot( isYZ.xy, float2( 1.0 ) );
	i0.y += isYZ.x + isYZ.y;
	i0.zw += 1.0 - isYZ.xy;
	i0.z += isYZ.z;
	i0.w += 1.0 - isYZ.z;

	// i0 now contains the unique values 0,1,2,3 in each channel
	float4 i3 = clamp( i0, 0.0, 1.0 );
	float4 i2 = clamp( i0-1.0, 0.0, 1.0 );
	float4 i1 = clamp( i0-2.0, 0.0, 1.0 );

	//  x0 = x0 - 0.0 + 0.0 * C.xxxx
	//  x1 = x0 - i1  + 1.0 * C.xxxx
	//  x2 = x0 - i2  + 2.0 * C.xxxx
	//  x3 = x0 - i3  + 3.0 * C.xxxx
	//  x4 = x0 - 1.0 + 4.0 * C.xxxx
	float4 x1 = x0 - i1 + C.xxxx;
	float4 x2 = x0 - i2 + C.yyyy;
	float4 x3 = x0 - i3 + C.zzzz;
	float4 x4 = x0 + C.wwww;

	// Permutations
	i = mod289(i); 
	float j0 = permute( permute( permute( permute(i.w) + i.z) + i.y) + i.x);
	float4 j1 = permute( permute( permute( permute (
		  i.w + float4(i1.w, i2.w, i3.w, 1.0 ))
		+ i.z + float4(i1.z, i2.z, i3.z, 1.0 ))
		+ i.y + float4(i1.y, i2.y, i3.y, 1.0 ))
		+ i.x + float4(i1.x, i2.x, i3.x, 1.0 ));

	// Gradients: 7x7x6 points over a cube, mapped onto a 4-cross polytop
	// 7*7*6 = 294, which is close to the ring size 17*17 = 289.
	float4 ip = float4(1.0/294.0, 1.0/49.0, 1.0/7.0, 0.0) ;

	float4 p0 = grad4(j0,   ip);
	float4 p1 = grad4(j1.x, ip);
	float4 p2 = grad4(j1.y, ip);
	float4 p3 = grad4(j1.z, ip);
	float4 p4 = grad4(j1.w, ip);

	// Normalise gradients
	float4 norm = taylorInvSqrt(float4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
	p0 *= norm.x;
	p1 *= norm.y;
	p2 *= norm.z;
	p3 *= norm.w;
	p4 *= taylorInvSqrt(dot(p4,p4));

	// Mix contributions from the five corners
	float3 m10 = max(0.6 - float3(dot(x0, x0), dot(x1, x1), dot(x2, x2)), 0.0);
	float2 m11 = max(0.6 - float2(dot(x3, x3), dot(x4, x4)), 0.0);
	float3 m20 = m10 * m10;
	float2 m21 = m11 * m11;
	float3 m40 = m20 * m20;
	float2 m41 = m21 * m21;

	float d0 = dot(p0, x0);
	float d1 = dot(p1, x1);
	float d2 = dot(p2, x2);
	float d3 = dot(p3, x3);
	float d4 = dot(p4, x4);

	float4 result;
	result.w = (dot(m40, float3(d0, d1, d2)) + dot(m41, float2(d3, d4)));

	float3 m30 = m20 * m10;
	float2 m31 = m21 * m11;

	float4 grad 
		  = (x0 * d0) * m30.x;
	grad += (x1 * d1) * m30.y;
	grad += (x2 * d2) * m30.z;
	grad += (x3 * d3) * m31.x;
	grad += (x4 * d4) * m31.y;
	grad *= -6.0;
	grad += p0 * m40.x;
	grad += p1 * m40.y;
	grad += p2 * m40.z;
	grad += p3 * m41.x;
	grad += p4 * m41.y;

	// we do not return the fourth component of the gradient at the moment
	result.xyz = grad.xyz;
	result *= 49.0;
	return result;
}
