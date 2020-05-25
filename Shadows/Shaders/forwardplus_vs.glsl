#version 430 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec4 Color;

uniform mat4 MVP;
uniform mat4 M;

out vec3 Pos_;
out vec4 Color_;

void main()
{
    Color_ = Color;
    vec4 Temp = M * vec4(Pos, 1.0);
    Pos_ = Temp.xyz / Temp.w;
    gl_Position = MVP * vec4(Pos, 1.0);
}
