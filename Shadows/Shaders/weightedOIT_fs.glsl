#version 330 core

precision mediump float;

in vec4 Color_;
in float LinearZ;

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 alpha;
layout(location = 2) out vec4 debug;

uniform vec4 ColorUniform;

#define ORIGINAL_WEIGHT_FUNC 0

void main()
{
    vec3 ColorN = ColorUniform.xyz;
    float AlphaN = ColorUniform.w;

#if ORIGINAL_WEIGHT_FUNC
    // Tuned to work well with FP16 accumulation buffers and 0.001 < linearDepth < 2.5
    // See Equation (9) from http://jcgt.org/published/0002/02/09/
    float WeightN = clamp(0.03 / (1e-5 + pow(LinearZ, 4.0)), 0.01, 3e3);
    
    // To avoid the transparent object to be black with far distance.
    WeightN = max(WeightN, 1.0);
#else
    // _WEIGHTED0 from https://github.com/candycat1992/OIT_Lab/blob/master/Assets/OIT/WeightedBlended/Shaders/WB_Accumulate.shader
    float WeightN = pow(LinearZ, -3.0);
#endif

    debug.x = gl_FragCoord.z;
    debug.y = abs(LinearZ);
    debug.z = WeightN;

    color = vec4(ColorN * AlphaN, AlphaN) * WeightN;
    alpha = vec4(AlphaN);
}