#version 330 core

precision highp float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoord;

uniform mat4 MVP;

out vec3 Normal_;
uniform mat4 M;
out vec2 TexCoord_;

void main()
{
	gl_Position = MVP * vec4(Pos, 1.0);
	TexCoord_ = TexCoord;
	Normal_ = Normal;
}