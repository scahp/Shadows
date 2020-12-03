#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;

uniform mat4 MVP;
uniform mat4 M;

out vec3 Pos_;
out vec3 Normal_;

void main()
{
    vec4 tempPos = M * vec4(Pos, 1.0);
    Pos_ = tempPos.xyz;
    Normal_ = Normal;
    gl_Position = MVP * vec4(Pos, 1.0);
}