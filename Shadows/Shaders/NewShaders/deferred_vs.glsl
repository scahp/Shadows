#version 330 core

#preprocessor

#include "common.glsl"

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec4 Color;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
layout(location = 4) in vec3 Bitangent;

#if defined(USE_TEXTURE)
layout(location = 5) in vec2 TexCoord;
#endif // USE_TEXTURE

uniform mat4 MVP;
uniform mat4 MV;
uniform mat4 M;

out vec3 Pos_;
out vec3 PosInView_;
out vec4 Color_;
out vec3 Normal_;
out mat3 TBN_;
out mat3 TBNInView_;

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
    PosInView_ = TransformPos(MV, Pos);

    {
        vec3 T = normalize(vec3(M * vec4(Tangent, 0.0)));
        vec3 B = normalize(vec3(M * vec4(Bitangent, 0.0)));
        vec3 N = normalize(vec3(M * vec4(Normal, 0.0)));
        TBN_ = mat3(T, B, N);
    }

    {
        vec3 T = normalize(vec3(MV * vec4(Tangent, 0.0)));
        vec3 B = normalize(vec3(MV * vec4(Bitangent, 0.0)));
        vec3 N = normalize(vec3(MV * vec4(Normal, 0.0)));
        TBNInView_ = mat3(T, B, N);
    }

    gl_Position = MVP * vec4(Pos, 1.0);
}