#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;

uniform mat4 MVP;
uniform mat4 M;

out vec3 Pos_;

void main()
{
    gl_Position = MVP * vec4(Pos, 1.0);
    Pos_ = (M * vec4(Pos, 1.0)).xyz;
}