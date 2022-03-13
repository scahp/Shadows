#version 330 core

precision mediump float;

uniform sampler2D tex_object;
uniform int TextureSRGB[1];
uniform int UseTexture;

in vec2 TexCoord_;
in vec4 Color_;

out vec4 color;

void main()
{
	if (UseTexture > 0)
	{
		color = texture2D(tex_object, TexCoord_);
		if (TextureSRGB[0] > 0)
			color.xyz = pow(color.xyz, vec3(2.2));
	}
	else
	{
		color = Color_;
	}
}