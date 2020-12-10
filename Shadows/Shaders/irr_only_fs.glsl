#version 330 core

precision mediump float;

const float PI = 3.1415926535897932384626433832795028841971693993751058209749;
const float SHBasisR = 1.0;

//uniform samplerCube tex_object;
uniform sampler2D tex_object;       // EnvMap - TwoMirrorBall SphereMap

uniform float Al[3];
uniform vec3 Llm[9];
uniform int IsIrranceMap;

out vec4 color;

in vec3 Normal_;

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

vec3 FetchEnvMap(vec2 InTexCoord)
{
    return texture(tex_object, vec2(InTexCoord.x, 1.0 - InTexCoord.y)).xyz;
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

void main()
{
    vec3 normal = normalize(Normal_);

    if (IsIrranceMap != 0)
    {
        float Ylm[9];
        GenerateYlm(Ylm, normal);

        float AlYlm[9] = float[9](
            Al[0] * Ylm[0],
            Al[1] * Ylm[1], Al[1] * Ylm[2], Al[1] * Ylm[3],
            Al[2] * Ylm[4], Al[2] * Ylm[5], Al[2] * Ylm[6], Al[2] * Ylm[7], Al[2] * Ylm[8]);

        color = vec4(SHReconstruction(AlYlm, Llm), 1.0);
    }
    else
    {
        vec2 uv = GetSphericalMap_TwoMirrorBall(normal);
        color = vec4(FetchEnvMap(uv), 1.0);
    }
}