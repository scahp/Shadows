#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 TexCoord;

uniform mat4 M;
uniform mat4 MVP;

out vec3 Pos_;
out vec2 TexCoord_;

vec3 TransformPos(mat4 m, vec3 v)
{
    return (m * vec4(v, 1.0)).xyz;
}

void main()
{
    TexCoord_ = TexCoord;
    Pos_ = TransformPos(M, Pos);
    //gl_Position = MVP * vec4(Pos, 1.0);

    gl_Position.xy = TexCoord_.xy * 2.0 - 1.0;
    gl_Position.y *= -1.0;
    gl_Position.z = 0.5;
    gl_Position.w = 1.0;
}
