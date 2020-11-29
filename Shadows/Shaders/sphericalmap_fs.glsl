#version 330 core

precision mediump float;

uniform sampler2D tex_object;
uniform vec3 Eye;

in vec3 Pos_;
in vec3 Normal_;

out vec4 color;

void main()
{
    vec3 incidentVector = normalize(Eye - Pos_);
    vec3 normal = normalize(Normal_);
    vec3 R = reflect(incidentVector, normal);

    float m = 2.0 * sqrt(R.x* R.x + R.y * R.y + (R.z + 1.0) * (R.z + 1.0));
    float u = R.x / m + 0.5;
    float v = R.y / m + 0.5;

    color = texture(tex_object, vec2(u, v));
}