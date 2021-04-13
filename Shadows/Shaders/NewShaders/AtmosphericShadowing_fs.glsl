#version 330 core
#preprocessor

precision mediump float;

uniform sampler2D PosSampler;
uniform sampler2D ShadowMapSampler;
uniform sampler2D DepthSampler;

uniform mat4 ShadowVPMat;
uniform mat4 InvMainCameraShadowVPMat;
uniform mat4 VP;
uniform mat4 V;
uniform vec3 CameraPos;
uniform vec3 LightCameraDirection;
uniform vec3 CameraDirection;
uniform float AnisoG;
uniform int UseNoise;
uniform vec3 CameraPosInShadowMap;
uniform float s;	// aspect ratio
uniform float g;	// projection distance
uniform mat4 P;

float TRAVEL_COUNT = 20.0;
float minAtmosphereDepth = 1.0; // dmin
float maxAtmosphereDepth = 1000.0; // dmax
float InScatteringLambda = 0.5;

in vec2 TexCoord_;
out vec4 color;

vec3 TransformShdowMapTextureSpace(vec3 InWorldPos)
{
	vec4 temp = (ShadowVPMat * vec4(InWorldPos, 1.0));
	temp /= temp.w;
	temp.xyz = temp.xyz * 0.5 + vec3(0.5);
	return clamp(temp.xyz, vec3(0.0), vec3(1.0));
}

float rand(vec2 co)
{
	return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

void CalculateAtmosphericShadowRays(out vec2 OutCameraRay, out vec3 OutShadowRay, vec2 InCoordInNDC)
{
	// Calculate point on camera-space at plane z = minAtmosphereDepth from CoordinateInNDC
	vec3 q = vec3(InCoordInNDC.x * minAtmosphereDepth * s / g, InCoordInNDC.y * minAtmosphereDepth / g, minAtmosphereDepth);
	OutCameraRay.xy = q.xy;

	// Transform camera-ray direction into shadowmap space
	OutShadowRay.x = dot(InvMainCameraShadowVPMat[0].xyz, q);
	OutShadowRay.y = dot(InvMainCameraShadowVPMat[1].xyz, q);
	OutShadowRay.z = dot(InvMainCameraShadowVPMat[2].xyz, q);
	OutShadowRay.xyz = OutShadowRay.xyz * 0.5 + vec3(0.5);
}

float CalculateAtmosphericShadowing(vec2 InCameraRay, vec3 InShadowRay)
{
	vec4 PosInCS = vec4(texture(PosSampler, TexCoord_).xyz, 1.0);
	PosInCS = V * PosInCS;
	PosInCS /= PosInCS.w;
	float ZInCS = -PosInCS.z;

	float m = 0.25;
	float weight = m;	// first weight is always m

	float dt = 1.0 / TRAVEL_COUNT;
	float dw = 2.0 * (1.0 - m) * dt;

	// CameraRay is a point on plane z = minAtmosphereDepth, so we added (minAtmosphereDepth * minAtmosphereDepth) to calculate length of q
	float invLength_q = 1.0 / sqrt(dot(InCameraRay, InCameraRay) + minAtmosphereDepth * minAtmosphereDepth);
	
	// Scale variable for making cameraray have length of minAtmosphereDepth
	float scale = minAtmosphereDepth / invLength_q;

	float atmosphericDepthRatio = maxAtmosphereDepth / minAtmosphereDepth;

	// Begin and End of point in shadowmap
	vec3 p1 = InShadowRay * scale;
	vec3 p2 = p1 * atmosphericDepthRatio + CameraPosInShadowMap;	// it is equal to maxAtmosphereDepth * InShadowRay / invLength_q
	p1 += CameraPosInShadowMap;

	// Begin and End of depth in camera space
	float z1 = minAtmosphereDepth * scale;
	float z2 = z1 * atmosphericDepthRatio;

	float t = dt;
	if (UseNoise != 0)
		t *= rand(vec2(gl_FragCoord.x * gl_FragCoord.y));
	float tmax = t + 1.0;

	float AccumulatedValue = 0.0;

	float zMax = min(ZInCS, maxAtmosphereDepth);
	for (; t <= tmax; t += dt)
	{
		float u = (t * (1.0 - m) + m) * t;
		float z = mix(z1, z2, u);
		if (z > zMax)
		{
			break;
		}

		vec3 ShadowMapUV = mix(p1, p2, u);

		float ShadowMapDepth = texture(ShadowMapSampler, ShadowMapUV.xy).x;
		if (ShadowMapDepth > ShadowMapUV.z)
			AccumulatedValue += weight;

		weight += dw;
	}

	return AccumulatedValue;
}

float AnisotropyIntensity(vec2 InCameraRay, float InInvLengthQ)
{
	float atmosphereBrightness = InScatteringLambda * (maxAtmosphereDepth - minAtmosphereDepth) / TRAVEL_COUNT; // lambda * (dmax − dmin) / (n + 1)

	float g = AnisoG;
	vec3 anisotropyConst = vec3(1.0 - g, 1.0 + g * g, 2.0 * g); // (1 − g, 1 + g * g, 2 * g)

	float invLength = 1.0;
	float h = anisotropyConst.x * (1.0 / sqrt(anisotropyConst.y - anisotropyConst.z * dot(vec3(InCameraRay, minAtmosphereDepth), LightCameraDirection) * InInvLengthQ));
	float intensity = h * h * h * atmosphereBrightness;

	return intensity;
}

float GetAccumulatedInscatteringValue(float InTravelDist, vec3 InToPixelNormalized)
{
	float Depth = texture(DepthSampler, TexCoord_).x;

	float m = 0.25;
	float weight = m;	// first weight is always m

	float dt = 1.0 / TRAVEL_COUNT;
	float dw = 2.0 * (1.0 - m) * dt;

	float t = dt;
	if (UseNoise != 0)
		t *= rand(vec2(gl_FragCoord.x * gl_FragCoord.y));
	float tmax = t + 1.0;

	vec3 CurrentPos = CameraPos;
	float AccumulatedValue = 0.0;
	for (; t <= tmax; t += dt)
	{
		float u = (t * (1.0 - m) + m) * t;
		float z = mix(minAtmosphereDepth, InTravelDist, u);
		vec3 v = mix(InToPixelNormalized * minAtmosphereDepth, InToPixelNormalized * InTravelDist, u);

		v = CameraPos + (InToPixelNormalized * (InTravelDist * u));

		vec4 temp = VP * vec4(v, 1.0);
		temp /= temp.w;

		if (temp.z > Depth)
		{
			break;
		}

		vec3 CurrentPosInShadowMapTS;
		{
			vec4 temp = (ShadowVPMat * vec4(v, 1.0));
			temp /= temp.w;
			temp.xyz = temp.xyz * 0.5 + vec3(0.5);
			CurrentPosInShadowMapTS = clamp(temp.xyz, vec3(0.0), vec3(1.0));
		}

		float ShadowMapDepth = texture(ShadowMapSampler, CurrentPosInShadowMapTS.xy).x;

		if (ShadowMapDepth > CurrentPosInShadowMapTS.z)
			AccumulatedValue += weight;

		weight += dw;
	}

	return AccumulatedValue;
}

void main()
{
	vec2 CoordInNDC = (TexCoord_ * 2.0 - vec2(1.0));
	vec2 CameraRay;
	vec3 ShadowRay;
	CalculateAtmosphericShadowRays(CameraRay, ShadowRay, CoordInNDC);
	//color.xy = CameraRay;
	//color.zw = vec2(0.0, 1.0);
	//return;
	float AccumulatedValue = CalculateAtmosphericShadowing(CameraRay, ShadowRay);
	//color.xyz = ShadowRay;
	//return;

	/*
	vec3 WorldPos = texture(PosSampler, TexCoord_).xyz;

	vec3 ToPixel = (WorldPos - CameraPos);
	float TravelDist = sqrt(dot(ToPixel, ToPixel));

	float DeltaDist = TravelDist / TRAVEL_COUNT;
	vec3 ToPixelNormalized = normalize(ToPixel);
	vec3 Delta = ToPixelNormalized * DeltaDist;

	float AccumulatedValue = GetAccumulatedInscatteringValue(TravelDist, ToPixelNormalized);
	*/
	//float invLength_q = 1.0 / sqrt(dot(CameraRay, CameraRay) + minAtmosphereDepth * minAtmosphereDepth);
	//AccumulatedValue *= AnisotropyIntensity(CameraRay, invLength_q);

	color = vec4(AccumulatedValue, AccumulatedValue, AccumulatedValue, 1.0);
}