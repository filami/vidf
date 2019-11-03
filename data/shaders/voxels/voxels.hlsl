

static const uint brickShift = 3;
static const uint mapLevels = 5;
static const uint brickSize = 1 << brickShift;
static const uint brickMask = brickSize - 1;
static const uint mapSize = 1 << (mapLevels * brickShift);



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
	} view;
};



struct BrickInstance
{
	int3 offset;
	int3 brickId;
};



Texture3D<uint> sparceBricksSRV : register(t0);
Texture3D<uint> voxelBricksSRV  : register(t1);
StructuredBuffer<BrickInstance> brickInstancesSRV : register(t2);



struct Output
{
	float4 hPosition : SV_Position;
	float4 color     : TEXCOORD0;
	float3 wPosition : TEXCOORD1;
};



Output vsVoxels(uint vertexId : SV_VertexId, uint instanceId : SV_InstanceId)
{
	const uint indices[] =
	{
		0, 1, 3, 0, 3, 2,
		0, 2, 6, 0, 6, 4,
		0, 4, 5, 0, 5, 1,
		7, 5, 4, 7, 4, 6,
		7, 3, 1, 7, 1, 5,
		7, 6, 3, 2, 3, 6,
	};
	const float3 vertices[] = 
	{	
		float3(0, 0, 0), // 0
		float3(1, 0, 0), // 1
		float3(0, 1, 0), // 2
		float3(1, 1, 0), // 3
		
		float3(0, 0, 1), // 4
		float3(1, 0, 1), // 5
		float3(0, 1, 1), // 6
		float3(1, 1, 1), // 7
	};
	const float3 normals[] = 
	{	
		float3(-1,  0,  0),
		float3( 1,  0,  0),
		float3( 0, -1,  0),
		float3( 0,  1,  0),		
		float3( 0,  0, -1),
		float3( 0,  0,  1),
	};
	
	const uint3 brickIdx = brickInstancesSRV[instanceId].brickId;
	const uint3 offset = brickInstancesSRV[instanceId].offset;
	const uint cubeId = vertexId / 36;
	const uint cubeVertexId = vertexId % 36;
	const uint cubeFaceId = cubeVertexId / 6;
	const uint3 coord = uint3(cubeId & brickMask, (cubeId >> brickShift) & brickMask, (cubeId >> (brickShift * 2)) & brickMask);
	
	const uint a = voxelBricksSRV[brickIdx * brickSize + coord];
	
	const float3 cubeVertex = offset + vertices[indices[cubeVertexId]] * a;
	const float3 vertex = cubeVertex + coord;
	
	Output output;
	output.hPosition = mul(view.projViewTM, float4(vertex, 1.0));
	output.color = float4(normals[cubeFaceId], 1) * 0.5 + 0.5;
	output.wPosition = vertex;
	
	return output;
}




float4 psVoxels(Output input) : SV_Target
{
	const float dist = distance(view.viewPosition, input.wPosition);
	const float fog = exp(-dist / 128);
	return float4(lerp(1, input.color.rgb, fog), 1.0);
}
