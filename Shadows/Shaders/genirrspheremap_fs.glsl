#version 330 core

precision highp float;

uniform sampler2D tex_object;
uniform samplerCube tex_object2;
in vec2 TexCoord_;

out vec4 color;

const float PI = 3.1415926535897932384626433832795028841971693993751058209749;

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
    //const vec2 invAtan = vec2(0.1591, 0.3183);
    //vec2 uv = vec2(atan(InDir.z, InDir.x), asin(InDir.y));
    //uv *= invAtan;
    //uv += 0.5;
    //return uv;
}

vec3 GetNormalFromTexCoord(vec2 InTexCoord)
{
    vec3 result;

    // ¹Ù²Þ
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

uniform float theta;
uniform float phi;

void main()
{
    vec2 tex = TexCoord_;
    tex.y = 1.0 - tex.y;

    //if (tex.x > 0.5)
    //{
    //    color = vec4(0.0, 0.0, 0.0, 1.0);
    //    return;
    //}

    //color = texture(tex_object, tex);
    //color.xyz = pow(color.xyz, vec3(1.0 / 2.2));
    //return;

    if (length(tex - vec2(0.5)) > 0.5)
    {
        color = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 normal = GetNormalFromTexCoord_TwoMirrorBall(tex);
    //color = vec4(normal, 1.0);
    //return;

    vec3 irradiance = vec3(0.0, 0.0, 0.0);
    vec3 up = normalize(vec3(0.0, 1.0, 0.0));
    if (abs(dot(normal, up)) > 0.99)
        up = vec3(0.0, 0.0, 1.0);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));
    float sampleDelta = 0.025;
    float nrSamples = 0.0;

    for (float phi = sampleDelta; phi <= 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = sampleDelta; theta <= 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)); // tangent space to world
            vec3 sampleVec = 
                tangentSample.x * right + 
                tangentSample.y * up + 
                tangentSample.z * normal;

            sampleVec = normalize(sampleVec);
            vec3 curRGB = texture(tex_object, GetSphericalMap_TwoMirrorBall(sampleVec)).rgb;
            //curRGB = pow(curRGB, vec3(1.0 / 2.5));

            irradiance += curRGB * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * (irradiance * (1.0 / float(nrSamples)));
    color = vec4(irradiance, 1.0);
    //color.xyz = pow(color.xyz, vec3(1.0 / 2.2));
    color.xyz *= exp2(-1.0);    // exposure x stop
}
