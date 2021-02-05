#version 330 core

precision highp float;

uniform sampler2D SceneColorSampler;
uniform sampler2D NormalSampler;
uniform sampler2D DepthSampler;

uniform mat4 V;
uniform mat4 P;
uniform mat4 InvP;
uniform vec2 ScreenSize;
uniform float Near;
uniform float Far;

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

vec4 ComputeReflectedColor(float intensity, vec3 intersection, vec4 skyColor)
{
	vec4 ssr_color = vec4(texture(SceneColorSampler, intersection.xy));
	return mix(skyColor, ssr_color, intensity);
}

void main()
{
	vec4 DiffuseColor = texture(SceneColorSampler, TexCoord_);
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
			float intensity = FindIntersection_Linear(samplePosInTS, ReflDirInTS, maxTraceDistance, intersection);
			reflectionColor = ComputeReflectedColor(intensity, intersection, skyColor);
		}
	}

	// Add the relfection color to the color of the sample.
	color = DiffuseColor + reflectionColor;
}