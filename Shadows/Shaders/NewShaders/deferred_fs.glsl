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
uniform sampler2D tex_object2;
uniform int TextureSRGB[1];
#endif // USE_TEXTURE

in vec3 Pos_;
in vec4 Color_;
in vec3 Normal_;

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
		vec4 tempColor = texture(tex_object2, TexCoord_);
		diffuse.xyz *= pow(tempColor.xyz, vec3(2.2));
		diffuse.w *= tempColor.w;
	}
	else
	{
		diffuse *= texture(tex_object2, TexCoord_);
	}
#else
	diffuse = Color_;
#endif // USE_TEXTURE

	out_color = diffuse;
	out_normal.xyz = normal;
	out_posInWorld.xyz = Pos_;
}