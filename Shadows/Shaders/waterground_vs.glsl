#version 330 core

precision highp float;

layout(location = 0) in vec3 Pos;

uniform mat4 MVP;
uniform mat4 M;

out vec2 TexCoord0_;

void main()
{
	vec4 wPos = M * vec4(Pos, 1.0);
	TexCoord0_.xy = wPos.xz * 0.02;

	gl_Position = MVP * vec4(Pos, 1.0);
}