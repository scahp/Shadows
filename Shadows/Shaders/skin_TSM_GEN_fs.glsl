#version 330 core

precision highp float;

struct jDirectionalLight
{
    vec3 LightDirection;
    float SpecularPow;
    vec3 Color;
    vec3 DiffuseLightIntensity;
    vec3 SpecularLightIntensity;
};

layout(std140) uniform DirectionalLightShadowMapBlock
{
    mat4 ShadowVP;
    mat4 ShadowV;
    vec3 LightPos;      // Directional Light Pos юс╫ц
    float LightZNear;
    float LightZFar;
    vec2 ShadowMapSize;
};

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1

uniform sampler2D tex_object2;      // Albedo

in vec2 TexCoord_;
in vec3 Pos_;
out vec4 color;

layout(std140) uniform DirectionalLightBlock
{
    jDirectionalLight DirectionalLight[MAX_NUM_OF_DIRECTIONAL_LIGHT];
};

void main()
{
    jDirectionalLight light = DirectionalLight[0];
    vec3 LightPos = -light.LightDirection * 500;
    float DistToLight = length(LightPos - Pos_);

    color.x = DistToLight;
    color.yz = TexCoord_;
    color.w = 1.0;

    //color.xyz = texture2D(tex_object2, TexCoord_).xyz;
}
