#version 330 core

precision mediump float;

in vec4 Color_;

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 alpha;

void main()
{
    vec3 ColorN = Color_.xyz;
    float AlphaN = Color_.w;

    float k = 3 * 1000 * (1.0 - gl_FragCoord.z * gl_FragCoord.z * gl_FragCoord.z);
    float WeightN = AlphaN * max(0.01, k);

    color = vec4(ColorN, AlphaN) * vec4(WeightN);
    alpha = vec4(AlphaN);
}