#version 330 core

#preprocessor

#include "common.glsl"

precision mediump float;

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1
layout(std140) uniform DirectionalLightBlock
{
	jDirectionalLight DirectionalLight[MAX_NUM_OF_DIRECTIONAL_LIGHT];
};

layout(std140) uniform DirectionalLightShadowMapBlock
{
	mat4 ShadowVP;
	mat4 ShadowV;
	vec3 LightPos;
	float LightZNear;
	float LightZFar;
	vec2 ShadowMapSize;
};

#define SHADOW_BIAS_DIRECTIONAL 0.01

// SSM + Directional
float IsShadowing(vec3 lightClipPos, sampler2DShadow shadowSampler)
{
	// for sampler2DShadow
	return texture(shadowSampler, vec3(lightClipPos.xy, lightClipPos.z - SHADOW_BIAS_DIRECTIONAL));

	// for sampler2D
	//float currentDepth = lightClipPos.z - SHADOW_BIAS_DIRECTIONAL;
	//float closestDepth = texture(shadowSampler, lightClipPos.xy).r;
	//float shadow = currentDepth < closestDepth ? 1.0 : 0.0;
	//return shadow;
}

uniform int NumOfDirectionalLight;

uniform sampler2D ColorSampler;
uniform sampler2D NormalSampler;
uniform sampler2D PosSampler;
uniform sampler2D SSAOSampler;

uniform sampler2DShadow DirectionalShadowSampler;

uniform vec3 Eye;

in vec2 TexCoord_;
out vec4 FinalColor;

void main()
{
	vec4 DiffuseAndOpacity = texture(ColorSampler, TexCoord_);
	vec3 Normal = texture(NormalSampler, TexCoord_).xyz;
	vec3 Pos = texture(PosSampler, TexCoord_).xyz;
	float SSAO = texture(SSAOSampler, TexCoord_).x;

	vec3 viewDir = normalize(Eye - Pos);
	vec3 directLight = vec3(1.0);
	if (NumOfDirectionalLight > 0)
	{
		jDirectionalLight light = DirectionalLight[0];
		directLight = GetDirectionalLight(light, Normal, viewDir);
	}

	vec4 tempShadowPos = (ShadowVP * vec4(Pos, 1.0));
	tempShadowPos /= tempShadowPos.w;
	vec3 ShadowPos;
	ShadowPos.xyz = tempShadowPos.xyz * 0.5 + 0.5;        // Transform NDC space coordinate from [-1.0 ~ 1.0] into [0.0 ~ 1.0].
	float IsShadowing = IsShadowing(ShadowPos, DirectionalShadowSampler);
	
	vec3 ConstantAmbient = DiffuseAndOpacity.xyz * 0.3;
	vec3 ColorResult = (ConstantAmbient + DiffuseAndOpacity.xyz * directLight * IsShadowing) * SSAO;
	FinalColor = vec4(ColorResult, DiffuseAndOpacity.w);
	//FinalColor = vec4(directLight, DiffuseAndOpacity.w);
	//FinalColor = DiffuseAndOpacity;
}
