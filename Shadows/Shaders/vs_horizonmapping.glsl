#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec4 Color;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;

uniform mat4 M;
uniform mat4 MVP;
uniform vec3 EyeWorldPos;
uniform vec3 LightDirection;		// from light to location

out vec2 TexCoord_;
out vec4 Color_;
out mat3 TBN;
out vec3 WorldSpaceViewDir;

void main()
{
    TexCoord_ = (Pos.xz + 0.5);
	Color_ = Color;
	gl_Position = MVP * vec4(Pos, 1.0);
	vec3 WorldPos = (M * vec4(Pos, 1.0)).xyz;
	WorldSpaceViewDir = normalize(EyeWorldPos - WorldPos);

	vec3 T = normalize(vec3(M * vec4(Tangent, 0.0)));
	vec3 B = normalize(vec3(M * vec4(cross(Normal, Tangent), 0.0)));
	vec3 N = normalize(vec3(M * vec4(Normal, 0.0)));

	TBN = mat3(T, B, N);
}
