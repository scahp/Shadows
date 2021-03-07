#version 330 core
#preprocessor

precision mediump float;

uniform sampler2D SceneColorSampler;
uniform sampler2D NormalSampler;
uniform sampler2D DepthSampler;
uniform sampler2D HiZSampler;

uniform mat4 V;
uniform mat4 P;
uniform mat4 InvP;
uniform vec2 ScreenSize;
uniform float Near;
uniform float Far;
uniform int DebugReflectionOnly;

#if USE_HIZ
uniform int MaxMipLevel;
#endif // USE_HIZ

in vec2 TexCoord_;
out vec4 color;

#define MAX_ITERATION 3000
#define MAX_THICKNESS 0.001

float GetDepthSample(vec2 uv)
{
	return texture(DepthSampler, uv).x;
}

vec4 GetViewSpaceNormal(vec2 uv)
{
	vec4 normal = texture(NormalSampler, uv);
	vec4 result = (V * vec4(normal.xyz, 0.0));
	result.w = normal.w;
	return result;
}

void ComputePosAndReflection(vec3 normalInVS, out vec3 outSamplePosInTS, out vec3 outReflDirInTS, out float outMaxDistance)
{
	float sampleDepth = GetDepthSample(TexCoord_);

	// OpenGL uv is same direction with clip space, so We don't need to reverse Y for converting from uv to clip space coordinate.
	vec4 samplePosInCS = vec4((TexCoord_ + vec2(0.5) / ScreenSize) * 2.0 - 1.0, sampleDepth * 2.0 - 1.0, 1.0);

	vec4 samplePosInVS = InvP * samplePosInCS;
	samplePosInVS /= vec4(samplePosInVS.w);

	vec3 CameraToSampleInVS = normalize(samplePosInVS.xyz);
	vec4 ReflectionInVS = vec4(reflect(CameraToSampleInVS.xyz, normalInVS.xyz), 0.0);

	vec4 ReflectionEndPosInVS = samplePosInVS + ReflectionInVS * 1000.0;

	// OpenGL Viewspace Forward Z is [1 ~ -1], it is reversed direction compared to DX or Vulkan. so I replaced below code direction.
	//float temp = (ReflectionEndPosInVS.z < 0) ? ReflectionEndPosInVS.z : 1.0;
	float temp = (ReflectionEndPosInVS.z > 0) ? -ReflectionEndPosInVS.z : 1.0;
	ReflectionEndPosInVS /= vec4(temp);

	vec4 RelfectionEndPosInCS = P * vec4(ReflectionEndPosInVS.xyz, 1.0);
	RelfectionEndPosInCS /= vec4(RelfectionEndPosInCS.w);

	vec3 ReflectionDir = normalize((RelfectionEndPosInCS - samplePosInCS).xyz);

	// Transform to texture space
	samplePosInCS.xyz *= vec3(0.5, 0.5, 0.5);
	samplePosInCS.xyz += vec3(0.5, 0.5, 0.5);

	ReflectionDir.xyz *= vec3(0.5, 0.5, 0.5);

	outSamplePosInTS = samplePosInCS.xyz;
	outReflDirInTS = ReflectionDir;

	// Compute the maximum distance to trace that the ray stay inside of visible area.
	outMaxDistance = (outReflDirInTS.x >= 0.0) ? ((1.0 - outSamplePosInTS.x) / outReflDirInTS.x) : (-outSamplePosInTS.x / outReflDirInTS.x);
	outMaxDistance = min(outMaxDistance, (outReflDirInTS.y < 0) ? (-outSamplePosInTS.y / outReflDirInTS.y) : ((1.0 - outSamplePosInTS.y) / outReflDirInTS.y));
	outMaxDistance = min(outMaxDistance, (outReflDirInTS.z < 0) ? (-outSamplePosInTS.z / outReflDirInTS.z) : ((1.0 - outSamplePosInTS.z) / outReflDirInTS.z));
}

#if USE_HIZ

// Make texel(or Cell) count at which MipLevel
vec2 getHiZCellCount(int mipLevel)
{
	int level = 1 << mipLevel;
	vec2 result = ScreenSize / vec2(float(level));
	result = max(vec2(1.0), floor(result));
	return result;
}

// Make texel(or Cell) position from uv and texel count.
vec2 getCell(vec2 pos, vec2 cell_count)
{
	return vec2(floor(pos * cell_count));
}

vec3 intersectDepthPlane(vec3 origin, vec3 dest, float t)
{
	return origin + dest * t;
}

// Move to next texel(Cell) in current miplevel.
vec3 intersectCellBoundary(vec3 origin, vec3 dest, vec2 cell, vec2 cell_count, vec2 crossStep, vec2 crossOffset)
{
	vec3 intersection = vec3(0.0);

	vec2 index = cell + crossStep;
	vec2 boundary = index / cell_count;
	boundary += crossOffset;

	vec2 delta = boundary - origin.xy;
	delta /= dest.xy;
	float t = min(delta.x, delta.y);

	intersection = intersectDepthPlane(origin, dest, t);
	return intersection;
}

// Sampling HiZ with MipLevel
float getMinimumDepthPlane(vec2 p, int mipLevel)
{
	return textureLod(HiZSampler, p, mipLevel).x;		// with PointSampler
}

// Check weather Cell is changed
bool crossedCellBoundary(vec2 oldCellIndex, vec2 newCellIndex)
{
	return (oldCellIndex.x != newCellIndex.x) || (oldCellIndex.y != newCellIndex.y);
}

float FindIntersection_HiZ(vec3 samplePosInTS, vec3 ReflDirInTS, float maxTraceDistance, out vec3 outIntersection)
{
	vec2 crossStep = vec2(((ReflDirInTS.x >= 0) ? 1 : -1), ((ReflDirInTS.y >= 0) ? 1 : -1));
	vec2 crossOffset = crossStep / ScreenSize / vec2(128.0);		// '1 / vec2(128.0)' makes ray would be in next texel(or Cell) not on boundary. it is heuristics value.
	crossStep = clamp(crossStep, vec2(0.0, 0.0), vec2(1.0, 1.0));

	// Calculate distance of depth between origin and maxTracingDistance.
	vec3 ray = samplePosInTS;
	float minZ = ray.z;
	float maxZ = ray.z + ReflDirInTS.z * maxTraceDistance;
	float deltaZ = (maxZ - minZ);

	// Set origin and max destination of ray.
	vec3 origin = ray;
	vec3 dest = ReflDirInTS * maxTraceDistance;

	int startLevel = 2;
	int stopLevel = 0;
	vec2 startCellCount = getHiZCellCount(startLevel);			// Get start miplevel's texel count.
	vec2 rayCell = getCell(ray.xy, startCellCount);				// Get start position cell index.

	// Go to next texel(cell) to avoid self-intersection.
	// Power of 2 texel texture has a property that crossing boundary of texel(Cell) makes crossing higher level miplevel(more detail miplevel) texel boundary too.
	// But not power of 2 has not this property. so we need to move half texel size of higher level miplevel more.
	//  - Not power of 2 texture will cover 0.5 texel more. so half texel size more is reasonable.
	//  - (crossOffset * 64.0) is same with (crossStep / ScreenSize / vec2(2.0)). This mean that 'crossOffset * 64.0' is half texel size.
	ray = intersectCellBoundary(origin, dest, rayCell, startCellCount, crossStep, crossOffset * 64.0);

	int level = startLevel;
	int iter = 0;
	bool isBackwardRay = ReflDirInTS.z < 0;
	float rayDir = isBackwardRay ? -1 : 1;

	while (level >= stopLevel && ray.z * rayDir <= maxZ * rayDir && iter < MAX_ITERATION)
	{
		vec2 cellCount = getHiZCellCount(level);			// Get old miplevel texel count.
		vec2 oldCellIdx = getCell(ray.xy, cellCount);		// Get old texel position of ray.

		float cell_minZ = getMinimumDepthPlane((oldCellIdx + 0.5) / cellCount, level);		// sampling HiZ Sampler with miplevel

		// Move to next step, when ray is not intersection with HiZ sample.
		vec3 tmpRay = ((cell_minZ > ray.z) && !isBackwardRay) ? intersectDepthPlane(origin, dest, (cell_minZ - minZ) / deltaZ) : ray;

		vec2 newCellIdx = getCell(tmpRay.xy, cellCount);	// Get new texel position of ray.

		// Thickness will use only at miplevel 0, thickness will be used to determine weather ray passed an object completely by using MAX_THICKNESS or not.
		float thickness = (level == 0) ? (ray.z - cell_minZ) : 0;
		
		// Crossed?
		// 1. Backward and Ray is not intersecting with HiZ Depth.
		// 2. Check weather ray passed an object completely or not.
		// 3. Moved to next texel(or Cell)
		bool crossed = (isBackwardRay && (cell_minZ > ray.z)) || (thickness > (MAX_THICKNESS)) || crossedCellBoundary(oldCellIdx, newCellIdx);

		// if it is crossed, go to next Cell boundary because we need adjust position to cellboundary to avoid skipping a texel(or Cell).
		// and MipLevel will increse because there is no intersection with HiZ depth, so we don't need more detail in depth.
		//
		// if it is not crossed, Ray will stay new texel position. and MipLevel will decrease to determine intersection in more detail HiZ MipLevels.
		ray = crossed ? intersectCellBoundary(origin, dest, oldCellIdx, cellCount, crossStep, crossOffset) : tmpRay;
		level = crossed ? min(MaxMipLevel, level + 1) : level - 1;

		++iter;
	}

	// If intersection happened, level 0 also confirm there is not crossing. so level will be -1.
	bool intersected = (level < stopLevel);
	outIntersection = ray;

	float intensity = intersected ? 1 : 0;
	return intensity;
}
#else // USE_HIZ
float FindIntersection_Linear(vec3 samplePosInTS, vec3 ReflDirInTS, float maxTraceDistance, out vec3 outIntersection)
{
	vec3 ReflectionEndPosInTS = samplePosInTS + ReflDirInTS * maxTraceDistance;

	vec3 dp = ReflectionEndPosInTS.xyz - samplePosInTS.xyz;
	ivec2 sampleScreenPos = ivec2(samplePosInTS.xy * ScreenSize);
	ivec2 endPosScreenPos = ivec2(ReflectionEndPosInTS.xy * ScreenSize);
	ivec2 dp2 = endPosScreenPos - sampleScreenPos;

	int max_dist = max(abs(dp2.x), abs(dp2.y));
	dp /= vec3(float(max_dist));

	vec4 rayPosInTS = vec4(samplePosInTS.xyz + dp, 0.0);
	vec4 RayDirInTS = vec4(dp.xyz, 0.0);
	vec4 rayStartPos = rayPosInTS;

	int hitIndex = -1;
	for (int i = 0; i < max_dist && i < MAX_ITERATION; i += 4)
	{
		float depth0 = 0;
		float depth1 = 0;
		float depth2 = 0;
		float depth3 = 0;

		vec4 rayPosInTS0 = rayPosInTS + RayDirInTS * 0.0;
		vec4 rayPosInTS1 = rayPosInTS + RayDirInTS * 1.0;
		vec4 rayPosInTS2 = rayPosInTS + RayDirInTS * 2.0;
		vec4 rayPosInTS3 = rayPosInTS + RayDirInTS * 3.0;

		depth3 = GetDepthSample(rayPosInTS3.xy);
		depth2 = GetDepthSample(rayPosInTS2.xy);
		depth1 = GetDepthSample(rayPosInTS1.xy);
		depth0 = GetDepthSample(rayPosInTS0.xy);

		{
			float thickness = (rayPosInTS3.z - depth3);
			hitIndex = (thickness >= 0 && thickness < MAX_THICKNESS) ? (i + 3) : hitIndex;
		}

		{
			float thickness = (rayPosInTS2.z - depth2);
			hitIndex = (thickness >= 0 && thickness < MAX_THICKNESS) ? (i + 2) : hitIndex;
		}

		{
			float thickness = (rayPosInTS1.z - depth1);
			hitIndex = (thickness >= 0 && thickness < MAX_THICKNESS) ? (i + 1) : hitIndex;
		}

		{
			float thickness = (rayPosInTS0.z - depth0);
			hitIndex = (thickness >= 0 && thickness < MAX_THICKNESS) ? (i + 0) : hitIndex;
		}

		if (hitIndex != -1)
			break;

		rayPosInTS = rayPosInTS3 + RayDirInTS;
	}

	bool intersected = hitIndex >= 0;
	outIntersection = rayStartPos.xyz + RayDirInTS.xyz * hitIndex;

	return (intersected ? 1.0 : 0.0);
}
#endif // USE_HIZ

vec4 ComputeReflectedColor(float intensity, vec3 intersection, vec4 skyColor)
{
	vec4 ssr_color = vec4(texture(SceneColorSampler, intersection.xy));
	return mix(skyColor, ssr_color, intensity);
}

void main()
{
	vec4 DiffuseColor = vec4(0.0); 
	if (DebugReflectionOnly == 0)
		DiffuseColor = texture(SceneColorSampler, TexCoord_);

	vec4 normalInVS = GetViewSpaceNormal(TexCoord_);
	float reflectionMask = normalInVS.w;	// should be fetched from texture 

	vec4 skyColor = vec4(0.0, 0.0, 0.0, 0.0);
	vec4 reflectionColor = vec4(0.0);

	if (reflectionMask != 0.0)
	{
		reflectionColor = skyColor;
		vec3 samplePosInTS = vec3(0);
		vec3 ReflDirInTS = vec3(0);
		float maxTraceDistance = 0;

		ComputePosAndReflection(normalInVS.xyz, samplePosInTS, ReflDirInTS, maxTraceDistance);

		vec3 intersection = vec3(0.0);
		if (ReflDirInTS.z > 0.0)
		{
#if USE_HIZ
			float intensity = FindIntersection_HiZ(samplePosInTS, ReflDirInTS, maxTraceDistance, intersection);
#else // USE_HIZ
			float intensity = FindIntersection_Linear(samplePosInTS, ReflDirInTS, maxTraceDistance, intersection);
#endif // USE_HIZ
			reflectionColor = ComputeReflectedColor(intensity, intersection, skyColor);
		}
	}

	// Add the relfection color to the color of the sample.
	color = DiffuseColor + reflectionColor;
}