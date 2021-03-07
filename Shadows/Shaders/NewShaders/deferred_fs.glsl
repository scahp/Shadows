#version 330 core

#preprocessor

#include "common.glsl"

precision mediump float;

#if defined(USE_MATERIAL)
uniform jMaterial Material;
#endif // USE_MATERIAL

uniform vec3 Eye;

#if defined(USE_TEXTURE)
uniform sampler2D DiffuseSampler;
uniform int TextureSRGB[1];

uniform sampler2D NormalSampler;
uniform int UseNormalSampler;

uniform int DebugWithNormalMap;
#endif // USE_TEXTURE

in vec3 Pos_;
in vec3 PosInView_;
in vec4 Color_;
in vec3 Normal_;
in vec3 NormalInView_;
in mat3 TBN_;
in mat3 TBNInView_;

#if defined(USE_TEXTURE)
in vec2 TexCoord_;
#endif // USE_TEXTURE

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_pos;

void main()
{
    vec3 normal = normalize(Normal_);
	vec3 normalInView = normalize(NormalInView_);
	vec4 diffuse = vec4(1.0);

#if defined(USE_TEXTURE)
	if (TextureSRGB[0] > 0)
	{
		// from sRGB to Linear color
		vec4 tempColor = texture(DiffuseSampler, TexCoord_);
		diffuse.xyz *= pow(tempColor.xyz, vec3(2.2));
		diffuse.w *= tempColor.w;
	}
	else
	{
		diffuse *= texture(DiffuseSampler, TexCoord_);
	}
#else
	diffuse = Color_;
#endif // USE_TEXTURE

	out_color = diffuse;
	out_color.xyz *= Material.Diffuse;

	if (UseNormalSampler > 0 && DebugWithNormalMap > 0)
	{
		// skip to transform normal because WorldMatrix is identity now.
		out_normal.xyz = texture(NormalSampler, TexCoord_).xyz;
		out_normal.xyz = (out_normal.xyz * 2.0 - 1.0);
		out_normal.xyz = normalize(TBN_ * out_normal.xyz);
	}
	else
	{
		out_normal.xyz = normal;
	}
	out_normal.w = Material.Reflectivity;
	
	out_pos.xyz = Pos_;
}