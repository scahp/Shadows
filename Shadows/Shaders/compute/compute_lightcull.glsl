#version 430 core

// Uniforms
uniform mat4 P;
vec4 screenSize;

// Shared values between all the threads in the group
shared uint MinDepth;
shared uint MaxDepth;
shared vec4 FrustumPlane[6];

//// Shared local storage for visible indices, will be written out to the global buffer at the end
//shared int visibleLightIndices[1024];
//shared mat4 viewProjection;

vec3 projToView(float x, float y, float z)
{
	float X = (2.0f * x) / tileNumber - 1.0f;
	float Y = (2.0f * y) / tileNumber - 1.0f;
	return Projection.GetInverse().Transform(Vector(X, Y, z));
};

vec4 CreateFrustumFromThreePoints(const vec3& p0, const vec3& p1, const vec3& p2)
{
	const auto direction0 = p1 - p0;
	const auto direction1 = p2 - p0;
	const auto normal = normalize(cross(direction0, direction1));
	const auto distance = dot(p2, normal);
	return vec4(normal, distance);
}

#define TILE_SIZE 16
layout (local_size_x = TILE_SIZE, local_size_y = TILE_SIZE) in;

void main(void)
{
	ivec2 tileID = ivec2(gl_WorkGroupID.xy);
	ivec2 tileNumber = ivec2(gl_NumWorkGroups.xy);
	ivec2 location = ivec2(gl_GlobalInvocationID.xy);

	if (0 == gl_LocalInvocationIndex) 
	{
		minDepthInt = 0xFFFFFFFF;
		maxDepthInt = 0;
	}

	float maxDepth, minDepth;
	vec2 DepthUV = vec2(location) / screenSize;
	float depth = texture(depthMap, DepthUV).r;

	uint depthInt = floatBitsToUint(depth);
	atomicMin(minDepth, depthInt);
	atomicMax(maxDepth, depthInt);

	barrier();

	if (0 == gl_LocalInvocationIndex) 
	{
		minDepth = uintBitsToFloat(minDepthInt);
		maxDepth = uintBitsToFloat(maxDepthInt);

		vec3 v[4];
		v[0] = projToView(tileID.x, tileID.y, 1.0);
		v[1] = projToView(tileID.x + 1, tileID.y, 1.0);
		v[2] = projToView(tileID.x + 1, tileID.y + 1, 1.0);
		v[3] = projToView(tileID.x, tileID.y + 1, 1.0);

		for (int i = 0; i < 4; i++)
			frustum[i] = CreateFrustumFromThreePoints(o, v[i], v[(i + 1) & 3]);
	}

	barrier();

	//// Step 3: Cull lights.
	//// Parallelize the threads against the lights now.
	//// Can handle 256 simultaniously. Anymore lights than that and additional passes are performed
	//uint threadCount = TILE_SIZE * TILE_SIZE;
	//uint passCount = (lightCount + threadCount - 1) / threadCount;
	//for (uint i = 0; i < passCount; i++) {
	//	// Get the lightIndex to test for this thread / pass. If the index is >= light count, then this thread can stop testing lights
	//	uint lightIndex = i * threadCount + gl_LocalInvocationIndex;
	//	if (lightIndex >= lightCount) {
	//		break;
	//	}

	//	vec4 position = lightBuffer.data[lightIndex].position;
	//	float radius = lightBuffer.data[lightIndex].paddingAndRadius.w;

	//	// We check if the light exists in our frustum
	//	float distance = 0.0;
	//	for (uint j = 0; j < 6; j++) {
	//		distance = dot(position, frustumPlanes[j]) + radius;

	//		// If one of the tests fails, then there is no intersection
	//		if (distance <= 0.0) {
	//			break;
	//		}
	//	}

	//	// If greater than zero, then it is a visible light
	//	if (distance > 0.0) {
	//		// Add index to the shared array of visible indices
	//		uint offset = atomicAdd(visibleLightCount, 1);
	//		visibleLightIndices[offset] = int(lightIndex);
	//	}
	//}

	barrier();

	//// One thread should fill the global light buffer
	//if (gl_LocalInvocationIndex == 0) {
	//	uint offset = index * 1024; // Determine bosition in global buffer
	//	for (uint i = 0; i < visibleLightCount; i++) {
	//		visibleLightIndicesBuffer.data[offset + i].index = visibleLightIndices[i];
	//	}

	//	if (visibleLightCount != 1024) {
	//		// Unless we have totally filled the entire array, mark it's end with -1
	//		// Final shader step will use this to determine where to stop (without having to pass the light count)
	//		visibleLightIndicesBuffer.data[offset + visibleLightCount].index = -1;
	//	}
	//}
}
