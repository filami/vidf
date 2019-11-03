


static const uint brickShift = 3;
static const uint mapLevels = 5;
static const uint brickSize = 1 << brickShift;
static const uint brickMask = brickSize - 1;
static const uint mapSize = 1 << (mapLevels * brickShift);

static const uint maxSize_ = 2048;
static const uint maxSizeMask_ = maxSize_ - 1;
static const uint maxSize = 2048 >> brickShift;
static const uint maxSizeMask = maxSize - 1;
static const uint maxSizeShift = 8;

static const uint numTAASamples = 8;


static const float PI = acos(-1);



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



cbuffer viewCB : register(b0)
{
	struct
	{
		float4x4 projTM;
		float4x4 viewTM;
		float4x4 projViewTM;
		float4x4 invProjViewTM;
		float2 viewportSize;
		float2 invViewportSize;
		float3 viewPosition;
		uint   frameId;
	} view;
};




Texture3D<uint> sparceBricksSRV      : register(t0);
Texture3D<uint> voxelBricksSRV       : register(t1);
Texture2D       quarterResSRV        : register(t3);
Texture2D       normalsSRV           : register(t3);
Texture2D       quarterResSamplesSRV : register(t4);
SamplerState    diffuseSS            : register(s0);


struct VSOutput
{
	float4 hPosition : SV_Position;
	float2 tc0 : TEXCOORD0;
	float2 tc1 : TEXCOORD1;
};



VSOutput vsVoxelRaytrace(uint vertexId : SV_VertexId)
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
	output.tc1 = vertices[vertexId] * float2(0.5, -0.5) + 0.5;
	return output;
}



struct Fragment
{
	float3 color;
	float3 normal;
	float  distance;
	int    steps;
	bool   isect;
};



void RaytraceBrick(inout Fragment fragment, in float3 rayOrig, in float3 rayDir, in int3 offset, in int idx)
{
	const uint3 brickIdx = uint3(idx & maxSizeMask, (idx >> maxSizeShift) & maxSizeMask, (idx >> (maxSizeShift * 2)) & maxSizeMask);
	
	for (uint j = 0; j < brickSize * brickSize * brickSize; ++j)
	{
		fragment.steps++;
		
		const int3 coord = uint3(j & brickMask, (j >> brickShift) & brickMask, (j >> (brickShift * 2)) & brickMask);
		const float2 it = BoxRayIntersect(rayOrig - offset, rayDir, coord, coord + 1);
		if (it.y < 0 || it.x > it.y)
			continue;
		if (it.x > fragment.distance)
			continue;
		
		const int3 voxelId = brickIdx * brickSize + coord;
		if (voxelBricksSRV[voxelId] != 1)
			continue;
		
		fragment.distance = it.x;
		fragment.isect = true;
	}
}



void RaytraceRoot(inout Fragment fragment, in float3 rayOrig, in float3 rayDir)
{	
	int stack = 0;
	uint istack[mapLevels - 1];
	uint jstack[mapLevels - 1];
	uint3 offsetStack[mapLevels - 1];
	uint brickSizeStack[mapLevels - 1];
	istack[stack] = 0;
	jstack[stack] = 0;
	offsetStack[stack] = 0;
	brickSizeStack[stack] = brickSize * brickSize * brickSize * brickSize;
	
	int limit = 0;
	bool failed = false;
	
	while (stack >= 0)
	{
		if (limit++ > 16)
		{
			failed = true;
			break;
		}
		
		uint i = istack[stack];
		const uint3 brickIdx = uint3(i & maxSizeMask, (i >> maxSizeShift) & maxSizeMask, (i >> (maxSizeShift * 2)) & maxSizeMask);
		
		bool pushed = false;
		for (uint j = jstack[stack]; j < brickSize * brickSize * brickSize; ++j)
		{
			fragment.steps++;
			
			const int3 coord = uint3(j & brickMask, (j >> brickShift) & brickMask, (j >> (brickShift * 2)) & brickMask);
			const float2 it = BoxRayIntersect(rayOrig - offsetStack[stack], rayDir, coord * brickSizeStack[stack], coord * brickSizeStack[stack] + brickSizeStack[stack]);
			if (it.y < 0 || it.x > it.y)
				continue;
			if (it.x > fragment.distance)
				continue;
			
			const int3 voxelId = brickIdx * brickSize + coord;
			const int idx = sparceBricksSRV[voxelId];
			if (idx == -1)
				continue;
			
			if (stack < int(mapLevels - 2))
			{
				jstack[stack] = j + 1;
				stack++;
				istack[stack] = idx;
				jstack[stack] = 0;
				offsetStack[stack] = offsetStack[stack - 1] + coord * brickSizeStack[stack - 1];
				brickSizeStack[stack] = brickSizeStack[stack - 1] >> brickShift;
				pushed = true;
				break;
			}
			else
			{
				RaytraceBrick(fragment, rayOrig, rayDir, offsetStack[stack] + (coord << brickShift), idx);
			}
		}
		if (!pushed)
			--stack;
	}
}



void RaytraceRootV2(inout Fragment fragment, in float3 rayOrig, in float3 rayDir)
{
	const float startSparceBrickSize = brickSize * brickSize * brickSize * brickSize;
	
	const float2 it = BoxRayIntersect(rayOrig, rayDir, 0, startSparceBrickSize * brickSize);
	if (it.y < 0 || it.x > it.y)
		return;
	if (it.x > fragment.distance)
		return;
	
	float rayT = max(0, it.x);
	float3 gridIntersect = rayOrig + rayDir * rayT;
	
	struct StackLevel
	{
		int3   brickIdx;
		int3   Pos;
		float3 NextCrossingT;
		float3 DeltaT;
		float3 Offset;
		float  sparceBrickSize;
	};
	StackLevel stack[mapLevels];
	
	stack[0].brickIdx = 0;
	stack[0].Pos = min(int3(gridIntersect) / startSparceBrickSize, brickSize - 1);	
	stack[0].NextCrossingT = rayT + ((stack[0].Pos + (sign(rayDir)*0.5+0.5)) * startSparceBrickSize - gridIntersect) / rayDir;
	stack[0].DeltaT = startSparceBrickSize / abs(rayDir);
	stack[0].Offset = 0;
	stack[0].sparceBrickSize = startSparceBrickSize;
	
	int3 Step = sign(rayDir);
	
	int stackLv = 0;
	
	[loop]
	for (uint j = 0; j < 256; ++j)
	{
		if (stack[stackLv].Pos.x < 0 || stack[stackLv].Pos.y < 0 || stack[stackLv].Pos.z < 0 ||
			stack[stackLv].Pos.x >= int(brickSize) || stack[stackLv].Pos.y >= int(brickSize) || stack[stackLv].Pos.z >= int(brickSize))
		{
			--stackLv;
			continue;
		}
		
		if (stackLv < 0)
			break;
		
		fragment.steps++;
		
		StackLevel newLevel = stack[stackLv];
		bool push = false;
		
		const int3 voxelId = stack[stackLv].brickIdx * brickSize + stack[stackLv].Pos;
		if (stackLv == 4)
		{
			uint value = voxelBricksSRV[voxelId];
			if (value == 1)
			{
				const float3 minBox = stack[stackLv].Offset + stack[stackLv].Pos * stack[stackLv].sparceBrickSize;
				const float3 maxBox = minBox + stack[stackLv].sparceBrickSize;
				const float2 it = BoxRayIntersect(rayOrig, rayDir, minBox, maxBox);
				
				const float3 normal = (rayOrig + rayDir * it.x) - (maxBox + minBox) * 0.5;
				if (abs(normal.x) > abs(normal.y) && abs(normal.x) > abs(normal.z))
					fragment.normal = float3(sign(normal.x), 0, 0);
				else if (abs(normal.y) > abs(normal.z))
					fragment.normal = float3(0, sign(normal.y), 0);
				else
					fragment.normal = float3(0, 0, sign(normal.z));
				fragment.distance = it.x;
				fragment.color = fragment.normal * 0.5 + 0.5;
				fragment.isect = true;
				break;
			}
		}
		else
		{
			const int idx = sparceBricksSRV[voxelId];
			if (idx != -1)
			{
				const float3 minBox = stack[stackLv].Offset + stack[stackLv].Pos * stack[stackLv].sparceBrickSize;
				const float3 maxBox = minBox + stack[stackLv].sparceBrickSize;
				const float2 it = BoxRayIntersect(rayOrig, rayDir, minBox, maxBox);
			
				float rayT = max(0, it.x);
				float3 gridIntersect = rayOrig + rayDir * rayT;
				
				int3 Pos = (gridIntersect - minBox) / (maxBox - minBox) * brickSize;
				Pos = clamp(Pos, 0, brickSize - 1);
				const float3 Offset = minBox;
				
				newLevel.brickIdx = uint3(idx & maxSizeMask, (idx >> maxSizeShift) & maxSizeMask, (idx >> (maxSizeShift * 2)) & maxSizeMask);
				newLevel.Pos = Pos;	
				newLevel.NextCrossingT = rayT + ((minBox + (Pos + (sign(rayDir)*0.5+0.5)) * stack[stackLv].sparceBrickSize / brickSize) - gridIntersect) / rayDir;
				newLevel.DeltaT = stack[stackLv].sparceBrickSize / abs(rayDir) / brickSize;
				newLevel.Offset = Offset;
				newLevel.sparceBrickSize = stack[stackLv].sparceBrickSize / brickSize;
				push = true;
			}
		}
		
		const int bits =
			((stack[stackLv].NextCrossingT.x < stack[stackLv].NextCrossingT.y) << 2) +
			((stack[stackLv].NextCrossingT.x < stack[stackLv].NextCrossingT.z) << 1) +
			((stack[stackLv].NextCrossingT.y < stack[stackLv].NextCrossingT.z));
		const int3 cmptoAxis[8] =
		{
			int3(0, 0, 1),
			int3(0, 1, 0),
			int3(0, 0, 1),
			int3(0, 1, 0),
			int3(0, 0, 1),
			int3(0, 0, 1),
			int3(1, 0, 0),
			int3(1, 0, 0),
		};
		const int3 axis = cmptoAxis[bits];
		stack[stackLv].Pos += Step * axis;
		stack[stackLv].NextCrossingT += stack[stackLv].DeltaT * axis;
		
		if (push)
		{
			stackLv++;
			stack[stackLv] = newLevel;
		}
	}
}



static const float3 lightDir = normalize(float3(2, 10, 3));



float4 psVoxelRaytrace(VSOutput input) : SV_Target
{
//	return quarterResSRV[input.hPosition.xy / 4];
	
	const float3 rayOrig = view.viewPosition;
	const float4 m = mul(view.invProjViewTM, float4(input.tc0, 1, 1));
	const float3 rayDir = normalize(m.xyz / m.w - rayOrig);
	
	Fragment fragment;
	fragment.color = 0;
	fragment.normal = 0;
	fragment.steps = 0;
	fragment.distance = 1024 * 1024;
	fragment.isect = false;
	
	/*/
	RaytraceRoot(fragment, rayOrig, rayDir);
	
	return fragment.steps / 1024.0 / 64.0;
	
	if (!fragment.isect)
		discard;
	
	return 1 - exp(-fragment.distance / 128);
	/**/
	
	/**/
	RaytraceRootV2(fragment, rayOrig, rayDir);
//	return fragment.steps / 256.0;	
	if (!fragment.isect)
		discard;
	
	const float3 position = rayOrig + rayDir * fragment.distance;
	
	Fragment shadowFragment;
	shadowFragment.color = 0;
	shadowFragment.normal = 0;
	shadowFragment.steps = 0;
	shadowFragment.distance = 1024 * 1024;
	shadowFragment.isect = false;
	RaytraceRootV2(shadowFragment, position + fragment.normal / 32.0, lightDir);
	
	const float ndotl = max(0, dot(fragment.normal, lightDir));
	const float4 quarter = quarterResSRV.Sample(diffuseSS, input.tc1);
	
	const float3 color = ndotl * quarter.a * 4.5 + quarter.rgb * 1.5;
	
	// return float4(ndotl + quarter.rgb, 1.0);	
	return float4(1 - exp(-color), 1.0);	
	// return float4(ndotl.xxx * (1 - shadowFragment.isect) * 0.5 + quarterResSRV.Sample(diffuseSS, input.tc1).r * 0.5, 1.0);	
	// return float4(ndotl.xxx * (1 - shadowFragment.isect) * 0.5 + quarterResSRV[input.hPosition.xy / 4].rgb * 0.5, 1);
//	return fragment.steps / 64.0;
	/**/
}



struct PSOutput
{
	float4 sample : SV_Target0;
	float4 normal : SV_Target1;
};



PSOutput psQuarterResVoxelRaytrace(VSOutput input)
{
	PSOutput output;
	output.sample = 0;
	
	const float3 rayOrig = view.viewPosition;
	const float4 m = mul(view.invProjViewTM, float4(input.tc0, 1, 1));
	const float3 rayDir = normalize(m.xyz / m.w - rayOrig);
	
	Fragment fragment;
	fragment.color = 0;
	fragment.normal = 0;
	fragment.steps = 0;
	fragment.distance = 1024 * 1024;
	fragment.isect = false;
	
	RaytraceRootV2(fragment, rayOrig, rayDir);
	
	uint rngState = WangHash(uint(view.frameId % numTAASamples)*156*99 + uint(input.hPosition.x)*156 + uint(input.hPosition.y));
	
	NormalBasis normalBasis = NormalBasisFromNormal(fragment.normal);
	output.normal = float4(fragment.normal, fragment.distance);
	
	const float3 position = rayOrig + rayDir * fragment.distance + fragment.normal / 1024.0;
	
	float accum = 0;
	{			
		const float3 dir = Transform(normalBasis, SampleCosineHemisphere(RandUFloat2(rngState)));
					
		Fragment shadowFragment;
		shadowFragment.color = 0;
		shadowFragment.normal = 0;
		shadowFragment.steps = 0;
		shadowFragment.distance = 1024 * 1024;
		shadowFragment.isect = false;
		RaytraceRootV2(shadowFragment, position, dir);
		
		const float shadow = (1 - shadowFragment.isect);
		const float3 light = float3(0.8, 0.95, 1.0);
		output.sample.rgb += light * shadow;
	}
	
	float shadow = 1;
	if (fragment.isect)
	{
		Fragment shadowFragment;
		shadowFragment.color = 0;
		shadowFragment.normal = 0;
		shadowFragment.steps = 0;
		shadowFragment.distance = 1024 * 1024;
		shadowFragment.isect = false;
		RaytraceRootV2(shadowFragment, position, lightDir);
		
		const float shadow = (1 - shadowFragment.isect);
		const float3 light = max(dot(fragment.normal, lightDir), 0) * float3(1.0, 0.95, 0.8);
		output.sample.rgb += light * shadow;
	}
	
	return output;
}



float4 psAccumQuarterRes(VSOutput input) : SV_Target
{
	float3 accum = 0;
	float count = 0;
	
	const float3 thisNormal = normalsSRV[input.hPosition.xy].xyz;
	const float thisDist = normalsSRV[input.hPosition.xy].a;
	accum.rgb += quarterResSamplesSRV[input.hPosition.xy].rgb;
	count += 1;
	
	for (int y = -5; y <= 5; ++y)
	{
		for (int x = -5; x <= 5; ++x)
		{
			int2 coord = input.hPosition.xy + int2(x, y);
			const float3 thatNormal = normalsSRV[coord].xyz;
			const float thatDist = normalsSRV[coord].a;
		
			float weigth = 1.0;
			weigth *= max(dot(thisNormal, thatNormal), 0);
			weigth *= max(1 - abs(thatDist - thisDist) * 0.5, 0);
			
			accum.rgb += quarterResSamplesSRV[coord].rgb * weigth;
			count += weigth;
		}
	}
	
//	return float4(count.xxx / 200, 0);
	return float4(accum / count, 0);
}
