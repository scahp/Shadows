#version 330 core

#include "common.glsl"

precision mediump float;

#define MAX_NUM_OF_POINT_LIGHT 100
#define MAX_NUM_OF_SPOT_LIGHT 100

uniform int NumOfPointLight;
uniform int NumOfSpotLight;

layout (std140) uniform PointLightBlock
{
	jPointLight PointLight[MAX_NUM_OF_POINT_LIGHT];
};

layout (std140) uniform SpotLightBlock
{
	jSpotLight SpotLight[MAX_NUM_OF_SPOT_LIGHT];
};

layout (std140) uniform PointLightShadowMapBlock
{
	float PointLightZNear;
	float PointLightZFar;
};

layout (std140) uniform SpotLightShadowMapBlock
{
	float SpotLightZNear;
	float SpotLightZFar;
};

uniform float PointLightESM_C;
uniform float SpotLightESM_C;

in vec4 fragPos_;

void main()
{
    if (NumOfPointLight > 0)
    {
        vec3 lightDir = fragPos_.xyz - PointLight[0].LightPos;
        float distSquared = dot(lightDir.xyz, lightDir.xyz);
        float distFromLight = (sqrt(distSquared) - PointLightZNear) / (PointLightZFar - PointLightZNear);
        float ez = exp(distFromLight * PointLightESM_C);

        gl_FragData[0].x = ez * ez;
        gl_FragData[0].y = ez;
        gl_FragData[0].w = 1.0;
    }
    else if (NumOfSpotLight > 0)
    {
        vec3 lightDir = fragPos_.xyz - SpotLight[0].LightPos;
        float distSquared = dot(lightDir.xyz, lightDir.xyz);
        float distFromLight = (sqrt(distSquared) - SpotLightZNear) / (SpotLightZFar - SpotLightZNear);
        float ez = exp(distFromLight * PointLightESM_C);

        gl_FragData[0].x = ez * ez;
        gl_FragData[0].y = ez;
        gl_FragData[0].w = 1.0;
    }
}