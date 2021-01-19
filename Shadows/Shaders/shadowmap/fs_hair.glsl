#version 430 core

#preprocessor

#include "common.glsl"

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1

layout (std140) uniform DirectionalLightBlock
{
	jDirectionalLight DirectionalLight[MAX_NUM_OF_DIRECTIONAL_LIGHT];
};

uniform int NumOfDirectionalLight;

layout (std140) uniform DirectionalLightShadowMapBlock
{
	mat4 ShadowVP;
	mat4 ShadowV;
	vec3 LightPos;      // Directional Light Pos temp
	float LightZNear;
	float LightZFar;
};

#if defined(USE_MATERIAL)
uniform int UseMaterial;
uniform jMaterial Material;
#endif // USE_MATERIAL

uniform vec3 Eye;
uniform vec4 Color;
uniform int ShadowOn;
uniform int ShadowMapWidth;
uniform int ShadowMapHeight;

#if defined(USE_TEXTURE)
uniform int UseTexture;
uniform sampler2D tex_object2;
uniform int TextureSRGB[1];
in vec2 TexCoord_;
#endif // USE_TEXTURE

in vec3 Pos_;
in vec4 Color_;
in vec3 Normal_;

float KajiyaKayShadingModelTest(vec3 tangent, vec3 toLight, vec3 toView)
{
	float dotTL = dot(tangent, normalize(toLight));
	float dotTV = dot(-tangent, toView);

	float sinTL = sqrt(1 - dotTL * dotTL);
	float sinTV = sqrt(1 - dotTV * dotTV);

	float diffuse = clamp(sinTL, 0.0, 1.0);
	float specular = clamp(pow(dotTL*dotTV + sinTL*sinTV, 6.0), 0.0, 1.0);
	return (diffuse + specular);
}

vec3 KajiyaKayShadingModel(vec3 tangent, vec3 toLight, vec3 toView, vec3 diffuseColor, vec3 specularColor, float specularPow)
{
	float dotTL = dot(tangent, normalize(toLight));
	float dotTV = dot(-tangent, toView);

	float sinTL = sqrt(1 - dotTL * dotTL);
	float sinTV = sqrt(1 - dotTV * dotTV);

	float diffuse = clamp(sinTL, 0.0, 1.0);
	float specular = clamp(pow(dotTL*dotTV + sinTL*sinTV, specularPow), 0.0, 1.0);
	return (diffuseColor * diffuse + specularColor * specular);
}

#define DS_LINKED_LIST_DEPTH 50

layout (std430) buffer StartElementBufEntry
{
	int start[];
};

struct NodeData
{
	float depth;
	float alpha;
	int next;
	int prev;
};

layout (std430) buffer LinkedListEntryDepthAlphaNext
{
	NodeData LinkedListData[];
};

out vec4 color;

void main()
{

    vec3 normal = normalize(Normal_);
    vec3 viewDir = normalize(Eye - Pos_);

    vec4 tempShadowPos = (ShadowVP * vec4(Pos_, 1.0));
    tempShadowPos /= tempShadowPos.w;
    vec3 ShadowPos = tempShadowPos.xyz * 0.5 + 0.5;        // Transform NDC space coordinate from [-1.0 ~ 1.0] into [0.0 ~ 1.0].

    vec4 shadowCameraPos = (ShadowV * vec4(Pos_, 1.0));    

	vec4 diffuse = Color;

	#if defined(USE_MATERIAL)
	if (UseMaterial > 0)
	{
		diffuse.xyz *= Material.Diffuse;
		diffuse.xyz += Material.Emissive;
	}
	#endif // USE_MATERIAL

	#if defined(USE_TEXTURE)
	if (UseTexture > 0)
	{
		if (TextureSRGB[0] > 0)
		{
			// from sRGB to Linear color
			vec4 tempColor = texture(tex_object2, TexCoord_);
			diffuse.xyz *= pow(tempColor.xyz, vec3(2.2));
			diffuse.w *= tempColor.w;
		}
		else
		{
			diffuse *= texture(tex_object2, TexCoord_);
		}
	}
	#endif // USE_TEXTURE

	vec3 finalColor = vec3(0.0, 0.0, 0.0);

	for(int i=0;i<MAX_NUM_OF_DIRECTIONAL_LIGHT;++i)
    {
        if (i >= NumOfDirectionalLight)
            break;

        jDirectionalLight light = DirectionalLight[i];
#if defined(USE_MATERIAL)
		if (UseMaterial > 0)
		{
			light.SpecularLightIntensity = Material.Specular;
			light.SpecularPow = Material.SpecularShiness;
		}
#endif // USE_MATERIAL

		finalColor += KajiyaKayShadingModelTest(normal, normalize(-light.LightDirection), viewDir);
    }

	ivec2 uv = ivec2(ShadowPos.x * ShadowMapWidth, ShadowPos.y * ShadowMapHeight);
	int index = int(uint(uv.y) * ShadowMapWidth + uint(uv.x));
	int currentIndex = start[index];
	if (currentIndex != -1)
	{
		int prevIndex = -1;
		for(int k=0;k<DS_LINKED_LIST_DEPTH;++k)
		{
			if (ShadowPos.z + 0.001 > LinkedListData[currentIndex].depth)
			{
				break;
			}
			prevIndex = currentIndex;
			currentIndex = LinkedListData[currentIndex].next;
			if (currentIndex == -1)
				break;
		}
		if (prevIndex != -1)
		{
			float alpha = max(LinkedListData[prevIndex].alpha, 0.1);
			finalColor *= vec3(alpha, alpha, alpha);
		}
	}
	color = vec4(diffuse.xyz * finalColor, diffuse.w);
}