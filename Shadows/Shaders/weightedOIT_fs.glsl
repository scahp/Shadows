#version 330 core

precision mediump float;

in vec4 Color_;
in float LinearZ;

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 alpha;

uniform vec4 ColorUniform;

float linearize_depth(float d, float zNear, float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

void main()
{
    vec3 ColorN = ColorUniform.xyz;
    float AlphaN = ColorUniform.w;

    // Weight function form : http://casual-effects.blogspot.com/2014/03/weighted-blended-order-independent.html
    // Insert your favorite weighting function here.
    // The color-based factor avoids color pollution from the edges of wispy clouds.
    // The z-based factor gives precedence to nearer surfaces.
    float WeightN = max(min(1.0, max(max(ColorN.r, ColorN.g), ColorN.b) * AlphaN), AlphaN) * clamp(0.03 / (1e-5 + pow(gl_FragCoord.z / 200, 4.0)), 1e-2, 3e3);

    color = vec4(ColorN, AlphaN) * vec4(WeightN);
    alpha = vec4(AlphaN);
}