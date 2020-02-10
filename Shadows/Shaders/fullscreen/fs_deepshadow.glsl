#version 430 core

#preprocessor

#include "common.glsl"

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1
#define FILTER_SIZE 2
#define FILTER_AREA ((FILTER_SIZE * 2 + 1) * (FILTER_SIZE * 2 + 1))

layout (std140) uniform DirectionalLightBlock
{
	jDirectionalLight DirectionalLight[MAX_NUM_OF_DIRECTIONAL_LIGHT];
};

uniform int NumOfDirectionalLight;

layout (std140) uniform DirectionalLightShadowMapBlock
{
	mat4 ShadowVP;
	mat4 ShadowV;
	vec3 LightPos;      // Directional Light Pos temp
	float LightZNear;
	float LightZFar;
};

#if defined(USE_MATERIAL)
uniform int UseMaterial;
uniform jMaterial Material;
#endif // USE_MATERIAL

uniform int UseAmbientLight;
uniform jAmbientLight AmbientLight;

uniform vec3 Eye;
uniform int ShadowOn;
uniform int ShadowMapWidth;
uniform int ShadowMapHeight;

in vec2 TexCoord_;

uniform sampler2DMS ColorSampler;
uniform sampler2DMS NormalSampler;
uniform sampler2DMS PosInWorldSampler;
uniform sampler2DMS PosInLightSampler;
uniform sampler2D ResolvedColorSampler;

layout(location = 0) out vec4 color;

void main()
{
	float shading = 1.0;

	vec4 DiffuseColor = vec4(0);
	vec3 ResultDirectColor = vec3(0);
	if (texture(ResolvedColorSampler, TexCoord_).a > 0.02)
	//if (texelFetch(PosInWorldSampler, ivec2(gl_FragCoord.xy), gl_SampleID).w > 0.0)
	{
		const int sampleCount = 4;
		for (int k = 0; k < sampleCount; ++k)
		{
			vec4 Pos = texelFetch(PosInWorldSampler, ivec2(gl_FragCoord.xy), k); //texture(PosInWorldSampler, TexCoord_).xyz;
			vec3 ShadowPos = texelFetch(PosInLightSampler, ivec2(gl_FragCoord.xy), k).xyz; //texture(PosInLightSampler, TexCoord_).xyz;
			vec4 Normal = texelFetch(NormalSampler, ivec2(gl_FragCoord.xy), k); //texture(NormalSampler, TexCoord_);

			vec3 toLight = normalize(LightPos - Pos.xyz);
			vec3 toEye = normalize(Eye - Pos.xyz);

			for (int i = 0; i < MAX_NUM_OF_DIRECTIONAL_LIGHT; ++i)
			{
				if (i >= NumOfDirectionalLight)
					break;

				jDirectionalLight light = DirectionalLight[i];
#if defined(USE_MATERIAL)
				if (UseMaterial > 0)
				{
					light.SpecularLightIntensity = Material.Specular;
					light.SpecularPow = Material.Shininess;
				}
#endif // USE_MATERIAL

				ResultDirectColor += GetDirectionalLight(light, Normal.xyz, toEye);
			}
			DiffuseColor += texelFetch(ColorSampler, ivec2(gl_FragCoord.xy), k); // texture(ColorSampler, TexCoord_);
		}
		ResultDirectColor = (ResultDirectColor / float(sampleCount)) * (DiffuseColor.xyz / float(sampleCount));
		DiffuseColor /= float(sampleCount);
	}
	else
	{
		vec4 Pos = texelFetch(PosInWorldSampler, ivec2(gl_FragCoord.xy), gl_SampleID); //texture(PosInWorldSampler, TexCoord_).xyz;
		vec3 ShadowPos = texelFetch(PosInLightSampler, ivec2(gl_FragCoord.xy), gl_SampleID).xyz; //texture(PosInLightSampler, TexCoord_).xyz;
		vec4 Normal = texelFetch(NormalSampler, ivec2(gl_FragCoord.xy), gl_SampleID); //texture(NormalSampler, TexCoord_);

		vec3 toLight = normalize(LightPos - Pos.xyz);
		vec3 toEye = normalize(Eye - Pos.xyz);

		for (int i = 0; i < MAX_NUM_OF_DIRECTIONAL_LIGHT; ++i)
		{
			if (i >= NumOfDirectionalLight)
				break;

			jDirectionalLight light = DirectionalLight[i];
#if defined(USE_MATERIAL)
			if (UseMaterial > 0)
			{
				light.SpecularLightIntensity = Material.Specular;
				light.SpecularPow = Material.Shininess;
			}
#endif // USE_MATERIAL

			ResultDirectColor += GetDirectionalLight(light, Normal.xyz, toEye);
		}
		DiffuseColor = texelFetch(ColorSampler, ivec2(gl_FragCoord.xy), gl_SampleID); // texture(ColorSampler, TexCoord_);
		ResultDirectColor = ResultDirectColor * DiffuseColor.xyz;
	}

	vec3 ambientColor = vec3(0.0, 0.0, 0.0);
	if (UseAmbientLight != 0)
		ambientColor += GetAmbientLight(AmbientLight);
	
	ambientColor *= DiffuseColor.xyz;

	color = vec4(ambientColor + ResultDirectColor, 1.0);
}