#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;

uniform mat4 MVP;

void main()
{
    gl_Position = MVP * vec4(Pos, 1.0);
}