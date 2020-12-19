#version 430 core

precision highp float;

#define SAMPLE_INTERVAL_X 16
#define SAMPLE_INTERVAL_Y 16

layout (local_size_x = SAMPLE_INTERVAL_X, local_size_y = SAMPLE_INTERVAL_Y) in;

layout(binding = 0, rgba16f) uniform readonly image2D tex_object;

const float PI = 3.1415926535897932384626433832795028841971693993751058209749;
const float SHBasisR = 1.0;
const float sampleDelta = 0.015f;

uniform int TexWidth;
uniform int TexHeight;

// SSBO
layout(std430, binding = 0) buffer LlmBuffer
{
	vec4 Llm[9];        // w component is padding
};

shared vec3 GlobalLlm[SAMPLE_INTERVAL_X * SAMPLE_INTERVAL_Y][9];
shared int GlobalNumOfSamples;

// https://www.pauldebevec.com/Probes/
vec2 GetSphericalMap_TwoMirrorBall(vec3 InDir)
{
    // Convert from Direction3D to UV[-1, 1]
    // - Direction : (Dx, Dy, Dz)
    // - r=(1/pi)*acos(Dz)/sqrt(Dx^2 + Dy^2)
    // - (Dx*r,Dy*r)
    //
    // To rerange UV from [-1, 1] to [0, 1] below explanation with code needed.
    // 0.159154943 == 1.0 / (2.0 * PI)
    // Original r is "r=(1/pi)*acos(Dz)/sqrt(Dx^2 + Dy^2)"
    // To adjust number's range from [-1.0, 1.0] to [0.0, 1.0] for TexCoord, we need this "[-1.0, 1.0] / 2.0 + 0.5"
    // - This is why we use (1.0 / 2.0 * PI) instead of (1.0 / PI) in "r".
    // - adding 0.5 execute next line. (here : "float u = 0.5 + InDir.x * r" and "float v = 0.5 + InDir.y * r")
    float d = sqrt(InDir.x * InDir.x + InDir.y * InDir.y);
    float r = (d > 0.0) ? (0.159154943 * acos(InDir.z) / d) : 0.0;
    float u = 0.5 + InDir.x * r;
    float v = 0.5 + InDir.y * r;
    return vec2(u, v);
}

vec3 GetNormalFromTexCoord_TwoMirrorBall(vec2 InTexCoord)
{
    // Reverse transform from UV[0, 1] to Direction3D.
    // Explanation of equation. scahp(scahp@naver.com)
    // 1. x = ((u - 0.5) / r) from "float u = 0.5 + x * r;"
    // 2. y = ((v - 0.5) / r) from "float v = 0.5 + y * r;"
    // 3. d = sqrt(((u - 0.5) / r)^2 + ((v - 0.5) / r))
    // 4. z = cos(r * d / 0.159154943)
    //  -> r * d is "sqrt((u - 0.5)^2 + (v - 0.5)^2)"
    // 5. Now we get z from z = cos(sqrt((u - 0.5)^2 + (v - 0.5)^2) / 0.159154943)
    // 6. d is sqrt(1.0 - z^2), exaplanation is below.
    //  - r is length of direction. so r length is 1.0
    //  - so r^2 = (x)^2 + (y)^2 + (z)^2 = 1.0
    //  - r^2 = d^2 + (z)^2 = 1.0
    //  - so, d is sqrt(1.0 - z^2)
    // I substitute sqrt(1.0 - z^2) for d in "z = cos(r * d / 0.159154943)"
    //  - We already know z, so we can get r from "float r = 0.159154943 * acos(z) / sqrt(1.0 - z * z);"
    // 7. Now we can get x, y from "x = ((u - 0.5) / r)" and "y = ((v - 0.5) / r)"

    float u = InTexCoord.x;
    float v = InTexCoord.y;

    float z = cos(sqrt((u - 0.5) * (u - 0.5) + (v - 0.5) * (v - 0.5)) / 0.159154943);
    float r = 0.159154943 * acos(z) / sqrt(1.0 - z * z);
    float x = (u - 0.5) / r;
    float y = (v - 0.5) / r;
    return vec3(x, y, z);
}

void GenerateYlm(out float Ylm[9], vec3 InDir)
{
    Ylm[0] = 0.5 * sqrt(1.0 / PI);
    Ylm[1] = 0.5 * sqrt(3.0 / PI) * InDir.y / SHBasisR;
    Ylm[2] = 0.5 * sqrt(3.0 / PI) * InDir.z / SHBasisR;
    Ylm[3] = 0.5 * sqrt(3.0 / PI) * InDir.x / SHBasisR;
    Ylm[4] = 0.5 * sqrt(15.0 / PI) * InDir.x * InDir.y / (SHBasisR * SHBasisR);
    Ylm[5] = 0.5 * sqrt(15.0 / PI) * InDir.y * InDir.z / (SHBasisR * SHBasisR);
    Ylm[6] = 0.25 * sqrt(5.0 / PI) * (-InDir.x * InDir.x - InDir.y * InDir.y + 2.0 * InDir.z * InDir.z) / (SHBasisR * SHBasisR);
    Ylm[7] = 0.5 * sqrt(15.0 / PI) * InDir.z * InDir.x / (SHBasisR * SHBasisR);
    Ylm[8] = 0.25 * sqrt(15.0 / PI) * (InDir.x * InDir.x - InDir.y * InDir.y) / (SHBasisR * SHBasisR);
}

void main(void)
{
    if (gl_LocalInvocationIndex == 0)		// Execute only first LocalInvocation
        GlobalNumOfSamples = 0;

    barrier();		// Sync for all LocalInvocations

    vec3 LocalLlm[9];
    LocalLlm[0] = vec3(0.0);
    LocalLlm[1] = vec3(0.0);
    LocalLlm[2] = vec3(0.0);
    LocalLlm[3] = vec3(0.0);
    LocalLlm[4] = vec3(0.0);
    LocalLlm[5] = vec3(0.0);
    LocalLlm[6] = vec3(0.0);
    LocalLlm[7] = vec3(0.0);
    LocalLlm[8] = vec3(0.0);

    ivec2 LocalGroupSize = ivec2(SAMPLE_INTERVAL_X, SAMPLE_INTERVAL_Y);
    ivec2 StartInterval = ivec2(gl_LocalInvocationID.xy) + ivec2(1, 1);

    int nrSamples = 0;
    for (float phi = sampleDelta * StartInterval.y; phi <= 2.0 * PI; phi += sampleDelta * LocalGroupSize.y)
    {
        for (float theta = sampleDelta * StartInterval.x; theta <= PI; theta += sampleDelta * LocalGroupSize.x)
        {
            // spherical to cartesian (in tangent space)
            vec3 sampleVec = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)); // tangent space to world
            sampleVec = normalize(sampleVec);

            float Ylm[9];
            GenerateYlm(Ylm, sampleVec);

            vec2 TargetUV = GetSphericalMap_TwoMirrorBall(sampleVec);
            int DataX = int(TargetUV.x * TexWidth);
            int DataY = int((1.0 - TargetUV.y) * TexHeight);
            vec3 curRGB = imageLoad(tex_object, ivec2(DataX, DataY)).xyz;

            // Radiance를 기록하는 것이므로 Alm 은 곱해주지 않음
            LocalLlm[0] += Ylm[0] * curRGB * sin(theta);
            LocalLlm[1] += Ylm[1] * curRGB * sin(theta);
            LocalLlm[2] += Ylm[2] * curRGB * sin(theta);
            LocalLlm[3] += Ylm[3] * curRGB * sin(theta);
            LocalLlm[4] += Ylm[4] * curRGB * sin(theta);
            LocalLlm[5] += Ylm[5] * curRGB * sin(theta);
            LocalLlm[6] += Ylm[6] * curRGB * sin(theta);
            LocalLlm[7] += Ylm[7] * curRGB * sin(theta);
            LocalLlm[8] += Ylm[8] * curRGB * sin(theta);
            nrSamples++;
        }
    }

    uint GlobalIndex = gl_LocalInvocationID.y * SAMPLE_INTERVAL_X + gl_LocalInvocationID.x;

    GlobalLlm[GlobalIndex][0] = LocalLlm[0];
    GlobalLlm[GlobalIndex][1] = LocalLlm[1];
    GlobalLlm[GlobalIndex][2] = LocalLlm[2];
    GlobalLlm[GlobalIndex][3] = LocalLlm[3];
    GlobalLlm[GlobalIndex][4] = LocalLlm[4];
    GlobalLlm[GlobalIndex][5] = LocalLlm[5];
    GlobalLlm[GlobalIndex][6] = LocalLlm[6];
    GlobalLlm[GlobalIndex][7] = LocalLlm[7];
    GlobalLlm[GlobalIndex][8] = LocalLlm[8];

    atomicAdd(GlobalNumOfSamples, nrSamples);		// Accumulate all SampleCounts

    barrier();		// Sync for all LocalInvocations

    if (gl_LocalInvocationIndex == 0)	// Execute only first LocalInvocation
    {
		// All put in togather to finalize computing Llm
        LocalLlm[0] = vec3(0.0);
        LocalLlm[1] = vec3(0.0);
        LocalLlm[2] = vec3(0.0);
        LocalLlm[3] = vec3(0.0);
        LocalLlm[4] = vec3(0.0);
        LocalLlm[5] = vec3(0.0);
        LocalLlm[6] = vec3(0.0);
        LocalLlm[7] = vec3(0.0);
        LocalLlm[8] = vec3(0.0);

        for (int k = 0; k < SAMPLE_INTERVAL_X * SAMPLE_INTERVAL_Y; ++k)
        {
            for (int m = 0; m < 9; ++m)
                LocalLlm[m] += GlobalLlm[k][m];
        }

        for (int i = 0; i < 9; ++i)
        {
            // 반구가 아닌 Sphere 전체에 대한 적분이기 때문에 2.0 * PI 적용
            Llm[i].xyz = 2.0 * PI * LocalLlm[i] * (1.0f / float(GlobalNumOfSamples));
        }
    }
}
