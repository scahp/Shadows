#version 330 core

#preprocessor

#include "shadow.glsl"

precision highp float;
precision highp sampler2DArray;

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1

uniform sampler2DShadow shadow_object; 
uniform int NumOfDirectionalLight;

layout (std140) uniform DirectionalLightBlock
{
	jDirectionalLight DirectionalLight[MAX_NUM_OF_DIRECTIONAL_LIGHT];
};

layout (std140) uniform DirectionalLightShadowMapBlock
{
	mat4 ShadowVP;
	mat4 ShadowV;
	vec3 LightPos;      // Directional Light Pos юс╫ц
	float LightZNear;
	float LightZFar;
	vec2 ShadowMapSize;
};

uniform vec3 Eye;
uniform int Collided;
uniform int ShadowOn;
uniform mat4 PSM;

in vec3 Pos_;
in vec4 Color_;
in vec3 Normal_;

out vec4 color;

#define PSM_SHADOW_BIAS_DIRECTIONAL 0.0
float IsPSMShadowing(vec3 lightClipPos, sampler2DShadow shadow_object)
{
    if (IsInShadowMapSpace(lightClipPos))
        return texture(shadow_object, vec3(lightClipPos.xy, lightClipPos.z - SHADOW_BIAS_DIRECTIONAL));

    return 1.0;
}


void main()
{
    vec3 normal = normalize(Normal_);
    vec3 viewDir = normalize(Eye - Pos_);

    vec4 tempShadowPos = (PSM * vec4(Pos_, 1.0));
    tempShadowPos /= tempShadowPos.w;
    vec3 ShadowPos = tempShadowPos.xyz * 0.5 + 0.5;        // Transform NDC space coordinate from [-1.0 ~ 1.0] into [0.0 ~ 1.0].

    vec4 shadowCameraPos = (ShadowV * vec4(Pos_, 1.0));    

    bool shadow = false;
    vec3 directColor = vec3(0.0, 0.0, 0.0);

    for(int i=0;i<MAX_NUM_OF_DIRECTIONAL_LIGHT;++i)
    {
        if (i >= NumOfDirectionalLight)
            break;

        float lit = 1.0;

		if (ShadowOn > 0)
		{
            lit = IsPSMShadowing(ShadowPos, shadow_object);
		}

        if (lit > 0.0)
        {
            jDirectionalLight light = DirectionalLight[i];
			directColor += GetDirectionalLight(light, normal, viewDir) * lit;
        }
    }

	directColor *= 1.0 / 3.14;

    color = vec4(directColor + vec3(0.1), 1.0);
}