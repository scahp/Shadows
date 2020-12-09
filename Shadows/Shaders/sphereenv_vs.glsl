#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;

uniform mat4 MVP;

out vec3 Normal_;

void main()
{
	Normal_ = Normal;
	gl_Position = MVP * vec4(Pos.x, Pos.y, Pos.z, 0.0);	// infinityfar
}