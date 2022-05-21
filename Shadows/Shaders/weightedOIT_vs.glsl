#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec4 Color;

uniform mat4 MVP;
uniform mat4 MV;

out vec4 Color_;
out float LinearZ;

void main()
{
    Color_ = Color;
    gl_Position = MVP * vec4(Pos, 1.0);
    
    vec4 PosCamera = MV * vec4(Pos, 1.0);
    PosCamera = PosCamera / PosCamera.w;
    LinearZ = PosCamera.z / (500.0f - 0.1f);
}