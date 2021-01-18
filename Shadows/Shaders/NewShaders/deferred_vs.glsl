#version 330 core

#preprocessor

#include "common.glsl"

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec4 Color;
layout(location = 2) in vec3 Normal;

#if defined(USE_TEXTURE)
layout(location = 3) in vec2 TexCoord;
#endif // USE_TEXTURE

uniform mat4 MVP;
uniform mat4 MV;
uniform mat4 M;

out vec3 Pos_;
out vec4 Color_;
out vec3 Normal_;

#if defined(USE_TEXTURE)
out vec2 TexCoord_;
#endif // USE_TEXTURE

void main()
{
#if defined(USE_TEXTURE)
    TexCoord_ = TexCoord;
#endif // USE_TEXTURE

    Color_ = Color;
    Normal_ = TransformNormal(M, Normal);
    Pos_ = TransformPos(M, Pos);

    gl_Position = MVP * vec4(Pos, 1.0);
}