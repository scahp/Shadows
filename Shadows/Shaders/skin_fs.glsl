#version 330 core

precision mediump float;

struct jAmbientLight
{
    vec3 Color;
    vec3 Intensity;
};

struct jDirectionalLight
{
    vec3 LightDirection;
    float SpecularPow;
    vec3 Color;
    vec3 DiffuseLightIntensity;
    vec3 SpecularLightIntensity;
};

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1
#define MAX_NUM_OF_POINT_LIGHT 1
#define MAX_NUM_OF_SPOT_LIGHT 1

uniform sampler2D tex_object2;
uniform sampler2D tex_object3;
uniform int UseTexture;
uniform vec3 Eye;

in vec2 TexCoord_;
in vec3 Pos_;
out vec4 color;

uniform int NumOfDirectionalLight;
uniform jAmbientLight AmbientLight;

layout(std140) uniform DirectionalLightBlock
{
	jDirectionalLight DirectionalLight[MAX_NUM_OF_DIRECTIONAL_LIGHT];
};

void main()
{
	vec3 viewDir = normalize(Eye - Pos_);
	vec3 normal = texture2D(tex_object3, TexCoord_).xyz;

	jDirectionalLight light = DirectionalLight[0];

	color = vec4(light.Color * clamp(dot(-light.LightDirection, normal), 0.0, 1.0) * light.DiffuseLightIntensity, 1.0);
	color *= texture2D(tex_object2, TexCoord_);
}