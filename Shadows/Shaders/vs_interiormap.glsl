#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec4 Color;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
layout(location = 4) in vec2 TexCoord;

uniform mat4 M;
uniform mat4 MVP;

out vec2 TexCoord_;
out vec4 Color_;
out vec3 LocalPos_;
out mat3 TBN_;
out vec3 WorldPos_;

void main()
{
    vec3 T = normalize(vec3(M * vec4(Tangent, 0.0)));
    vec3 B = normalize(vec3(M * vec4(cross(Normal, Tangent), 0.0)));
    vec3 N = normalize(vec3(M * vec4(Normal, 0.0)));
	TBN_ = transpose(mat3(T, B, N));
	
	vec4 wpos = M * vec4(Pos, 1.0);
	wpos /= wpos.w;
	WorldPos_ = wpos.xyz;

	LocalPos_ = Pos.xyz;
    //TexCoord_ = (Pos.xz + 0.5);
	TexCoord_ = TexCoord;
	Color_ = Color;
	gl_Position = MVP * vec4(Pos, 1.0);
}
