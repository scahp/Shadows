#version 330 core

precision mediump float;

uniform int ID;

out vec4 color;

void main()
{
    color = vec4(float(ID));
}