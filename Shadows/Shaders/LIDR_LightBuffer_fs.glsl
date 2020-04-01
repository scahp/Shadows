#version 330 core

precision mediump float;

uniform vec4 LightIndex;
out vec4 color;

void main()
{
    color = LightIndex;
}