#version 330 core

precision highp float;

uniform sampler2D tex_object;
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

    //if (length(tex - vec2(0.5)) > 0.5)
    //{
    //    color = vec4(0.0, 0.0, 0.0, 1.0);
    //    return;
    //}

    //color = vec4(0.0, 0.0, 0.0, 1.0);

    //color.xy = tex;
    ////return;

    color = texture(tex_object, tex);
    color.xyz = pow(color.xyz, vec3(1.0/2.2));
    //return;

    //color = vec4(GetNormalFromTexCoord(TexCoord_), 1.0);
    vec3 normal = GetNormalFromTexCoord(tex);

    //color.xy = GetSphericalMap(normal);
    //color.z = 0.0;
    //color.w = 1.0;
    //return;

    //if (dot(normal, vec3(0.0, 0.0, 1.0)) <= 0)
    //{
    //    color = vec4(1.0, 0.0, 0.0, 1.0);
    //    return;
    //}
    //return;

    //color.xyz = normal.zzz * 0.5 + vec3(0.5);
    //color.w = 1.0;
    //return;

    vec3 irradiance = vec3(0.0, 0.0, 0.0);
    vec3 up = normalize(vec3(0.0, 1.0, 0.0));
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));
    float sampleDelta = 0.025;
    float nrSamples = 0.0;

    for (float p = 0.0; p < 2.0 * PI; p += sampleDelta)
    {
        for (float t = 0.0; t < 0.5 * PI; t += sampleDelta)
        {
            // spherical to cartesian (in tangent space)
            vec3 tangentSample = vec3(sin(t) * cos(p), sin(t) * sin(p), cos(t)); // tangent space to world
            vec3 sampleVec = 
                tangentSample.x * right + 
                tangentSample.y * up + 
                tangentSample.z * normal;

            sampleVec = normalize(sampleVec);
            vec3 curRGB = texture(tex_object, GetSphericalMap(sampleVec)).rgb;
            irradiance += curRGB * cos(t) * sin(t);
            nrSamples++;

            //color.xyz = curRGB;
            //color.w = 1.0;
            //return;
        }
    }
    irradiance = PI * irradiance * (1.0 / float(nrSamples));
    color = vec4(irradiance, 1.0);
    //color.xyz = pow(color.xyz, vec3(1.0 / 2.2));
}
