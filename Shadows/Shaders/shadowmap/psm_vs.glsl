#version 330 core

#preprocessor
#include "common.glsl"
precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec4 Color;
layout(location = 2) in vec3 Normal;

uniform mat4 MVP;
uniform mat4 M;

out vec3 Pos_;
out vec4 Color_;
out vec3 Normal_;

void main()
{
	gl_Position = MVP * vec4(Pos, 1.0);

    Color_ = Color;
    Normal_ = TransformNormal(M, Normal);
    Pos_ = TransformPos(M, Pos);
}