#version 330 core
#preprocessor

precision mediump float;

uniform sampler2D PosSampler;
uniform sampler2D ShadowMapSampler;
uniform sampler2D DepthSampler;

uniform mat4 ShadowVPMat;
uniform mat4 VP;
uniform vec3 CameraPos;

in vec2 TexCoord_;
out vec4 color;

vec2 TransformShdowMapTextureSpace(vec3 InWorldPos)
{
	vec4 temp = (ShadowVPMat * InWorldPos);
	temp /= temp.w;
	temp.xy = temp.xy * 0.5 + vec2(0.5);
	return temp.xy;
}

vec2 TransformDepthTextureSpace(vec3 InWorldPos)
{
	vec4 temp = (VP * InWorldPos);
	temp /= temp.w;
	temp.xy = temp.xy * 0.5 + vec2(0.5);
	return temp.xy;
}

void main()
{
	vec3 WorldPos = texture(PosSampler, TexCoord_).xyz;
	
	// vec2 PosInSS TransformShdowMapTextureSpace(WorldPos);
	// vec2 CameraPosInSS TransformShdowMapTextureSpace(CameraPos);

	vec3 ToPixel = (WorldPos - CameraPos);
	float TravelDist = sqrt(dot(ToPixel, ToPixel));

#define TRAVEL_COUNT 10.0

	float DeltaDist = TravelDist / TRAVEL_COUNT;
	vec3 ToPixelNormalized = normalized(ToPixel);
	vec3 Delta = ToPixelNormalized * DeltaDist;

	vec3 CurrentPos = CameraPos;
	for (int i = 0; i < 10; ++i)
	{
		CurrentPos += Delta;

		//vec2 ShadowUV = TransformShdowMapTextureSpace(CurrentPos);
		//vec2 DepthUV = TransformDepthTextureSpace(CurrentPos);

		//float ShadowMapZ = texture(ShadowMapSampler, ShadowUV).z;
		//float DepthZ = texture(DepthSampler, DepthUV).z;)
	}

	color = vec4(texture(PosSampler, TexCoord_).xyz, 1.0);
}