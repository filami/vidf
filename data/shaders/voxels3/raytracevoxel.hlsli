#ifndef VOXELRAYTRACE_H
#define VOXELRAYTRACE_H

#include "raytracecommon.hlsli"


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



struct Fragment
{
	uint   index;
	float3 normal;
	float  distance;
	int    steps;
	bool   isect;
};



void RayPlaneIntersect(inout Fragment fragment, in Ray ray, in float4 plane)
{
	float3 origin = ray.orig - plane.xyz * plane.w;
	float a = dot(plane.xyz, ray.dir);
	if (a == 0)
		return;
	float b = -dot(plane.xyz, origin) / a;
	if (b < 0)
		return;
	fragment.index = 0;
	fragment.normal = plane.xyz;
	fragment.distance = b;
	fragment.steps = 1;
	fragment.isect = true;
}



void RaytraceRootV2(inout Fragment fragment, in Texture3D<uint> sparceBricksSRV, in Texture3D<uint> voxelBricksSRV, in Ray ray)
{
	const float startSparceBrickSize = brickSize * brickSize * brickSize * brickSize;
	
	RayPlaneIntersect(fragment, ray, float4(0, 0, 1, 0));
	
	const float2 it = BoxRayIntersect(ray.orig, ray.dir, 0, startSparceBrickSize * brickSize);
	if (it.y < 0 || it.x > it.y)
		return;
	if (it.x > fragment.distance)
		return;
	
	float rayT = max(0, it.x);
	float3 gridIntersect = ray.orig + ray.dir * rayT;
	
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
	stack[0].NextCrossingT = rayT + ((stack[0].Pos + (sign(ray.dir)*0.5+0.5)) * startSparceBrickSize - gridIntersect) / ray.dir;
	stack[0].DeltaT = startSparceBrickSize / abs(ray.dir);
	stack[0].Offset = 0;
	stack[0].sparceBrickSize = startSparceBrickSize;
	
	int3 Step = sign(ray.dir);
	
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
			if (value != 0)
			{
				const float3 minBox = stack[stackLv].Offset + stack[stackLv].Pos * stack[stackLv].sparceBrickSize;
				const float3 maxBox = minBox + stack[stackLv].sparceBrickSize;
				const float2 it = BoxRayIntersect(ray.orig, ray.dir, minBox, maxBox);
				
				const float3 normal = (ray.orig + ray.dir * it.x) - (maxBox + minBox) * 0.5;
				if (abs(normal.x) > abs(normal.y) && abs(normal.x) > abs(normal.z))
					fragment.normal = float3(sign(normal.x), 0, 0);
				else if (abs(normal.y) > abs(normal.z))
					fragment.normal = float3(0, sign(normal.y), 0);
				else
					fragment.normal = float3(0, 0, sign(normal.z));
				fragment.distance = it.x;
				fragment.index = value;
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
				const float2 it = BoxRayIntersect(ray.orig, ray.dir, minBox, maxBox);
			
				float rayT = max(0, it.x);
				float3 gridIntersect = ray.orig + ray.dir * rayT;
				
				int3 Pos = (gridIntersect - minBox) / (maxBox - minBox) * brickSize;
				Pos = clamp(Pos, 0, brickSize - 1);
				const float3 Offset = minBox;
				
				newLevel.brickIdx = uint3(idx & maxSizeMask, (idx >> maxSizeShift) & maxSizeMask, (idx >> (maxSizeShift * 2)) & maxSizeMask);
				newLevel.Pos = Pos;	
				newLevel.NextCrossingT = rayT + ((minBox + (Pos + (sign(ray.dir)*0.5+0.5)) * stack[stackLv].sparceBrickSize / brickSize) - gridIntersect) / ray.dir;
				newLevel.DeltaT = stack[stackLv].sparceBrickSize / abs(ray.dir) / brickSize;
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


#endif
