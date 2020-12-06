#version 330 core

precision mediump float;

uniform sampler2D tex_object;
uniform vec3 Eye;

in vec3 Pos_;
in vec3 Normal_;

out vec4 color;

vec2 GetSphericalMap(vec3 InDir)
{
    float m = 2.0 * sqrt(InDir.x * InDir.x + InDir.y * InDir.y + (InDir.z + 1.0) * (InDir.z + 1.0));
    float u = InDir.x / m + 0.5;
    float v = InDir.y / m + 0.5;
    return vec2(u, v);

    //const vec2 invAtan = vec2(0.1591, 0.3183);
    //vec2 uv = vec2(atan(InDir.z, InDir.x), asin(InDir.y));
    //uv *= invAtan;
    //uv += 0.5;
    //return uv;
}

void main()
{
    vec3 incidentVector = normalize(Eye - Pos_);
    vec3 normal = normalize(Normal_);
    vec3 R = reflect(incidentVector, normal);

    bool isUsingTwoMirrorBall = true;

    if (isUsingTwoMirrorBall)
    {
        float Dx = R.x;
        float Dy = R.y;
        float Dz = R.z;

        float d = sqrt(Dx * Dx + Dy * Dy);
        //r = if (d, 0.159154943 * acos(Dz) / d, 0);
        float r = 0.159154943 * acos(Dz) / d;
        float u = 0.5 + Dx * r;
        float v = 0.5 + Dy * r;
        color = texture(tex_object, vec2(u, v));
    }
    else
    {
        float m = 2.0 * sqrt(R.x * R.x + R.y * R.y + (R.z + 1.0) * (R.z + 1.0));
        float u = R.x / m + 0.5;
        float v = R.y / m + 0.5;

        color = texture(tex_object, vec2(u, v));
    }
    color.xyz = pow(color.xyz, vec3(1.0 / 2.2));
}