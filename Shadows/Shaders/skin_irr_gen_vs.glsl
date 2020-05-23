#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec2 TexCoord;

uniform mat4 M;
uniform mat4 MVP;

out vec3 Pos_;
out vec2 TexCoord_;
out vec4 VPos_;

layout(std140) uniform DirectionalLightShadowMapBlock
{
    mat4 ShadowVP;
    mat4 ShadowV;
    vec3 LightPos;      // Directional Light Pos 임시
    float LightZNear;
    float LightZFar;
    vec2 ShadowMapSize;
};

vec3 TransformPos(mat4 m, vec3 v)
{
    return (m * vec4(v, 1.0)).xyz;
}

void main()
{
    TexCoord_ = TexCoord;
    Pos_ = TransformPos(M, Pos);
    VPos_ = ShadowV * M * vec4(Pos, 1.0);

    gl_Position.xy = TexCoord_.xy * 2.0 - 1.0;
    gl_Position.y *= -1.0;
    gl_Position.z = 0.5;
    gl_Position.w = 1.0;
}
