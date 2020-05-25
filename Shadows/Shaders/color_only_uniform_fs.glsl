#version 330 core

precision mediump float;

uniform vec4 Color;

out vec4 OutColor;

void main()
{
    OutColor = vec4(Color);
}