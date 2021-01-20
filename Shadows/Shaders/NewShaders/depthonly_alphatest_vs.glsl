#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 TexCoord;

uniform mat4 MVP;

out vec2 TexCoord_;

void main()
{
    TexCoord_ = TexCoord;
    gl_Position = MVP * vec4(Pos, 1.0);
}