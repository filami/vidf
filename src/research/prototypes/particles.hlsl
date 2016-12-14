
#include "snoise.hlsl"

DepthStencilState noDepthTest
{
	DepthEnable = false;
	DepthWriteMask = 0;
};

RasterizerState noCulling
{
	FillMode = Solid;
	CullMode = None;
};

BlendState disableBlend
{
	BlendEnable[0] = false;
};

SamplerState Nearest
{
	Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;
};


///////////////////////////////////////////////////////////

uint RandXorshift(uint rngState)
{
    rngState = rngState ^ (rngState << 13);
    rngState = rngState ^ (rngState >> 17);
    rngState = rngState ^ (rngState << 5);
    return rngState;
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

uint Rand(inout uint randKey)
{
	++randKey;
	return WangHash(RandXorshift(randKey));
}

float RandUNorm(inout uint randKey)
{
	return float(Rand(randKey)) * (1.0f / 4294967296.0f);
}

float RandSNorm(inout uint randKey)
{
	return RandUNorm(randKey)*2.0-1.0;
}

float2 RandCircle(inout uint randKey)
{
	const float theta = RandUNorm(randKey) * 2.0f * 3.1415;
	float s = sin(theta);
	float c = cos(theta);
	return float2(c, s);
}

///////////////////////////////////////////////////////////


cbuffer perViewCB : register(b0)
{
	float4x4 pv_viewProjectionTM;
	float2 pv_viewportSize;
	float2 pv_invViewportSize;
	float  pv_sceneTime;
	float  pv_deltaTime;
};


struct Particle
{
	float3 position;
	float3 velocity;
	float age;
	float invLifetime;
};

RWStructuredBuffer<Particle> particlesUAV : register(u0);
RWTexture2D<float> frameBufferRedUAV : register(u1);
RWTexture2D<float> frameBufferGreenUAV : register(u2);
RWTexture2D<float> frameBufferBlueUAV : register(u3);
Texture2D frameBufferRedSRV : register(t0);
Texture2D frameBufferGreenSRV : register(t1);
Texture2D frameBufferBlueSRV : register(t2);


float3 Curl(float4 sample)
{
	float4 offy = float4(149, 311, 191, 491);
	float4 offz = float4(233, 197, 43, 59);
	float3 gradX = SNoiseGrad(sample).xyz;
	float3 gradY = SNoiseGrad(sample + offy).xyz;
	float3 gradZ = SNoiseGrad(sample + offz).xyz;
	
	return float3(
		gradY.z - gradZ.y,
		gradZ.x - gradX.z,
		gradX.y - gradY.x);
}


void SpawnParticle(inout Particle particle, inout uint randKey)
{
	const float t = RandUNorm(randKey) * 2.0 * 3.1415;
	const float3 dir = float3(cos(t), sin(t), 0.0);	
	
//	particle.position = float3(0, 0, 0);
//	particle.velocity = dir * (RandUNorm(randKey) * 0.5 + 0.5) * 0.5;

	// particle.position = float3(-1, RandSNorm(randKey)*0.05, RandSNorm(randKey)*0.05);
	const float2 circle = RandCircle(randKey) * pow(RandUNorm(randKey), 32.0) * 0.05;
	// particle.position = float3(-1, RandSNorm(randKey)*2.0, RandSNorm(randKey)*2.0);
	particle.position = float3(-1, circle);
	particle.velocity = float3(RandUNorm(randKey)*0.25+2.2, RandSNorm(randKey)*0.2, RandSNorm(randKey)*0.2);

	particle.age = 0.0;
	particle.invLifetime = 1.0 / (RandUNorm(randKey) * 0.75 + 1.2);
}


void UpdateAge(inout Particle particle, float dT)
{
	particle.age += particle.invLifetime * dT;
}


void Move(inout Particle particle, float dT, float drag, float3 velocityField)
{
	particle.velocity = lerp(velocityField, particle.velocity, exp(-drag*dT));
	particle.position += particle.velocity * dT;
}


float3 GetColor(Particle particle)
{
	const float realAge = particle.age * rcp(particle.invLifetime);
	const float beat1 = exp(-frac(pv_sceneTime * 2.5 + (1-realAge)*0.5)*12.0);
	const float beat2 = exp(-frac(pv_sceneTime * 2.5 + (1-realAge)*1.5)*4.0);
	const float beat3 = exp(-frac(pv_sceneTime * 2.5)*2.0);

	const float c = saturate(1.0 - particle.age);
	const float c1 = pow(c, 2.0);
	const float c2 = pow(c, 6.0);
	
	return
		(float3(c1, c, c) * beat2 * float3(0.5, 0.75, 1.0) * 0.015 +
		float3(c1, c2, c2*0.5) * beat1 * 0.05) * beat3;
}


[numthreads(1024, 1, 1)] 
void InitCS(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
	const uint particleId = groupID.x * 1024 + groupThreadID.x;
	uint randKey = particleId;
	
	SpawnParticle(particlesUAV[particleId], randKey);
}


[numthreads(1024, 1, 1)] 
void UpdateCS(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
	const uint particleId = groupID.x * 1024 + groupThreadID.x;
	Particle particle = particlesUAV[particleId];
	uint randKey = particleId;
	
	float dT = pv_deltaTime;
	if (particle.age > 1.0)
	{
		SpawnParticle(particle, randKey);
		dT *= RandUNorm(randKey);
	}
	
	const float3 curl = Curl(float4(particlesUAV[particleId].position*4.0, pv_sceneTime*1.5));
	const float3 targetVelocity = curl*0.8 + float3(0.35, 0.0, 0.0);
	
	UpdateAge(particle, dT);
	Move(particle, dT, 2.0, targetVelocity);
	
	particlesUAV[particleId] = particle;
}



[numthreads(1024, 1, 1)] 
// [numthreads(32, 1, 1)] 
void DrawParticlesCS(uint3 groupID : SV_GroupID, uint3 groupThreadID : SV_GroupThreadID)
{
	const uint particleId = groupID.x * 1024 + groupThreadID.x;
	
	const float4 hPosition = mul(float4(particlesUAV[particleId].position, 1), pv_viewProjectionTM);
	if (hPosition.w < 0.0)
		return;
	
	// const int2 pixelCoord = (particlesUAV[particleId].position.xy*0.5+0.5) * pv_viewportSize;
	const int2 pixelCoord = (hPosition.xy/hPosition.w*0.5+0.5) * pv_viewportSize;
	
	if (pixelCoord.x >= 0 && pixelCoord.y >= 0 && pixelCoord.x < pv_viewportSize.x && pixelCoord.y < pv_viewportSize.y)
	{
		const float3 color = GetColor(particlesUAV[particleId]) /* 32*/;
		frameBufferRedUAV[pixelCoord] += color.r;
		frameBufferGreenUAV[pixelCoord] += color.g;
		frameBufferBlueUAV[pixelCoord] += color.b;
	}
}



struct QuadVSOut
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
};

QuadVSOut QuadVS(uint vertexId : SV_VertexId)
{
	QuadVSOut output;
	const float2 vertices[] =
	{
		float2(-1, -1),
		float2(3, -1),
		float2(-1, 3),
	};
	output.position = float4(vertices[vertexId], 0, 1);
	output.texCoord = vertices[vertexId]*0.5+0.5;
	return output;
}


float4 CopyToFramePS(QuadVSOut input) : SV_Target
{
//	const float3 inputColor = frameBufferSRV.Sample(Nearest, input.texCoord).rgb;
	const float3 inputColor = float3(
		frameBufferRedSRV.Sample(Nearest, input.texCoord).r,
		frameBufferGreenSRV.Sample(Nearest, input.texCoord).r,
		frameBufferBlueSRV.Sample(Nearest, input.texCoord).r);
	return float4(pow(saturate(inputColor), 1.0/2.2), 1.0);
}


technique11 TestTech
{
	pass p0 { SetComputeShader(CompileShader(cs_5_0, InitCS())); }
	pass p1 { SetComputeShader(CompileShader(cs_5_0, UpdateCS())); }
	pass p2 { SetComputeShader(CompileShader(cs_5_0, DrawParticlesCS())); }
	
	pass p3
	{
		SetVertexShader(CompileShader(vs_5_0, QuadVS()));
		SetPixelShader(CompileShader(ps_5_0, CopyToFramePS()));
		SetDepthStencilState(noDepthTest, 0);
		SetRasterizerState(noCulling);
		SetBlendState(disableBlend, float4(0.0, 0.0, 0.0, 0.0), 0xffffffff);
	}
}
