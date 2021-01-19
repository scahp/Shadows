#version 430 core

#preprocessor

#include "common.glsl"

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1
#define FILTER_SIZE 2
#define FILTER_AREA ((FILTER_SIZE * 2 + 1) * (FILTER_SIZE * 2 + 1))

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

uniform int UseAmbientLight;
uniform jAmbientLight AmbientLight;

uniform vec3 Eye;
uniform int ShadowOn;
uniform int ShadowMapWidth;
uniform int ShadowMapHeight;

in vec2 TexCoord_;

uniform sampler2D ColorSampler;
uniform sampler2D NormalSampler;
uniform sampler2D PosInWorldSampler;
uniform sampler2D PosInLightSampler;

layout(location = 0) out vec4 color;

float KajiyaKayShadingModelTest(vec3 tangent, vec3 toLight, vec3 toView)
{
	float dotTL = dot(tangent, normalize(toLight));
	float dotTV = dot(-tangent, normalize(toView));

	float sinTL = sqrt(1 - dotTL * dotTL);
	float sinTV = sqrt(1 - dotTV * dotTV);

	float diffuse = clamp(sinTL, 0.0, 1.0);
	float specular = clamp(pow(dotTL*dotTV + sinTL*sinTV, 6.0), 0.0, 1.0);
	//return (diffuse + specular);
	return (clamp(diffuse, 0.0, 1.0) * 0.7f + clamp(specular, 0.0, 1.0) * 0.8f) * 0.9f + 0.09f;
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

struct NeighborData
{
	int right;
	int top;
};

layout (std430) buffer LinkedListEntryNeighbors
{
	NeighborData NeighborListData[];
};

void depthSearch(inout NodeData entry, inout NeighborData entryNeighbors, float z, out float outShading)
{
	int newNum = -1;

	NodeData temp;
	if (z > entry.depth)
	{
		for(int k=0;k<DS_LINKED_LIST_DEPTH;++k)
		{
			if (entry.next == -1)
			{
				outShading = entry.alpha;
				break;
			}

			temp = LinkedListData[entry.next];
			if (z < temp.depth)
			{
				outShading = entry.alpha;
				break;
			}
			newNum = entry.next;
			entry = temp;
		}
	}
	else
	{
		for(int k=0;k<DS_LINKED_LIST_DEPTH;++k)
		{
			if (entry.prev == -1)
			{
				outShading = 1.0f;
				break;
			}

			newNum = entry.prev;
			entry = LinkedListData[entry.prev];

			if (z >= entry.depth)
			{
				outShading = entry.alpha;
				break;
			}
		}
	}

	if (newNum != -1)
		entryNeighbors = NeighborListData[newNum];
}

void main()
{
	vec4 Normal = texture(NormalSampler, TexCoord_);
	if (Normal.x == 0.0 && Normal.y == 0.0 && Normal.z == 0.0)
		return;

	bool isDataFromTexture = (Normal.w == 0.0);

	vec4 DiffuseColor = texture(ColorSampler, TexCoord_);
	vec3 Pos = texture(PosInWorldSampler, TexCoord_).xyz;
	vec3 ShadowPos = texture(PosInLightSampler, TexCoord_).xyz;

	vec3 toLight = normalize(LightPos - Pos);
	vec3 toEye = normalize(Eye - Pos);

	vec3 directColor = vec3(0.0, 0.0, 0.0);
	vec3 ambientColor = vec3(0.0, 0.0, 0.0);

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

		if (isDataFromTexture)
			directColor += GetDirectionalLight(light, Normal.xyz, toEye);
		else
			directColor += KajiyaKayShadingModelTest(Normal.xyz, -light.LightDirection, toEye);
			//directColor += KajiyaKayShadingModelTest(Normal.xyz, toLight, toEye);
    }

	if (UseAmbientLight != 0)
		ambientColor += GetAmbientLight(AmbientLight);

	float shading = 0.0;
	if (ShadowOn > 0.0)
	{
		ivec2 uv = ivec2(ShadowPos.x * ShadowMapWidth, ShadowPos.y * ShadowMapHeight);

		NodeData currentXEntry;
		NodeData currentYEntry;
		NeighborData currentXEntryNeighbors;
		NeighborData currentYEntryNeighbors;

		bool noXLink = true;
		int currentX = uv.x - FILTER_SIZE;
		for (int x = 0; x < FILTER_SIZE * 2 + 1; ++x)
		{
			bool noYLink = true;
			int currentY = uv.y - FILTER_SIZE;

			if (noXLink && !(currentX < 0 || currentY < 0 || currentX >= ShadowMapWidth || currentY >= ShadowMapHeight))
			{
				int startIndex = start[currentY * ShadowMapWidth + currentX];
				if (startIndex != -1)
				{
					currentXEntry = LinkedListData[startIndex];
					currentXEntryNeighbors = NeighborListData[startIndex];
					noXLink = false;
				}
			}

			if (noXLink)
			{
				noYLink = true;
			}
			else
			{
				currentYEntry = currentXEntry;
				currentYEntryNeighbors = currentXEntryNeighbors;
			}

			for (int y = 0; y < FILTER_SIZE * 2 + 1; ++y)
			{
				if (currentX < 0 || currentY < 0 || currentX >= ShadowMapWidth || currentY >= ShadowMapHeight)
				{
					shading += 1.0;
					++currentY;
					continue;
				}

				if (noYLink)
				{
					int startIndex = start[currentY * ShadowMapWidth + currentX];
					if (startIndex == -1)
					{
						shading += 1.0;
						++currentY;
						continue;
					}

					noYLink = false;
					currentYEntry = LinkedListData[startIndex];
					currentYEntryNeighbors = NeighborListData[startIndex];
				}

				float localShading = 0.0f;
				depthSearch(currentYEntry, currentYEntryNeighbors, ShadowPos.z, localShading);
				shading += localShading;

				++currentY;

				if (currentYEntryNeighbors.top != -1 && !noYLink)
				{
					currentYEntry = LinkedListData[currentYEntryNeighbors.top];
					currentYEntryNeighbors = NeighborListData[currentYEntryNeighbors.top];
				}
				else
				{
					noYLink = true;
				}
			}

			++currentX;

			if (currentXEntryNeighbors.right != -1 && !noXLink)
			{
				currentXEntry = LinkedListData[currentXEntryNeighbors.right];
				currentXEntryNeighbors = NeighborListData[currentXEntryNeighbors.right];
			}
			else
			{
				noXLink = true;
			}
		}

		shading /= FILTER_AREA;
	}
	else
	{
		shading = 1.0;
	}

	ambientColor *= DiffuseColor.xyz;
	directColor *= (1.0 / 3.14) * DiffuseColor.xyz * clamp(shading, 0.1, 1.0);

	color = vec4(ambientColor + directColor, 1.0);
}