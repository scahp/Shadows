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

    color = texture(tex_object, GetSphericalMap(R));
}