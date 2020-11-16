#version 330 core

precision highp float;

uniform sampler2D tex_object;

in vec2 TexCoord0_;

out vec4 color;

void main()
{
    color = texture(tex_object, TexCoord0_);
}