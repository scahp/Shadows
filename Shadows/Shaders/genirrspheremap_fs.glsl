#version 330 core

precision highp float;

uniform sampler2D tex_object;
uniform samplerCube tex_object2;
uniform float Al[3];
uniform vec3 Llm[9];
uniform int IsGenIrradianceMapFromSH;
uniform int IsGenerateLlmRealtime;

in vec2 TexCoord_;

out vec4 color;

const float PI = 3.1415926535897932384626433832795028841971693993751058209749;
const float SHBasisR = 1.0;
const float sampleDelta = 0.025;

vec2 GetSphericalMap(vec3 InDir)
{
    float temp = InDir.x * InDir.x + InDir.y * InDir.y + (InDir.z + 1.0) * (InDir.z + 1.0);
    if (temp > 0.0)
    {
        float m = 2.0 * sqrt(temp);
        float u = InDir.x / m + 0.5;
        float v = InDir.y / m + 0.5;
        return vec2(u, v);
    }
    return vec2(0.5);
}

vec3 GetNormalFromTexCoord(vec2 InTexCoord)
{
    vec3 result;

    // 바꿈
    float t = InTexCoord.x;
    float s = InTexCoord.y;

    result.x = 2.0 * sqrt(-4.0 * s * s + 4.0 * s - 1.0 - 4.0 * t * t + 4.0 * t) * (2.0 * t - 1);
    result.y = 2.0 * sqrt(-4.0 * s * s + 4.0 * s - 1.0 - 4.0 * t * t + 4.0 * t) * (2.0 * s - 1);
    result.z = (-8.0 * s * s + 8.0 * s - 8.0 * t * t + 8.0 * t - 3.0);
    return (result);
}

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
    //color = texture(tex_object, vec2(u, v));
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
}

vec3 SHReconstruction(float InAlYlm[9], vec3 InLlm[9])
{
    /* Optimized code in paper
    // All constant put in c[0~5].
    float c[5];
    c[0] = 0.429043;
    c[1] = 0.511664;
    c[2] = 0.743125;
    c[3] = 0.886227;
    c[4] = 0.247708;

    return c[0] * InLlm[8] * (x * x - y * y) + c[2] * InLlm[6] * z * z + c[3] * InLlm[0] - c[4] * InLlm[6]
        + 2.0 * c[0] * (InLlm[4] * x * y + InLlm[7] * x * z + InLlm[5] * y * z)
        + 2.0 * c[1] * (InLlm[3] * x + InLlm[1] * y + InLlm[2] * z);
    */

    vec3 result = vec3(0.0);
    for (int i = 0; i < 9; ++i)
        result += InAlYlm[i] * InLlm[i];
    return result;
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

void GenerateLlm(out vec3 Llm[9])
{
    Llm[0] = vec3(0.0);
    Llm[1] = vec3(0.0);
    Llm[2] = vec3(0.0);
    Llm[3] = vec3(0.0);
    Llm[4] = vec3(0.0);
    Llm[5] = vec3(0.0);
    Llm[6] = vec3(0.0);
    Llm[7] = vec3(0.0);
    Llm[8] = vec3(0.0);

    // 개선이 필요함.
    // 현재는 픽셀마다 Llm 9개를 모두 구해서 계산하는 방식임
    float nrSamples = 0.0;
    for (float phi = sampleDelta; phi <= 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = sampleDelta; theta <= PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 sampleVec = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)); // tangent space to world
            sampleVec = normalize(sampleVec);
            vec3 curRGB = FetchEnvMap(GetSphericalMap_TwoMirrorBall(sampleVec));

            float Ylm[9];
            GenerateYlm(Ylm, sampleVec);

            Llm[0] += Ylm[0] * curRGB * sin(theta);
            Llm[1] += Ylm[1] * curRGB * sin(theta);
            Llm[2] += Ylm[2] * curRGB * sin(theta);
            Llm[3] += Ylm[3] * curRGB * sin(theta);
            Llm[4] += Ylm[4] * curRGB * sin(theta);
            Llm[5] += Ylm[5] * curRGB * sin(theta);
            Llm[6] += Ylm[6] * curRGB * sin(theta);
            Llm[7] += Ylm[7] * curRGB * sin(theta);
            Llm[8] += Ylm[8] * curRGB * sin(theta);

            ++nrSamples;
        }
    }
    for (int i = 0; i < 9; ++i)
        Llm[i] = 2.0 * PI * (Llm[i] * (1.0 / float(nrSamples)));
}

vec3 GenerateIrradiance(vec3 InNormal)
{
    vec3 up = normalize(vec3(0.0, 1.0, 0.0));
    if (abs(dot(InNormal, up)) > 0.99)
        up = vec3(0.0, 0.0, 1.0);
    vec3 right = normalize(cross(up, InNormal));
    up = normalize(cross(InNormal, right));

    float nrSamples = 0.0;

    vec3 irradiance = vec3(0.0, 0.0, 0.0);
    for (float phi = sampleDelta; phi <= 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = sampleDelta; theta <= 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)); // tangent space to world
            vec3 sampleVec =
                tangentSample.x * right +
                tangentSample.y * up +
                tangentSample.z * InNormal;

            sampleVec = normalize(sampleVec);
            vec3 curRGB = FetchEnvMap(GetSphericalMap_TwoMirrorBall(sampleVec));

            irradiance += curRGB * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * (irradiance * (1.0 / float(nrSamples)));
    return irradiance;
}

void main()
{
    if (length(TexCoord_ - vec2(0.5)) > 0.5)
    {
        color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 normal = GetNormalFromTexCoord_TwoMirrorBall(TexCoord_);

    // SH로 부터 Irradiance를 생성할 것인지 여부
    if (IsGenIrradianceMapFromSH > 0)
    {
        float Ylm[9];
        GenerateYlm(Ylm, normal);

        float AlYlm[9] = float[9](
            Al[0] * Ylm[0],
            Al[1] * Ylm[1], Al[1] * Ylm[2], Al[1] * Ylm[3],
            Al[2] * Ylm[4], Al[2] * Ylm[5], Al[2] * Ylm[6], Al[2] * Ylm[7], Al[2] * Ylm[8]);

        // 실시간으로 생성한 Llm 을 사용할 것인지? (현재는 픽셀별로 Llm을 따로 만들어서 아주 느림, 외부에서 실시간으로 생성해서 넣도록 하자.)
        if (IsGenerateLlmRealtime > 0)
        {
            vec3 LocalLlm[9];
            GenerateLlm(LocalLlm);
            color.xyz = SHReconstruction(AlYlm, LocalLlm);
        }
        else
        {
            color.xyz = SHReconstruction(AlYlm, Llm);
        }
    }
    else
    {
        color.xyz = GenerateIrradiance(normal);     // 현재 픽셀에 대한 Irradiance를 Brute force 방식으로 생성
    }
    color.w = 1.0;
    color.xyz *= exp2(-1.0);    // exposure x stop
}
