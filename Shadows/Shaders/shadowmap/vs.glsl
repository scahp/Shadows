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

// out vec3 ShadowPos_;
// out vec3 ShadowCameraPos_;
out vec3 Pos_;
out vec3 Pos2_;

out vec4 Color_;
out vec3 Normal_;

//#if defined(USE_TEXTURE)
//out vec2 TexCoord_;
//#endif // USE_TEXTURE

#if defined(USE_CSM)
#define NUM_CASCADES 3
uniform mat4 CascadeLightVP[NUM_CASCADES];
out vec3 PosV_;
out vec4 LightSpacePos[NUM_CASCADES];
#endif // USE_CSM

void main()
{
	gl_Position = MVP * vec4(Pos, 1.0);

#if defined(USE_CSM)
	for (int i = 0; i < NUM_CASCADES; i++)
		LightSpacePos[i] = CascadeLightVP[i] * M * vec4(Pos, 1.0);
	PosV_ = TransformPos(MV, Pos);
#endif // USE_CSM

//#if defined(USE_TEXTURE)
//    TexCoord_ = TexCoord;
//#endif // USE_TEXTURE

    Color_ = Color;
    Normal_ = TransformNormal(M, Normal);
    Pos_ = TransformPos(M, Pos);

	vec4 Pos2 =  MVP * vec4(Pos, 1.0);
	Pos2 = Pos2 / Pos2.w;
	Pos2_ = Pos2.xyz;
}