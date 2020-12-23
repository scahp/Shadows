#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Radiance;
layout(location = 2) in vec3 TotalRadiance;

uniform mat4 MVP;
uniform vec3 Diffuse;
uniform vec3 Emission;
uniform float Area;

out vec4 Color_;

void main()
{
    gl_Position = MVP * vec4(Pos, 1.0);

    vec3 Exitance = Radiance.xyz * Diffuse;       // Radiosity * Diffuse
    Color_ = vec4(Emission + Exitance, 1.0);
}