
struct OIT
{
	half4 fragments[8];
	float depth[8];
	uint  numFrags;
};

RasterizerOrderedStructuredBuffer<OIT> rovTestROV : register(u0);

Texture2D solidSRV : register(t0);
StructuredBuffer<OIT> rovTestSRV : register(t1);

static const float pi = acos(-1.0);


struct LightShade
{
	float3 position;
	float3 normal;
	float  radius;
	float  brightness;
	uint   color;
};

struct LightKdNode
{
	// first 2 bits:
	//	0 = leaf
	//	1 = x plane
	//	2 = y plane
	//	3 = z plane
	// For Nodes:
	//  30 bits for above child index
	// For Leafs:
	//	30 bits for light data index
	uint  data;
	float plane;
};


struct LightKdLeaf
{
//	float3 boundsMin;
//	float3 boundsMax;
	uint   firstLightIdx;
	uint   numLightIdx;
};


StructuredBuffer<LightKdNode> lightKdNodeSRV  : register(t0);
StructuredBuffer<LightKdLeaf> lightKdLeafSRV : register(t1);
StructuredBuffer<uint>        lightIdsSRV : register(t2);
StructuredBuffer<LightShade>  lightShadeSRV : register(t3);

Texture2DArray shadowSRV : register(t4);
StructuredBuffer<float4x4>  shadowTMSRV : register(t5);

Texture2D diffuseSRV : register(t6);
SamplerState diffuseSS : register(s0);
SamplerComparisonState shadowSS : register(s1);


cbuffer viewCB : register(b0)
{
	struct
	{
		float4x4 projTM;
		float4x4 viewTM;
		float2 viewportSize;
		float2 invViewportSize;
		float3 viewPosition;
		uint   numLights;
	} view;
};



cbuffer cascadeShadowCB : register(b1)
{
	struct
	{
		float4x4 projTM;
		float3 lightDir;
		float2 texelSize;
	} shadow;
};


struct Input
{
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 texCoord : TEXCOORD0;
};


struct Output
{
	float4 hPosition : SV_Position;
	float3 wPosition : TEXCOORD0;
	float3 wNormal : TEXCOORD1;
	float2 texCoord : TEXCOORD2;
};


Output vsMain(Input input)
{
	Output output;
	output.hPosition = mul(view.projTM, mul(view.viewTM, float4(input.position, 1.0)));
	output.wPosition = input.position;
	output.wNormal = input.normal.xyz;
	output.texCoord = input.texCoord;

	return output;
}



void psOITClear(float4 coord : SV_Position)
{
	const uint idx = coord.x + view.viewportSize.x * coord.y;
	for (int i = 0; i < 8; i++)
	{
		// rovTestROV[idx].fragments[i] = half4(0, 0, 0, 1);
		rovTestROV[idx].depth[i] = 1.0;
	}
//	rovTestROV[idx].depth[0] = 1.0;
	rovTestROV[idx].numFrags = 0;
}



float LightAttenuation(float brightness, float lightRadius, float distance)
{
	const float x = saturate(distance * rcp(lightRadius));
	return rcp(1 + 32*x*x) * (1 - smoothstep(0.5, 1.0, x));
}


uint WangHash(uint seed)
{
	seed = (seed ^ 61) ^ (seed >> 16);
	seed *= 9;
	seed = seed ^ (seed >> 4);
	seed *= 0x27d4eb2d;
	seed = seed ^ (seed >> 15);
	return seed;
}


float RandSNorm(inout uint seed)
{
	seed = WangHash(seed);
	return float(seed) * (1.0 / 4294967296.0) * 2 - 1;
}


float3 IndexToColor(uint v)
{
	uint seed = v;
	float3 output;
	output.x = RandSNorm(seed);
	output.y = RandSNorm(seed);
	output.z = RandSNorm(seed);
	return output;
}



float3 UintToColor(uint v)
{
	float3 output;
	output.r = ((v >> 16) & 0xff) / 255.0;
	output.g = ((v >> 8) & 0xff) / 255.0;
	output.b = (v & 0xff) / 255.0;
	return output;
}


float Schlick(float g, float costh)
{
	float k = 1.55 * g - 0.55 * g * g * g;
	return (1.0 - k * k) / (4.0 * pi * pow(1.0 - k * costh, 2.0));
}



float2 CascadeShadowLevel(float3 wPosition, uint cascadeIdx)
{
	float3 hp = mul(shadowTMSRV[cascadeIdx], float4(wPosition, 1.0)).xyz;
	hp.y *= -1.0;
	hp.xy = hp.xy * 0.5 + 0.5;
	const float shadow = shadowSRV.SampleCmpLevelZero(shadowSS, float3(hp.xy, cascadeIdx), hp.z);
	const float coverage = saturate(min(min(min(hp.x, 1-hp.x), min(hp.y, 1-hp.y)), min(hp.z, 1-hp.z)) * 128);
	return float2(shadow, coverage);
}


float SunShadow(float3 wPosition)
{
	const uint numCascades = 3;
	
	float finalShadow = CascadeShadowLevel(wPosition, numCascades-1).x;
	[unroll]
	for (int cascade = numCascades - 2; cascade >= 0; --cascade)
	{
		const float2 shadow = CascadeShadowLevel(wPosition, cascade);
		finalShadow = lerp(finalShadow, shadow.x, shadow.y);
	}
	
	return finalShadow;
}


void AccumLight(const float3 wPosition, const float3 wNormal, out float3 diffuseAccum)
{
	diffuseAccum = 0;
	
	uint kdNodeId = 0;
	uint kdLeafId = 0;
	while (true)
	{
		const uint data = lightKdNodeSRV[kdNodeId].data;
		const uint axis = data & 0x3;
		if (axis == 0)
		{
			kdLeafId = data >> 2;
			break;
		}
		const float plane = lightKdNodeSRV[kdNodeId].plane;
		const float test = (axis == 1) ? wPosition.x : (axis == 2) ? wPosition.y : wPosition.z;
		if (test < plane)
			kdNodeId = kdNodeId + 1;
		else
			kdNodeId = lightKdNodeSRV[kdNodeId].data >> 2;
	}

	float sum = 0.0;
	const LightKdLeaf kdLeaf = lightKdLeafSRV[kdLeafId];
	for (uint i = kdLeaf.firstLightIdx; i < kdLeaf.firstLightIdx + kdLeaf.numLightIdx; ++i)
	{
		LightShade light = lightShadeSRV[lightIdsSRV[i]];
		float3 wToLight = light.position - wPosition;
		const float dist = length(wToLight);
		wToLight = normalize(wToLight);
		const float3 color = UintToColor(light.color);
		const float ndotl = max(dot(wToLight, wNormal), 0.0);
		// const float attn = LightAttenuation(light.brightness, light.radius, dist);
		// diffuseAccum += ndotl * color * attn;
		// diffuseAccum += color * attn * light.brightness * 4.0;
		
		const float attn = LightAttenuation(light.brightness, light.radius, dist) * saturate(dot(light.normal, wNormal));
		diffuseAccum += color * attn * light.brightness * 4.0;
		sum += attn;
	}
	diffuseAccum /= sum;

	const float sunShadow = SunShadow(wPosition);
	diffuseAccum += max(dot(shadow.lightDir, wNormal), 0.0) * sunShadow * 15.5;
}



float3 ShadePixel(Output input)
{
	const float3 wNormal = normalize(input.wNormal);
	const float3 diffuse = diffuseSRV.Sample(diffuseSS, input.texCoord).rgb;
	float3 diffuseAccum;
	AccumLight(input.wPosition, wNormal, diffuseAccum);
	float3 output = diffuse * diffuseAccum;

	///

	/*
	// const float density = 0.35;
	const float density = 0.015;

	const float step = 0.2;
	const float3 wToLight = -shadow.lightDir;
	const float3 wToCamera = normalize(view.viewPosition - input.wPosition);
	const float dist = distance(view.viewPosition, input.wPosition);
	const float costh = dot(-shadow.lightDir, wToCamera);
	const float ph = Schlick(0.5, costh);
	for (float t = 0.0; t < dist; t += step)
	{
		const float sunShadow = SunShadow(input.wPosition + wToCamera * t);
		output *= exp(-density*step);
		output += density * step * ph * sunShadow * 5.5 * 0.15;
	}
	*/
	
	return output;
}


float4 psMain(Output input) : SV_Target
{
	return float4(ShadePixel(input), 1);
}



[earlydepthstencil]
void psMainOIT(Output input)
{
	const float3 wNormal = normalize(input.wNormal);
	const float3 diffuse = ShadePixel(input);
	const float3 emissive = 0;
	// const float filter = (1 - pow(max(0, dot(normalize(view.viewPosition - input.wPosition), wNormal)), 1.0 / 4.0));
	const float filter = 0.66;
	
	const uint pxIdx = input.hPosition.x + view.viewportSize.x * input.hPosition.y;
	rovTestROV[pxIdx].numFrags = min(8, rovTestROV[pxIdx].numFrags + 1);
	half4 fragment = half4(diffuse * filter + emissive, 1.0 - filter);
	float depth = input.hPosition.z;
	for (uint i = 0; i < rovTestROV[pxIdx].numFrags; i++)
	{
		if (input.hPosition.z < rovTestROV[pxIdx].depth[i])
		{
			half4 tempFragment = fragment;
			float tempDepth = depth;
			fragment = rovTestROV[pxIdx].fragments[i];
			depth = rovTestROV[pxIdx].depth[i];
			rovTestROV[pxIdx].fragments[i] = tempFragment;
			rovTestROV[pxIdx].depth[i] = tempDepth;
		}
	}
}



Texture2D blur0 : register(t2);
Texture2D blur1 : register(t3);
Texture2D blur2 : register(t4);
Texture2D blur3 : register(t5);
Texture2D blur4 : register(t6);


struct VSOutput
{
	float4 hPosition : SV_Position;
	float2 tc0 : TEXCOORD0;
};



VSOutput vsFinalMain(uint vertexId : SV_VertexId)
{
	float2 vertices[] =
	{
		float2(-1, -1),
		float2(-1,  3),
		float2(3, -1),
	};
	VSOutput output;
	output.hPosition = float4(vertices[vertexId], 0.5, 1.0);
	output.tc0 = vertices[vertexId] * float2(0.5, -0.5) + 0.5;
	return output;
}



float3 rgb2hsv(float3 c)
{
	float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
	float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10;
	return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}



float3 hsv2rgb(float3 c)
{
	float4 K = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	float3 p = abs(frac(c.xxx + K.xyz) * 6.0 - K.www);
	return c.z * lerp(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}



float4 psFinalMain(VSOutput input) : SV_Target
{
	const uint idx = input.hPosition.x + view.viewportSize.x * input.hPosition.y;

	// float3 output = 0.15;
	float3 output = solidSRV[input.hPosition.xy].rgb;

	for (int i = rovTestSRV[idx].numFrags - 1; i >= 0; --i)
		output = output * rovTestSRV[idx].fragments[i].a + rovTestSRV[idx].fragments[i].rgb;
		
	output =
		output * 0.75 + 
		blur0.Sample(diffuseSS, input.tc0).rgb * 0.2 +
		blur1.Sample(diffuseSS, input.tc0).rgb * 0.15 +
		blur2.Sample(diffuseSS, input.tc0).rgb * 0.1 +
		blur3.Sample(diffuseSS, input.tc0).rgb * 0.05 +
		blur4.Sample(diffuseSS, input.tc0).rgb * 0.025;
		
		
	static const float brightness = 1.5;
	static const float saturation = 0.95;
	static const float vibrance = 1.2;
	output = saturate(1-exp(-output * brightness));
	float3 hsv = saturate(rgb2hsv(output));
	hsv.y = pow(hsv.y * saturation, 1.0 / vibrance);
	output = hsv2rgb(hsv);

	return float4(output, 1.0);
}
