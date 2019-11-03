
cbuffer viewCB : register(b0)
{
	struct
	{
		float4x4 projViewTM;
		float4x4 invProjViewTM;
		float2 viewportSize;
		float2 invViewportSize;
		float3 viewPosition;
		uint   frameIdx;
	} view;
};



struct LightShade
{
	float3 position;
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



RaytracingAccelerationStructure Scene : register(t0);
Texture2D<float4> gBuffer0 : register(t1);
Texture2D<float4> gBuffer1 : register(t2);
Texture2D<float> depthBuffer : register(t3);
StructuredBuffer<LightKdNode> lightKdNodeSRV  : register(t4);
StructuredBuffer<LightKdLeaf> lightKdLeafSRV  : register(t5);
StructuredBuffer<uint>        lightIdsSRV     : register(t6);
StructuredBuffer<LightShade>  lightShadeSRV   : register(t7);

RWTexture2D<float4> RenderTarget : register(u0);
RWTexture2D<float3> momentsUAV : register(u1);



static const float FLT_MAX = 1024.0 * 1024.0;
static const float PI = acos(-1);



typedef BuiltInTriangleIntersectionAttributes MyAttributes;
struct RayPayload
{
    float4 color;
};



float3 UintToColor(uint v)
{
	float3 output;
	output.r = ((v >> 16) & 0xff) / 255.0;
	output.g = ((v >> 8) & 0xff) / 255.0;
	output.b = (v & 0xff) / 255.0;
	return output;
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

float RandUNorm(inout uint rngState)
{
	rngState = WangHash(rngState);
	return float(rngState) * (1.0 / 4294967296.0);
}

float2 RandUFloat2(inout uint rngState)
{
	return float2(RandUNorm(rngState), RandUNorm(rngState));
}



float3 SampleSphere(float2 sample)
{
	const float z = sample.x * 2 - 1;
	const float r = sqrt(max(0.0f, 1.0f - z * z));
	const float phi = 2 * PI * sample.y;
	return float3(r * cos(phi), r * sin(phi), z);
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



bool ShadowRay(in RayDesc ray)
{
	RayPayload payload = { float4(0, 0, 0, 0) };
	TraceRay(
		Scene,
		RAY_FLAG_CULL_BACK_FACING_TRIANGLES |
		RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH |
		RAY_FLAG_SKIP_CLOSEST_HIT_SHADER,
		~0, 0, 1, 0,
		ray,
		payload);
	return payload.color.a < 0;
}



[shader("raygeneration")]
void rgMain()
{
    const float2 coord = ((float2)DispatchRaysIndex() / (float2)DispatchRaysDimensions() * 2 - 1) * float2(1, -1);
	const uint2  pixel = DispatchRaysIndex().xy;
	
	const float hDepth = depthBuffer[pixel];
	float4 position = mul(view.invProjViewTM, float4(coord, hDepth, 1));
	position /= position.w;
	
	const float3 normal = gBuffer1[pixel].xyz;
	NormalBasis normalBasis = NormalBasisFromNormal(normal);
	
	uint rngState = WangHash(uint(pixel.x)*1280 + uint(pixel.y) + uint(view.frameIdx)*1280*720);
	
	// Sun light
//	const float3 sunDir = normalize(float3(1, 2, 3));
//	const float3 sunColor = float3(1.0, 0.85, 0.6) * 50.0;
//	const float3 skyColor = float3(0.6, 0.85, 1.0) * 10.0;
//	const float exposure = 2.0;

	// Moon light
	const float3 sunDir = normalize(float3(1, 2, 3));
	const float3 sunColor = float3(0.85, 0.95, 1.0) * 1.0;
	const float3 skyColor = float3(0.5, 0.8, 1.0) * 0.75;
	const float exposure = 1.5;

	// no sky lights, inside a cave
//	const float3 sunDir = normalize(float3(1, 2, 3));
//	const float3 sunColor = 0;
//	const float3 skyColor = 0;
//	const float exposure = 4.0;
	
	float3 currOutput = float3(0, 0, 0);
	
	{
		RayDesc ray;
		ray.Origin = position.xyz;
		ray.Direction = Transform(normalBasis, SampleCosineHemisphere(RandUFloat2(rngState)));
		ray.TMin = 0.0;
		ray.TMax = FLT_MAX;
		if (ShadowRay(ray))
			currOutput += skyColor;
	}
	
	{
		const float3 lightDir = SampleCone(sunDir, RandUFloat2(rngState), 0.015);
		const float3 sunLight = max(0, dot(normal, lightDir)) / PI;
		
		RayDesc ray;
		ray.Direction = SampleCone(lightDir, RandUFloat2(rngState), 0.015);
		ray.Origin = position.xyz;
		ray.TMin = 0.0;
		ray.TMax = FLT_MAX;
		const float visibility = ShadowRay(ray);
		
		currOutput += sunLight * sunColor * visibility;
	}
	
	{
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
			const float test = (axis == 1) ? position.x : (axis == 2) ? position.y : position.z;
			if (test < plane)
				kdNodeId = kdNodeId + 1;
			else
				kdNodeId = lightKdNodeSRV[kdNodeId].data >> 2;
		}
		
		#if 1
		const uint numSamples = 1;
				
		const LightKdLeaf kdLeaf = lightKdLeafSRV[kdLeafId];
	//	const uint numLights = 176;
		const uint numLights = kdLeaf.numLightIdx;		
		
		float3 accumLight = 0;
		for (uint i = 0; i < numSamples; ++i)
		{
		//	const uint lightIdx = uint(RandUNorm(rngState) * numLights);
			const uint lightIdx = lightIdsSRV[kdLeaf.firstLightIdx + uint(RandUNorm(rngState) * numLights)];
			
			LightShade light = lightShadeSRV[lightIdx];
			float3 lightPos = light.position + SampleSphere(RandUFloat2(rngState)) * 0.15;
			float3 wToLight = lightPos - position.xyz;
			const float dist = length(wToLight);
			wToLight = normalize(wToLight);
			const float3 color = UintToColor(light.color);
			const float ndotl = max(dot(wToLight, normal), 0.0);
			
			RayDesc ray;
			ray.Direction = wToLight;
			ray.Origin = position.xyz;
			ray.TMin = 0.0;
			ray.TMax = dist;
			const float visibility = ShadowRay(ray);
			
			const float attn = LightAttenuation(light.brightness, light.radius, dist);
			accumLight += color * ndotl * attn * light.brightness * visibility;
		}
		currOutput += accumLight * (numLights * rcp(numSamples));
		#endif
		
		#if 0
		const LightKdLeaf kdLeaf = lightKdLeafSRV[kdLeafId];
		const uint numLights = kdLeaf.numLightIdx;
		rngState = WangHash(kdLeafId);
	//	currOutput = float3(RandUNorm(rngState), RandUNorm(rngState), RandUNorm(rngState));
		currOutput = float3(numLights / 8.0, numLights / 16.0, numLights / 64.0);
		#endif
	}
	
	currOutput *= exposure;
	
	const float speed = 128;
	const float3 prevOutput = RenderTarget[pixel].rgb;
	const float histLenPrev = momentsUAV[pixel].z;
	const float histLen = min(histLenPrev + 1.0f, speed);
	const float alpha = max(rcp(speed), rcp(histLen));
	// const float3 output = lerp(prevOutput, currOutput, alpha);
	const float3 output = currOutput;
	// const float3 output = (pixel.x == 128 && pixel.y == 128) ? 1024 : 0.0;
	
	const float2 momPrev = momentsUAV[pixel].xy;
	const float currLum = dot(currOutput, rcp(3));
	const float2 momCurr = float2(currLum, currLum * currLum);
	const float2 mom = lerp(momPrev, momCurr, alpha);
	const float var = sqrt(max(mom.y - mom.x * mom.x, 0));
        
	RenderTarget[pixel] = float4(output, var);
	momentsUAV[pixel] = float3(mom, histLen);
}



[shader("closesthit")]
void chMain(inout RayPayload payload, in MyAttributes attr)
{
    float3 barycentrics = float3(1 - attr.barycentrics.x - attr.barycentrics.y, attr.barycentrics.x, attr.barycentrics.y);
    payload.color = float4(barycentrics, RayTCurrent());
}



[shader("miss")]
void msMain(inout RayPayload payload)
{
    payload.color = float4(0, 0, 0, -1);
}
