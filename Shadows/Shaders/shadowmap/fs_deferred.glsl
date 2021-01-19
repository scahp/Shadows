#version 330 core

#preprocessor

#include "common.glsl"

precision mediump float;
precision mediump sampler2DArray;

layout (std140) uniform DirectionalLightShadowMapBlock
{
	mat4 ShadowVP;
	mat4 ShadowV;
	vec3 LightPos;      // Directional Light Pos �ӽ�
	float LightZNear;
	float LightZFar;
};

layout (std140) uniform PointLightShadowMapBlock
{
	float PointLightZNear;
	float PointLightZFar;
};

layout (std140) uniform SpotLightShadowMapBlock
{
	float SpotLightZNear;
	float SpotLightZFar;
};

#if defined(USE_MATERIAL)
uniform jMaterial Material;
#endif // USE_MATERIAL

uniform vec3 Eye;
uniform int Collided;

#if defined(USE_TEXTURE)
uniform sampler2D tex_object2;
uniform int TextureSRGB[1];
#endif // USE_TEXTURE

uniform sampler2D shadow_object_point;
uniform sampler2D shadow_object_spot;
uniform sampler2D shadow_object;
uniform int UseTexture;
uniform int UseUniformColor;
uniform vec4 Color;
uniform int UseMaterial;
uniform int ShadingModel;

// in vec3 ShadowPos_;
// in vec3 ShadowCameraPos_;
in vec3 Pos_;
in vec4 Color_;
in vec3 Normal_;

#if defined(USE_TEXTURE)
in vec2 TexCoord_;
#endif // USE_TEXTURE

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_posInWorld;
layout (location = 3) out vec4 out_posInLight;

void main()
{
    vec3 normal = normalize(Normal_);
    vec3 viewDir = normalize(Eye - Pos_);

    vec4 tempShadowPos = (ShadowVP * vec4(Pos_, 1.0));
    tempShadowPos /= tempShadowPos.w;
    vec3 ShadowPos = tempShadowPos.xyz * 0.5 + 0.5;        // Transform NDC space coordinate from [-1.0 ~ 1.0] into [0.0 ~ 1.0].

    vec4 shadowCameraPos = (ShadowV * vec4(Pos_, 1.0));    

	vec4 diffuse = vec4(1.0);

#if defined(USE_TEXTURE)
	if (UseTexture > 0)
	{
		if (TextureSRGB[0] > 0)
		{
			// from sRGB to Linear color
			vec4 tempColor = texture(tex_object2, TexCoord_);
			diffuse.xyz *= pow(tempColor.xyz, vec3(2.2));
			diffuse.w *= tempColor.w;
		}
		else
		{
			diffuse *= texture(tex_object2, TexCoord_);
		}
	}
	else
	{
		diffuse = Color_;
	}
#else
	diffuse = Color_;
#endif // USE_TEXTURE

	if (UseUniformColor > 0)
		diffuse = Color;

    if (Collided != 0)
        diffuse = vec4(1.0, 1.0, 1.0, 1.0);

#if defined(USE_MATERIAL)
	if (UseMaterial > 0)
	{
		diffuse.xyz *= Material.Diffuse;
		diffuse.xyz += Material.Emissive;
	}
#endif // USE_MATERIAL

	out_color = diffuse;
	out_normal.xyz = normal;
	out_normal.w = float(ShadingModel);
	out_posInWorld.xyz = Pos_;
	out_posInLight.xyz = ShadowPos;
}