#version 430 core

precision mediump float;

uniform vec4 Color[4];

in vec2 UV_;
out vec4 color;

void main()
{
    vec4 c1 = mix(Color[0], Color[1], UV_.x);
    vec4 c2 = mix(Color[2], Color[3], UV_.x);

    color = mix(c1, c2, UV_.y);
}