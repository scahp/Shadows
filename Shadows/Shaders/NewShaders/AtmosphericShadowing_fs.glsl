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
uniform vec3 CameraDirection;

in vec2 TexCoord_;
out vec4 color;

vec3 TransformShdowMapTextureSpace(vec3 InWorldPos)
{
	vec4 temp = (ShadowVPMat * vec4(InWorldPos, 1.0));
	temp /= temp.w;
	temp.xyz = temp.xyz * 0.5 + vec3(0.5);
	return clamp(temp.xyz, vec3(0.0), vec3(1.0));
}

void main()
{
	vec3 anisotropConst; // (1 − g, 1 + g * g, 2 * g)
	float g = 0.5;
	anisotropyConst.x = 1.0 - g;
	anisotropyConst.y = 1.0 + g * g;
	anisotropyConst.z = 2.0 * g;
	float dmin = 1.0;
	float dmax = 10.0;
	float invlength = 1.0 / (dmax - dmin);

	float h = anisotropyConst.x * 1.0 / (anisotropyConst.y − anisotropyConst.z *
		dot(vec3(normalize(CameraDirection.xyz), dmin), cameraLightDirection) * invlength);

	float atmosphereBrightness;
	float intensity = h * h * h * atmosphereBrightness;

	vec3 WorldPos = texture(PosSampler, TexCoord_).xyz;
	float Depth = texture(DepthSampler, TexCoord_).x;

	vec3 ToPixel = (WorldPos - CameraPos);
	float TravelDist = sqrt(dot(ToPixel, ToPixel));

#define TRAVEL_COUNT 10.0

	float DeltaDist = TravelDist / TRAVEL_COUNT;
	vec3 ToPixelNormalized = normalize(ToPixel);
	vec3 Delta = ToPixelNormalized * DeltaDist;

	vec3 CurrentPos = CameraPos;
	float AccmulatedValue = 0.0;
	for (int i = 0; i < 10; ++i)
	{
		CurrentPos += Delta;

		vec3 CurrentPosInShadowMapTS = TransformShdowMapTextureSpace(CurrentPos);
		float ShadowMapDepth = texture(ShadowMapSampler, CurrentPosInShadowMapTS.xy).x;

		vec4 temp = VP * vec4(CurrentPos, 1.0);
		temp /= temp.w;

		if (temp.z > Depth)
			break;

		if (ShadowMapDepth > CurrentPosInShadowMapTS.z)
			AccmulatedValue += (1.0 / TRAVEL_COUNT);
	}

	AccmulatedValue *= intensity;
	color = vec4(AccmulatedValue, AccmulatedValue, AccmulatedValue, 1.0);
}