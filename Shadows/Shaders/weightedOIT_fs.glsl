#version 330 core

precision mediump float;

in vec4 Color_;

out vec4 color;

void main()
{
    color = vec4(Color_);
}