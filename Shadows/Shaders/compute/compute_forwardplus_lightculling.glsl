#version 430 core

#define TILE_SIZE 32

layout(local_size_x = TILE_SIZE, local_size_y = TILE_SIZE, local_size_z = 1) in;

struct PointLight 
{
	vec3 Pos;
	float Radius;
	vec4 Color;
};

struct VisibleIndex 
{
	int index;
};

layout(std430, binding = 0) buffer LightBuffer
{
	PointLight LightData[];
} lightBuffer;

layout(std430, binding = 1) buffer VisibleLightIndicesBuffer 
{
	VisibleIndex data[];
} visibleLightIndicesBuffer;

uniform mat4 View;
uniform mat4 ProjectionView;
uniform mat4 InverseProjection;
uniform int LightCount;
uniform vec2 ScreenSize;
uniform sampler2D tex_object;		// depthmap

vec3 MulViewProj(vec3 worldPos)
{
	vec4 result = ProjectionView * vec4(worldPos, 1.0);
	result /= result.w;
	return result.xyz;
}

vec3 MulView(vec3 worldPos)
{
	vec4 result = View * vec4(worldPos, 1.0);
	result /= result.w;
	return result.xyz;
}

vec3 ProjToView(float x, float y, float z)
{
	float X = (2.0f * x) - 1.0f;
	float Y = (2.0f * y) - 1.0f;
	float Z = (2.0f * z) - 1.0f;
	vec4 result = InverseProjection * vec4(X, Y, Z, 1);
	result /= result.w;
	return result.xyz;
};

vec4 CreatePlaneEquation(vec3 a, vec3 b, vec3 c)
{
	vec3 normal = normalize(cross(b - a, c - a));
	float d = -dot(normal, a);
	return vec4(normal, d);
}

float DistanceBetweenPlanePoint(vec4 plane, vec3 pos, float radius)
{
	return dot(plane.xyz, pos) + plane.w - radius;
}

shared uint minDepth;
shared uint maxDepth;
shared uint numOfVisibleLight;
shared vec4 frustumPlanes[6];
shared int visibleLightIndices[1024];
shared float tileMinDepth;
shared float tileMaxDepth;

void main(void)
{
	ivec2 pixelLocation = ivec2(gl_GlobalInvocationID.xy);
	ivec2 tileID = ivec2(gl_WorkGroupID.xy);
	ivec2 numOfTiles = ivec2(gl_NumWorkGroups.xy);
	uint tileIndex = tileID.y * numOfTiles.x + tileID.x;

	if (gl_LocalInvocationIndex == 0)
	{
		minDepth = 0xFFFFFFFF;
		maxDepth = 0;
		numOfVisibleLight = 0;
	}
	
	barrier();

	float localMaxDepth, localMinDepth;
	vec2 pixelLocationInUV = vec2(pixelLocation) / ScreenSize;
	float depth = texture(tex_object, pixelLocationInUV).r;

	uint depthInt = floatBitsToUint(depth);
	atomicMin(minDepth, depthInt);
	atomicMax(maxDepth, depthInt);

	barrier();

	vec3 v[4];
	if (gl_LocalInvocationIndex == 0)
	{
		tileMinDepth = uintBitsToFloat(minDepth);
		tileMaxDepth = uintBitsToFloat(maxDepth);
		tileMinDepth = -ProjToView(0.0, 0.0, tileMinDepth).z;
		tileMaxDepth = -ProjToView(0.0, 0.0, tileMaxDepth).z;

		vec2 temp = vec2(gl_NumWorkGroups);
		v[0] = ProjToView(tileID.x / temp.x,			tileID.y / temp.y,			1.0f);
		v[1] = ProjToView((tileID.x + 1) / temp.x,		tileID.y / temp.y,			1.0f);
		v[2] = ProjToView((tileID.x + 1) / temp.x,		(tileID.y + 1) / temp.y,	1.0f);
		v[3] = ProjToView(tileID.x / temp.x,			(tileID.y + 1) / temp.y,	1.0f);

		vec3 o = vec3(0.0);
		for(int i=0;i<4;++i)
			frustumPlanes[i] = CreatePlaneEquation(o, v[i], v[(i + 1) & 3]);
	}

	barrier();

	uint threadCount = TILE_SIZE * TILE_SIZE;
	uint passCount = (LightCount + threadCount - 1) / threadCount;
	for (uint i = 0; i < passCount; i++)
	{
		uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
		if (lightIndex >= LightCount) {
			break;
		}

		//if (lightIndex != 8)
		//	continue;

		vec3 LPos = lightBuffer.LightData[lightIndex].Pos;
		LPos = MulView(LPos);
		
		float LRadius = lightBuffer.LightData[lightIndex].Radius;
		float a0 = DistanceBetweenPlanePoint(frustumPlanes[0], LPos, LRadius);	// bottom
		float a1 = DistanceBetweenPlanePoint(frustumPlanes[1], LPos, LRadius);	// right
		float a2 = DistanceBetweenPlanePoint(frustumPlanes[2], LPos, LRadius);	// top
		float a3 = DistanceBetweenPlanePoint(frustumPlanes[3], LPos, LRadius);	// left

		float tempZ = -LPos.z;
		bool isInsideZ = (((tileMinDepth - LRadius) < tempZ) && (tempZ < (tileMaxDepth + LRadius)));

		bool isInside = (a0 < 0 && a1 < 0 && a2 < 0 && a3 < 0);
		if (isInside && isInsideZ)
		{
			uint offset = atomicAdd(numOfVisibleLight, 1);
			visibleLightIndices[offset] = int(lightIndex);

			//if (tileIndex == 0)
			//	lightBuffer.LightData[lightIndex].Color.xyz = vec3(1.0, 0.0, 0.0);
		}
	}

	barrier();

	if (gl_LocalInvocationIndex == 0)
	{
		uint offset = tileIndex * 1024;	// global buffer index start position
		for (int i = 0; i < numOfVisibleLight; ++i)
		{
			visibleLightIndicesBuffer.data[offset + i].index = visibleLightIndices[i];
		}

		if (numOfVisibleLight != 1024)
			visibleLightIndicesBuffer.data[offset + numOfVisibleLight].index = -1;
	}

	//vec4 texel;
	//ivec2 p = ivec2(gl_GlobalInvocationID.xy);
	///*
	// ivec2 tileID = ivec2(gl_WorkGroupID.xy);
	// v[0] = projToView(tileID.x, tileID.y, 1.0f);
	// v[1] = projToView((tileID.x + 1), tileID.y, 1.0f);
	// v[2] = projToView((tileID.x + 1), (tileID.y + 1), 1.0f);
	// v[3] = projToView(tileID.x, (tileID.y + 1), 1.0f);
	//*/
	//float m = 1.0 / TILE_SIZE;
	////for (int i = 0; i < LightCount; ++i)
	//if (tileIndex == 0 && gl_LocalInvocationIndex == 0)
	//{
	//	int i = 8;
	//	{
	//		lightBuffer.LightData[i].Pos = MulView(lightBuffer.LightData[i].Pos);
	//		lightBuffer.LightData[i].Pos.xyz = lightBuffer.LightData[i].Pos.xyz * 0.5 + 0.5;

	//		if (i == 8)
	//		{
	//			//lightBuffer.LightData[i].Radius = 0.0;
	//			//lightBuffer.LightData[i].Color = vec4(tileID.x / float(TILE_SIZE), tileID.y / float(TILE_SIZE), 0.5, 1.0);
	//			//if (tileIndex == 0)
	//			{
	//				float a0 = DistanceBetweenPlanePoint(frustumPlanes[0], lightBuffer.LightData[i].Pos.xyz, lightBuffer.LightData[i].Radius);	// bottom
	//				float a1 = DistanceBetweenPlanePoint(frustumPlanes[1], lightBuffer.LightData[i].Pos.xyz, lightBuffer.LightData[i].Radius);	// right
	//				float a2 = DistanceBetweenPlanePoint(frustumPlanes[2], lightBuffer.LightData[i].Pos.xyz, lightBuffer.LightData[i].Radius);	// top
	//				float a3 = DistanceBetweenPlanePoint(frustumPlanes[3], lightBuffer.LightData[i].Pos.xyz, lightBuffer.LightData[i].Radius);	// left

	//				bool isInside = (a0 < 0 && a1 < 0 && a2 < 0 && a3 < 0);

	//				vec2 temp = vec2(gl_NumWorkGroups);

	//				lightBuffer.LightData[i].Pos.x = (tileID.x);
	//				lightBuffer.LightData[i].Pos.y = (gl_NumWorkGroups.x);
	//				lightBuffer.LightData[i].Pos.z = tileIndex;
	//				if (isInside)
	//					lightBuffer.LightData[i].Color.xyz = vec3(1.0, 0.0, 0.0);
	//				else
	//					lightBuffer.LightData[i].Color = vec4(vec3(0.5), 1.0);
	//			}
	//		}
	//		else
	//		{
	//			lightBuffer.LightData[i].Color = vec4(0.0, 0.0, 0.0, 1.0);
	//		}

	//		//vec2 TileID = vec2(floor(lightBuffer.LightData[i].Pos.x * TILE_SIZE), floor(lightBuffer.LightData[i].Pos.y * TILE_SIZE));
	//	}
	//}
}