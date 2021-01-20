#version 330 core

#preprocessor

#include "common.glsl"

precision mediump float;

#if defined(USE_MATERIAL)
uniform jMaterial Material;
#endif // USE_MATERIAL

uniform vec3 Eye;
uniform int Collided;

#if defined(USE_TEXTURE)
uniform sampler2D DiffuseSampler;
uniform int TextureSRGB[1];

uniform sampler2D DisplacementSampler;
uniform int UseDisplacementSampler;
#endif // USE_TEXTURE

in vec3 Pos_;
in vec4 Color_;
in vec3 Normal_;
in mat3 TBN;

#if defined(USE_TEXTURE)
in vec2 TexCoord_;
#endif // USE_TEXTURE

layout (location = 0) out vec4 out_color;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_posInWorld;

void main()
{
    vec3 normal = normalize(Normal_);
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

	if (UseDisplacementSampler > 0)
	{
		out_normal.xyz = texture(DisplacementSampler, TexCoord_).xyz;
		out_normal.xyz = (out_normal.xyz * 2.0 - 1.0);
		out_normal.xyz = normalize(TBN * out_normal.xyz);
	}
	else
	{
		out_normal.xyz = normal;
	}
	
	out_posInWorld.xyz = Pos_;
}