#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec4 Color;
layout(location = 2) in vec3 Normal;
layout(location = 3) in vec3 Tangent;
layout(location = 4) in vec2 TexCoord;

uniform mat4 M;
uniform mat4 InvM;
uniform mat4 MVP;
uniform vec3 WorldSpace_CameraPos;
uniform vec3 WorldSpace_LightDir_ToSurface;
uniform vec3 LocalSpace_LightDir_ToSurface;

out vec2 TexCoord_;
out vec4 Color_;
out mat3 TBN_;
//out vec3 TangentSpace_ViewDir_ToSurface_;
out vec3 TangentSpace_LightDir_ToSurface_;
out vec3 WorldPos_;
out vec3 LocalPos_;

void main()
{
    TexCoord_ = TexCoord;
	Color_ = Color;
	gl_Position = MVP * vec4(Pos, 1.0);

	#define WORLD_SPACE 1
#if WORLD_SPACE
    vec3 T = normalize(vec3(M * vec4(Tangent, 0.0)));
    vec3 B = normalize(vec3(M * vec4(cross(Normal, Tangent), 0.0)));
    vec3 N = normalize(vec3(M * vec4(Normal, 0.0)));
#else
    vec3 T = normalize(Tangent);
    vec3 B = normalize(cross(Normal, Tangent));
    vec3 N = normalize(Normal);
#endif
	TBN_ = transpose(mat3(T, B, N));

	vec4 wpos = M * vec4(Pos, 1.0);
	wpos /= wpos.w;

	WorldPos_ = wpos.xyz;
    LocalPos_ = Pos;
	//TangentSpace_ViewDir_ToSurface_ = TBN_ * normalize(wpos.xyz - WorldSpace_CameraPos);

#if WORLD_SPACE
	TangentSpace_LightDir_ToSurface_ = TBN_ * normalize(WorldSpace_LightDir_ToSurface);
#else		
    //vec4 LocalDir = InvM * vec4(normalize(WorldSpace_LightDir_ToSurface), 0.0);
    //TangentSpace_LightDir_ToSurface_ = TBN_ * LocalDir.xyz;
    //TangentSpace_LightDir_ToSurface_ = TBN_ * LocalSpace_LightDir_ToSurface;
#endif
}
