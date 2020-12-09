#version 330 core

precision mediump float;

in float SH_;

out vec4 color;

void main()
{
    color.xyz = mix(vec3(0.0, 0.0, 1.0), vec3(0.0, 1.0, 0.0), SH_ * 0.5 + 0.5);
}