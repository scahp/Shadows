#version 330 core

#include "common.glsl"

precision mediump float;

layout(location = 0) in vec3 Pos;
uniform mat4 PSM;
uniform mat4 M;
uniform mat4 MVP;

void main()
{
    gl_Position = PSM * M * vec4(Pos, 1.0);
}