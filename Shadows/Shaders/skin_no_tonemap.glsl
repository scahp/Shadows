#version 330 core

precision mediump float;

uniform sampler2D tex_object;
in vec2 TexCoord_;

out vec4 color;

void main()
{
	color = texture2D(tex_object, TexCoord_);
	color.xyz = pow(color.xyz, vec3(1.0 / 2.2));
}