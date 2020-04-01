#version 430 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec4 Color;

uniform mat4 M;
uniform mat4 MVP;

out vec3 WorldPos_;
out vec4 Color_;
out vec4 ClipSpacePos_;

void main()
{
    Color_ = Color;
    WorldPos_ = (M * vec4(Pos, 1.0)).xyz;
    gl_Position = MVP * vec4(Pos, 1.0);
    
    ClipSpacePos_ = gl_Position;
    //ClipSpacePos_.xy = (ClipSpacePos_.xy + vec2(ClipSpacePos_.w)) * 0.5;
    //ClipSpacePos_ = gl_Position.xy / gl_Position.w;
    //ClipSpacePos_.xy = ClipSpacePos_.xy * vec2(0.5) + vec2(0.5);
    //ClipSpacePos_ = (gl_Position.xy + 0.5) * 0.5;
    //ClipSpacePos_.xy /= gl_Position.w;
}