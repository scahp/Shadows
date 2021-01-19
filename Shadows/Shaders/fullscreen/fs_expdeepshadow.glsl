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

uniform int UseAmbientLight;
uniform jAmbientLight AmbientLight;

#if defined(USE_MATERIAL)
uniform int UseMaterial;
uniform jMaterial Material;
#endif // USE_MATERIAL

uniform vec3 Eye;
uniform int ShadowOn;
uniform int ShadowMapWidth;
uniform int ShadowMapHeight;

in vec2 TexCoord_;

uniform sampler2D ColorSampler;
uniform sampler2D NormalSampler;
uniform sampler2D PosInWorldSampler;
uniform sampler2D PosInLightSampler;

uniform float ESM_C;

layout(location = 0) out vec4 color;

float KajiyaKayShadingModelTest(vec3 tangent, vec3 toLight, vec3 toView)
{
	float dotTL = dot(tangent, normalize(toLight));
	float dotTV = dot(-tangent, toView);

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

float logConv(float w0, float d1, float w1, float d2)
{
	return (d1 + log(w0 + (w1 * exp(d2 - d1))));
}

float bilinearInterpolation(float s, float t, float v0, float v1, float v2, float v3) 
{ 
	return (1-s)*(1-t)*v0 + s*(1-t)*v1 + (1-s)*t*v2 + s*t*v3;
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

void depthSearch(inout NodeData entry, inout NeighborData entryNeighbors, float z, out float outShading, out float outDepth)
{
	int newNum = -1;

	NodeData temp;
	if (z > entry.depth)
	{
		for(int k=0;k<DS_LINKED_LIST_DEPTH;++k)
		{
			if (entry.next == -1)
			{
				outDepth = entry.depth;
				outShading = entry.alpha;
				break;
			}

			temp = LinkedListData[entry.next];
			if (z < temp.depth)
			{
				outDepth = entry.depth;
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
				outDepth = entry.depth;
				outShading = 1.0f;
				break;
			}

			newNum = entry.prev;
			entry = LinkedListData[entry.prev];

			if (z >= entry.depth)
			{
				outDepth = entry.depth;
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

	ivec2 uv = ivec2(ShadowPos.x * ShadowMapWidth, ShadowPos.y * ShadowMapHeight);

	float depthSamples[FILTER_SIZE * 2 + 2][FILTER_SIZE * 2 + 2];
	float shadingSamples[FILTER_SIZE * 2 + 2][FILTER_SIZE * 2 + 2];

	NodeData currentXEntry;
	NodeData currentYEntry;
	NeighborData currentXEntryNeighbors;
	NeighborData currentYEntryNeighbors;

	bool noXLink = true;
	int currentX = uv.x - FILTER_SIZE;
	for(int x = 0;x < FILTER_SIZE * 2 + 2;++x)
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

		for(int y = 0;y < FILTER_SIZE * 2 + 2;++y)
		{
			if (currentX < 0 || currentY < 0 || currentX >= ShadowMapWidth || currentY >= ShadowMapHeight)
			{
				depthSamples[x][y] = 1.0f;	
				shadingSamples[x][y] = 1.0f;
				++currentY;
				continue;
			}

			if (noYLink)
			{
				int startIndex = start[currentY * ShadowMapWidth + currentX];
				if (startIndex == -1)
				{
					depthSamples[x][y] = 1.0f;	
					shadingSamples[x][y] = 1.0f;
					++currentY;
					continue;
				}

				noYLink = false;
				currentYEntry = LinkedListData[startIndex];
				currentYEntryNeighbors = NeighborListData[startIndex];
			}

			depthSearch(currentYEntry, currentYEntryNeighbors, ShadowPos.z, shadingSamples[x][y], depthSamples[x][y]);

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

#if FILTER_SIZE > 0
	float depthSamples2[2][FILTER_SIZE * 2 + 2];
	float shadingSamples2[2][FILTER_SIZE * 2 + 2];

	float oneOver = 1.0f / (FILTER_SIZE * 2 + 1);
	
	for(int y = 0; y < FILTER_SIZE * 2 + 2; y++)
		for(int x = FILTER_SIZE; x < FILTER_SIZE + 2; x++)
		{
			int x2 = x - FILTER_SIZE;
			float filteredShading = 0;

			float sample0 = depthSamples[x2][y];
			filteredShading = shadingSamples[x2][y];
			x2++;
			
			float sample1 = depthSamples[x2][y];
			filteredShading += shadingSamples[x2][y];
			x2++;
		
			float filteredDepth = logConv(oneOver, sample0, oneOver, sample1);

			for(; x2 <= x + FILTER_SIZE; x2++)
			{
				filteredDepth = logConv(1.0f, filteredDepth, oneOver, depthSamples[x2][y]);
				filteredShading += shadingSamples[x2][y];
			}

			depthSamples2[x - FILTER_SIZE][y] = filteredDepth;
			shadingSamples2[x - FILTER_SIZE][y] = filteredShading * oneOver;
		}

	for(int x = FILTER_SIZE; x < FILTER_SIZE + 2; x++)
		for(int y = FILTER_SIZE; y < FILTER_SIZE + 2; y++)
		{
			int y2 = y - FILTER_SIZE;
			float filteredShading = 0;

			float sample0 = depthSamples2[x - FILTER_SIZE][y2];
			filteredShading = shadingSamples2[x - FILTER_SIZE][y2];
			y2++;
			
			float sample1 = depthSamples2[x - FILTER_SIZE][y2];
			filteredShading += shadingSamples2[x - FILTER_SIZE][y2];
			y2++;
		
			float filteredDepth = logConv(oneOver, sample0, oneOver, sample1);

			for(; y2 <= y + FILTER_SIZE; y2++)
			{
				filteredDepth = logConv(1.0f, filteredDepth, oneOver, depthSamples2[x - FILTER_SIZE][y2]);
				filteredShading += shadingSamples2[x - FILTER_SIZE][y2];
			}

			depthSamples[x][y] = filteredDepth;
			shadingSamples[x][y] = filteredShading * oneOver;
		}
#endif

	float dx = fract(ShadowPos.x * ShadowMapWidth);
	float dy = fract(ShadowPos.y * ShadowMapHeight);

	float depth = bilinearInterpolation(dx, dy, depthSamples[FILTER_SIZE][FILTER_SIZE], depthSamples[FILTER_SIZE + 1][FILTER_SIZE], depthSamples[FILTER_SIZE][FILTER_SIZE + 1], depthSamples[FILTER_SIZE + 1][FILTER_SIZE + 1]);
	float shading = bilinearInterpolation(dx, dy, shadingSamples[FILTER_SIZE][FILTER_SIZE], shadingSamples[FILTER_SIZE + 1][FILTER_SIZE], shadingSamples[FILTER_SIZE][FILTER_SIZE + 1], shadingSamples[FILTER_SIZE + 1][FILTER_SIZE + 1]);
	shading = clamp(shading * exp(20.0f * (depth - (ShadowPos.z))), 0.1f, 1.0f);

	ambientColor *= DiffuseColor.xyz;
	directColor *= (1.0 / 3.14) * DiffuseColor.xyz * clamp(shading, 0.1, 1.0);

	color = vec4(ambientColor + directColor, 1.0);
}