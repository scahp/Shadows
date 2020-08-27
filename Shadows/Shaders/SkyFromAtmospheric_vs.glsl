#version 330 core

precision highp float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;

uniform mat4 MVP;
uniform mat4 M;
out vec3 Normal_;

void main()
{
	gl_Position = MVP * vec4(Pos, 1.0);
	Normal_ = Normal;
}
