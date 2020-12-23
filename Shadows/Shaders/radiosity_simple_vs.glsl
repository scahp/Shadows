#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Radiance;
layout(location = 2) in vec3 TotalRadiance;

uniform mat4 MVP;
uniform vec3 Diffuse;
uniform vec3 Emission;
uniform float Area;
uniform int DrawRadiance;

out vec4 Color_;

void main()
{
    gl_Position = MVP * vec4(Pos, 1.0);
    if (DrawRadiance > 0)
        Color_ = vec4(Radiance, 1.0);
    else
        Color_ = vec4(Diffuse, 1.0);
}