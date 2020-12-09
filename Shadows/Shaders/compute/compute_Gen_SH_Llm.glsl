#version 430 core

precision highp float;

#define SAMPLE_SIZE_X 32
#define SAMPLE_SIZE_Y 32

layout (local_size_x = 1, local_size_y = 1) in;

uniform sampler2D tex_object;		// EnvironmentMap
//layout(binding = 0, rgb10_a2) uniform readonly image2D tex_object;

const float PI = 3.1415926535897932384626433832795028841971693993751058209749;

layout(std430, binding = 0) buffer LlmBuffer
{
	vec3 Llm[];
};

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

vec3 FetchEnvMap(vec2 InTexCoord)
{
    return texture(tex_object, vec2(InTexCoord.x, 1.0 - InTexCoord.y)).xyz;
    //return imageLoad(tex_object, ivec2(InTexCoord.x * 1000, InTexCoord.y * 1000)).xyz;
}

void main(void)
{
    float SHBasisR = 1.0;

    Llm[0] = vec3(0.0);
    Llm[1] = vec3(0.0);
    Llm[2] = vec3(0.0);
    Llm[3] = vec3(0.0);
    Llm[4] = vec3(0.0);
    Llm[5] = vec3(0.0);
    Llm[6] = vec3(0.0);
    Llm[7] = vec3(0.0);
    Llm[8] = vec3(0.0);

    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    vec3 MaxR = vec3(0.0);
    for (float phi = sampleDelta; phi <= 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = sampleDelta; theta <= PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 sampleVec = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)); // tangent space to world
            sampleVec = normalize(sampleVec);
            vec3 curRGB = FetchEnvMap(GetSphericalMap_TwoMirrorBall(sampleVec));

            float LocalYlm[9];
            LocalYlm[0] = 0.5 * sqrt(1.0 / PI);       // 0
            LocalYlm[1] = 0.5 * sqrt(3.0 / PI) * sampleVec.y / SHBasisR;      // 1
            LocalYlm[2] = 0.5 * sqrt(3.0 / PI) * sampleVec.z / SHBasisR;      // 2
            LocalYlm[3] = 0.5 * sqrt(3.0 / PI) * sampleVec.x / SHBasisR;      // 3
            LocalYlm[4] = 0.5 * sqrt(15.0 / PI) * sampleVec.x * sampleVec.y / (SHBasisR * SHBasisR);      // 4
            LocalYlm[5] = 0.5 * sqrt(15.0 / PI) * sampleVec.y * sampleVec.z / (SHBasisR * SHBasisR);      // 5
            LocalYlm[6] = 0.25 * sqrt(5.0 / PI) * (-sampleVec.x * sampleVec.x - sampleVec.y * sampleVec.y + 2.0 * sampleVec.z * sampleVec.z) / (SHBasisR * SHBasisR);     // 6
            LocalYlm[7] = 0.5 * sqrt(15.0 / PI) * sampleVec.z * sampleVec.x / (SHBasisR * SHBasisR);  // 7
            LocalYlm[8] = 0.25 * sqrt(15.0 / PI) * (sampleVec.x * sampleVec.x - sampleVec.y * sampleVec.y) / (SHBasisR * SHBasisR); // 8

            Llm[0] += LocalYlm[0] * curRGB * cos(theta) + sin(theta);
            Llm[1] += LocalYlm[1] * curRGB * cos(theta) + sin(theta);
            Llm[2] += LocalYlm[2] * curRGB * cos(theta) + sin(theta);
            Llm[3] += LocalYlm[3] * curRGB * cos(theta) + sin(theta);
            Llm[4] += LocalYlm[4] * curRGB * cos(theta) + sin(theta);
            Llm[5] += LocalYlm[5] * curRGB * cos(theta) + sin(theta);
            Llm[6] += LocalYlm[6] * curRGB * cos(theta) + sin(theta);
            Llm[7] += LocalYlm[7] * curRGB * cos(theta) + sin(theta);
            Llm[8] += LocalYlm[8] * curRGB * cos(theta) + sin(theta);

            ++nrSamples;
        }
    }

    for (int i = 0; i < 9; ++i)
        Llm[i] = PI * (Llm[i] * (1.0 / float(nrSamples)));
}
