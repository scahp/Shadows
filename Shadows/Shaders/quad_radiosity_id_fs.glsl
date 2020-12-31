#version 430 core

precision mediump float;

uniform int ID;
uniform vec3 Normal;
uniform vec3 CameraPos;

in vec3 Pos_;

out vec4 color;

void main()
{
    vec3 WorldLookAt = normalize(Pos_ - CameraPos);
    if (dot(WorldLookAt, -Normal) < 0.0)
        discard;

    color = vec4(float(ID));
}