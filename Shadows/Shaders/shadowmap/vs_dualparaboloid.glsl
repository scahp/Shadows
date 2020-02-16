#version 330 core

#preprocessor

#include "common.glsl"

precision mediump float;

layout(location = 0) in vec3 Pos;

uniform mat4 MV;

void main()
{
	vec4 PosMV = MV * vec4(Pos, 1.0);
	PosMV /= PosMV.w;
	gl_Position = PosMV;
}