#version 430 core

#include "common.glsl"

precision mediump float;

layout(location = 0) in vec4 Pos;
layout(location = 1) in vec4 Color;

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1
#define MAX_NUM_OF_POINT_LIGHT 100
#define MAX_NUM_OF_SPOT_LIGHT 100

uniform int NumOfDirectionalLight;
uniform int NumOfPointLight;
uniform int NumOfSpotLight;

layout (std140) uniform DirectionalLightBlock
{
	jDirectionalLight DirectionalLight[MAX_NUM_OF_DIRECTIONAL_LIGHT];
};

layout (std140) uniform PointLightBlock
{
	jPointLight PointLight[MAX_NUM_OF_POINT_LIGHT];
};

layout (std140) uniform SpotLightBlock
{
	jSpotLight SpotLight[MAX_NUM_OF_SPOT_LIGHT];
};

uniform mat4 M;
uniform mat4 VP;
uniform mat4 MVP;

out vec4 Color_;
out vec3 LightDirWS_;

void main()
{
    Color_ = Color;

	vec4 PosTemp = M * vec4(Pos.xyz, 1.0);
	PosTemp.xyz /= PosTemp.w;

	vec3 lightPos;
	if (NumOfPointLight > 0)
	{
		lightPos = PointLight[0].LightPos;
		LightDirWS_ = normalize(PosTemp.xyz - lightPos);
	}
	else if (NumOfSpotLight > 0)
	{
		lightPos = SpotLight[0].LightPos;
		LightDirWS_ = normalize(PosTemp.xyz - lightPos);
	}
	else if (NumOfDirectionalLight > 0)
	{
		LightDirWS_ = DirectionalLight[0].LightDirection;
	}

	{
		gl_Position = PosTemp;
	}
}
