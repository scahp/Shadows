#version 330 core

precision highp float;

layout(location = 0) in vec3 Pos;

uniform mat4 MVP;
uniform mat4 MV;

out vec4 VPos_;

void main()
{
    VPos_ = MV * vec4(Pos, 1.0);
    gl_Position = MVP * vec4(Pos, 1.0);
}