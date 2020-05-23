#version 330 core

precision highp float;

uniform float NearDist;
uniform float FarDist;

in vec4 VPos_;

out vec4 color;

void main()
{
    float vZ = (VPos_.z / VPos_.w);
    vZ = -vZ / FarDist;
    color = vec4(vZ);
    gl_FragDepth = vZ;
}