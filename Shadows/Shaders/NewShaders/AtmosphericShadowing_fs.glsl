#version 330 core
#preprocessor

precision mediump float;

uniform sampler2D PosSampler;
uniform sampler2D ShadowMapSampler;
uniform sampler2D DepthSampler;

uniform mat4 ShadowVPMat;
uniform mat4 VP;
uniform vec3 CameraPos;
uniform vec3 LightCameraDirection;
uniform float AnisoG;
uniform int UseNoise;
uniform float CameraNear; // dmin
uniform float CameraFar; // dmax
uniform float SlopeOfDist;
uniform int TravelCount;
uniform float InScatteringLambda;

in vec2 TexCoord_;
out vec4 color;

vec3 TransformShdowMapTextureSpace(vec3 InWorldPos)
{
	vec4 temp = (ShadowVPMat * vec4(InWorldPos, 1.0));
	temp /= temp.w;
	temp.xyz = temp.xyz * 0.5 + vec3(0.5);
	return clamp(temp.xyz, vec3(0.0), vec3(1.0));
}

vec3 TransformNDC(vec3 InWorldPos)
{
	vec4 temp = (VP * vec4(InWorldPos, 1.0));
	temp /= temp.w;
	return clamp(temp.xyz, vec3(0.0), vec3(1.0));
}

float rand(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float AnisotropyIntensity(vec3 InFromSurfaceToCamera)
{
	float atmosphereBrightness = InScatteringLambda * (CameraFar - CameraNear) / TravelCount; // lambda * (dmax − dmin) / (n + 1)

	float g = AnisoG;
	vec3 anisotropyConst = vec3(1.0 - g, 1.0 + g * g, 2.0 * g); // (1 − g, 1 + g * g, 2 * g)

	float invLength = 1.0;
	float h = anisotropyConst.x * (1.0 / sqrt(anisotropyConst.y - anisotropyConst.z * dot(InFromSurfaceToCamera, LightCameraDirection)));
	float intensity = h * h * h * atmosphereBrightness;

	return intensity;
}

float GetAccumulatedInscatteringValue(float InTravelDist, vec3 InToPixelNormalized)
{
	float Depth = texture(DepthSampler, TexCoord_).x;

	float dt = 1.0 / TravelCount;
	float dw = 2.0 * (1.0 - SlopeOfDist) * dt;

	float t = dt;
	if (UseNoise != 0)
		t += dt * rand(vec2(gl_FragCoord.x * gl_FragCoord.y / CameraFar)) * 3.0;
	float tmax = t + 1.0;

	// Start and end Depth to march ray in NDC
	float z1 = TransformNDC(CameraPos + InToPixelNormalized * CameraNear).z;
	float z2 = TransformNDC(CameraPos + InToPixelNormalized * InTravelDist).z;

	// Start and end pos in shadowmap texture space
	vec3 p1 = TransformShdowMapTextureSpace(CameraPos + CameraNear);
	vec3 p2 = TransformShdowMapTextureSpace(CameraPos + InToPixelNormalized * InTravelDist);

	float weight = SlopeOfDist;	// first weight is always SlopeOfDist

	float AccumulatedValue = 0.0;
	for (; t <= tmax; t += dt)
	{
		float u = (t * (1.0 - SlopeOfDist) + SlopeOfDist) * t;
		float z = mix(z1, z2, u);

		// When the z meet Depth on screen, stop raymarching
		if (z > Depth)
			break;

		vec3 CurrentPosInShadowMapTS = mix(p1, p2, u);
		float ShadowMapDepth = texture(ShadowMapSampler, CurrentPosInShadowMapTS.xy).x;
		if (ShadowMapDepth > CurrentPosInShadowMapTS.z)
			AccumulatedValue += weight;

		weight += dw;
	}

	return AccumulatedValue;
}

void main()
{	
	vec3 WorldPos = texture(PosSampler, TexCoord_).xyz;

	vec3 ToPixel = (WorldPos - CameraPos);
	float TravelDist = sqrt(dot(ToPixel, ToPixel));
	vec3 ToPixelNormalized = normalize(ToPixel);

	float AccumulatedValue = GetAccumulatedInscatteringValue(TravelDist, ToPixelNormalized);
	AccumulatedValue *= AnisotropyIntensity(-ToPixelNormalized);

	color.x = AccumulatedValue;
}