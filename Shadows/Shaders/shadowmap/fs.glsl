#version 330 core

#preprocessor

#include "shadow.glsl"

precision mediump float;
precision mediump sampler2DArray;

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1
#define MAX_NUM_OF_POINT_LIGHT 1
#define MAX_NUM_OF_SPOT_LIGHT 1

uniform int UseAmbientLight;
uniform int NumOfDirectionalLight;
uniform int NumOfPointLight;
uniform int NumOfSpotLight;

uniform jAmbientLight AmbientLight;
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

layout (std140) uniform DirectionalLightShadowMapBlock
{
	mat4 ShadowVP;
	mat4 ShadowV;
	vec3 LightPos;      // Directional Light Pos �ӽ�
	float LightZNear;
	float LightZFar;
	vec2 ShadowMapSize;
};

layout (std140) uniform PointLightShadowMapBlock
{
	float PointLightZNear;
	float PointLightZFar;
	vec2 PointShadowMapSize;
};

layout (std140) uniform SpotLightShadowMapBlock
{
	float SpotLightZNear;
	float SpotLightZFar;
	vec2 SpotShadowMapSize;
};

#if defined(USE_MATERIAL)
uniform jMaterial Material;
#endif // USE_MATERIAL

uniform vec3 Eye;
uniform int Collided;

#if defined(USE_TEXTURE)
uniform sampler2D tex_object2;
uniform int TextureSRGB[1];
#endif // USE_TEXTURE

uniform sampler2DShadow shadow_object_point_shadow;
uniform sampler2D shadow_object_point;
uniform sampler2DShadow shadow_object_spot_shadow;
uniform sampler2D shadow_object_spot;
uniform sampler2DShadow shadow_object;
uniform sampler2D shadow_object_test;
//uniform sampler2D shadow_object;
uniform float PCF_Size_Directional;
uniform float PCF_Size_OmniDirectional;
uniform float ESM_C;
uniform float PointLightESM_C;
uniform float SpotLightESM_C;
uniform int UseTexture;
uniform int UseUniformColor;
uniform vec4 Color;
uniform int UseMaterial;
uniform int ShadowOn;
uniform int CSMDebugOn;

// in vec3 ShadowPos_;
// in vec3 ShadowCameraPos_;
in vec3 Pos_;
in vec4 Color_;
in vec3 Normal_;

#if defined(USE_TEXTURE)
in vec2 TexCoord_;
#endif // USE_TEXTURE

out vec4 color;

#if defined(USE_CSM)
#define NUM_CASCADES 3
in vec3 PosV_;
in vec4 LightSpacePos[NUM_CASCADES];
uniform float CascadeEndsW[NUM_CASCADES];

int GetCascadeIndex(float viewSpaceZ)
{
	for (int i = 0; i < NUM_CASCADES; ++i)
	{
		if (-viewSpaceZ < CascadeEndsW[i])
			return i;
	}
	return -1;
}

#define CSM_BIAS 0.003

bool IsInRange01(vec3 pos)
{
	return (pos.x >= 0.0 && pos.x <= 1.0 && pos.y >= 0.0 && pos.y <= 1.0 && pos.z >= 0.0 && pos.z <= 1.0);
}

float IsShadowingCSM(sampler2DShadow csm_sampler, vec4 lightClipScapePos)
{
	if (IsInRange01(lightClipScapePos.xyz))
		return texture(csm_sampler, vec3(lightClipScapePos.xy, lightClipScapePos.z - CSM_BIAS));

	return 0.0;
}
float IsShadowingCSM(sampler2DShadow csm_sampler, vec4 lightClipScapePos, int cascadeIndex)
{
	if (IsInRange01(lightClipScapePos.xyz))
	{
		vec2 coords = vec2(lightClipScapePos.x, (lightClipScapePos.y + float(cascadeIndex)) / float(NUM_CASCADES) );
		return texture(csm_sampler, vec3(coords.xy, lightClipScapePos.z - CSM_BIAS));
	}

	return 0.0;
}
#endif // USE_CSM

void main()
{
#if defined(USE_VSM)
    float vsmBiasForOmniDirectional = 0.1;
#endif // defined(USE_VSM) || defined(USE_EVSM)

#if defined(USE_EVSM)
    float evsmBiasForOmniDirectional = 0.1;
#endif // USE_EVSM

    vec3 normal = normalize(Normal_);
    vec3 viewDir = normalize(Eye - Pos_);

    vec4 tempShadowPos = (ShadowVP * vec4(Pos_, 1.0));
    tempShadowPos /= tempShadowPos.w;
    vec3 ShadowPos = tempShadowPos.xyz * 0.5 + 0.5;        // Transform NDC space coordinate from [-1.0 ~ 1.0] into [0.0 ~ 1.0].

    vec4 shadowCameraPos = (ShadowV * vec4(Pos_, 1.0));    

	vec4 diffuse = vec4(1.0);

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
	color = diffuse;
	return;
#else
	diffuse = Color_;
#endif // USE_TEXTURE

	if (UseUniformColor > 0)
		diffuse = Color;

    if (Collided != 0)
        diffuse = vec4(1.0, 1.0, 1.0, 1.0);

#if defined(USE_MATERIAL)
	if (UseMaterial > 0)
	{
		diffuse.xyz *= Material.Diffuse;
		diffuse.xyz += Material.Emissive;
	}
#endif // USE_MATERIAL

    bool shadow = false;
    vec3 directColor = vec3(0.0, 0.0, 0.0);

	vec3 ambientColor = vec3(0.0, 0.0, 0.0);
    if (UseAmbientLight != 0)
        ambientColor = GetAmbientLight(AmbientLight);

#if defined(USE_POISSON_SAMPLE)
        InitPoissonSamples(gl_FragCoord.xy);
#endif

    for(int i=0;i<MAX_NUM_OF_DIRECTIONAL_LIGHT;++i)
    {
        if (i >= NumOfDirectionalLight)
            break;

        float lit = 1.0;

		if (ShadowOn > 0)
		{
			vec2 shadowMapTexelSize = vec2(1.0) / ShadowMapSize;

	#if defined(USE_PCSS)
			{
	#if defined(USE_POISSON_SAMPLE)
				lit = PCSS_PoissonSample(ShadowPos, -shadowCameraPos.z, shadow_object_test, shadow_object, LightZNear, shadowMapTexelSize);
	#else // USE_POISSON_SAMPLE
				lit = PCSS(ShadowPos, -shadowCameraPos.z, shadow_object_test, shadow_object, LightZNear, shadowMapTexelSize);
	#endif // USE_POISSON_SAMPLE
			}
	#elif defined(USE_PCF)
			{
	#if defined(USE_POISSON_SAMPLE)
				lit = PCF_PoissonSample(ShadowPos, vec2(PCF_Size_Directional, PCF_Size_Directional) * shadowMapTexelSize, shadow_object);
	#else // USE_POISSON_SAMPLE
				lit = PCF(ShadowPos, vec2(PCF_Size_Directional, PCF_Size_Directional) * shadowMapTexelSize, shadow_object);
	#endif // USE_POISSON_SAMPLE
			}
	#elif defined(USE_VSM)
			{
				lit = VSM(ShadowPos, LightPos, Pos_, shadow_object_test);
			}
	#elif defined(USE_ESM)
			{
				lit = ESM(ShadowPos, LightPos, Pos_, LightZNear, LightZFar, ESM_C, shadow_object_test);
			}
	#elif defined(USE_EVSM)
			{
				lit = EVSM(ShadowPos, LightPos, Pos_, LightZNear, LightZFar, ESM_C, shadow_object_test);
			}
	#else
			{
#if defined(USE_CSM)
				vec4 lightClipSpacePos[NUM_CASCADES];

				int index = -1;
				for (int i = 0; i < NUM_CASCADES; ++i)
				{
					lightClipSpacePos[i] = (LightSpacePos[i] / LightSpacePos[i].w) * 0.5 + vec4(0.5);
					if (IsInRange01(lightClipSpacePos[i].xyz) && (-PosV_.z <= CascadeEndsW[i])
						)
					{
						index = i;
						break;
					}
				}

				if (CSMDebugOn > 0)
				{
					if (index == 0)
						directColor.x += 0.5f;
					if (index == 1)
						directColor.y += 0.5f;
					if (index == 2)
						directColor.z += 0.5f;
				}

				if (index != -1)
					lit = IsShadowingCSM(shadow_object, lightClipSpacePos[index], index);
#else	// USE_CSM
				lit = IsShadowing(ShadowPos, shadow_object);
#endif	// USE_CSM
			}
#endif	// USE_PCSS
		}

        if (lit > 0.0)
        {
            jDirectionalLight light = DirectionalLight[i];
#if defined(USE_MATERIAL)
			if (UseMaterial > 0)
			{
				light.SpecularLightIntensity = Material.Specular;
				light.SpecularPow = Material.SpecularShiness;
			}
#endif // USE_MATERIAL

			directColor += GetDirectionalLight(light, normal, viewDir) * lit;
        }
    }

    for(int i=0;i<MAX_NUM_OF_POINT_LIGHT;++i)
    {
        if (i >= NumOfPointLight)
            break;

        vec3 toLight = PointLight[i].LightPos - Pos_;
        float distFromLightSqrt = dot(toLight, toLight);

        if (distFromLightSqrt > (PointLight[i].MaxDistance * PointLight[i].MaxDistance))
            continue;

        float lit = 1.0;

		if (ShadowOn > 0)
		{
			vec2 shadowMapTexelSize = 1.0 / PointShadowMapSize.xx;		// shadow array texture size is (w, w * 6)

	#if defined(USE_PCSS)
			{

	#if defined(USE_POISSON_SAMPLE)
				lit = PCSS_OmniDirectional_PoissonSample(Pos_, PointLight[i].LightPos, PointLightZNear, shadowMapTexelSize, PointLight[i].MaxDistance, shadow_object_point_shadow, shadow_object_point);
	#else // USE_POISSON_SAMPLE
				lit = PCSS_OmniDirectional(Pos_, PointLight[i].LightPos, PointLightZNear, shadowMapTexelSize, PointLight[i].MaxDistance, shadow_object_point_shadow, shadow_object_point);
	#endif // USE_POISSON_SAMPLE
			}
	#elif defined(USE_PCF)
			{
				vec2 radiusSquredUV = vec2(PCF_Size_OmniDirectional, PCF_Size_OmniDirectional) * shadowMapTexelSize;
	#if defined(USE_POISSON_SAMPLE)
				lit = PCF_OmniDirectional_PoissonSample(Pos_, PointLight[i].LightPos, PointLight[i].MaxDistance, radiusSquredUV, shadow_object_point_shadow);
	#else
				lit = PCF_OmniDirectional(Pos_, PointLight[i].LightPos, PointLight[i].MaxDistance, radiusSquredUV, shadow_object_point_shadow);
	#endif
			}
	#elif defined(USE_VSM)
			{
				lit = VSM_OmniDirectional(PointLight[i].LightPos, Pos_, shadow_object_point, vsmBiasForOmniDirectional);
			}
	#elif defined(USE_ESM)
			{
				lit = ESM_OmniDirectional(PointLight[i].LightPos, Pos_, PointLightZNear, PointLightZFar, PointLightESM_C, shadow_object_point);
			}
	#elif defined(USE_EVSM)
			{
				lit = EVSM_OmniDirectional(PointLight[i].LightPos, Pos_, PointLightZNear, PointLightZFar, PointLightESM_C, shadow_object_point, evsmBiasForOmniDirectional);
			}
	#else
			{
				lit = IsShadowing(Pos_, PointLight[i].LightPos, PointLight[i].MaxDistance, shadow_object_point_shadow);
			}
	#endif
		}

        if (lit > 0.0)
        {
            jPointLight light = PointLight[i];
#if defined(USE_MATERIAL)
			if (UseMaterial > 0)
			{
				light.SpecularLightIntensity = Material.Specular;
				light.SpecularPow = Material.SpecularShiness;
			}
#endif // USE_MATERIAL

			directColor += GetPointLight(light, normal, Pos_, viewDir) * lit;
        }
    }
    
    for(int i=0;i<MAX_NUM_OF_SPOT_LIGHT;++i)
    {
        if (i >= NumOfSpotLight)
            break;

        vec3 toLight = SpotLight[i].LightPos - Pos_;
        float distFromLightSqrt = dot(toLight, toLight);

        if (distFromLightSqrt > (SpotLight[i].MaxDistance * SpotLight[i].MaxDistance))
            continue;

        float lit = 1.0;

		if (ShadowOn > 0)
		{
			vec2 shadowMapTexelSize = 1.0 / SpotShadowMapSize.xx;		// shadow array texture size is (w, w * 6)

	#if defined(USE_PCSS)
			{
	#if defined(USE_POISSON_SAMPLE)
				lit = PCSS_OmniDirectional_PoissonSample(Pos_, SpotLight[i].LightPos, SpotLightZNear, shadowMapTexelSize, SpotLight[i].MaxDistance, shadow_object_spot_shadow, shadow_object_spot);
	#else // USE_POISSON_SAMPLE
				lit = PCSS_OmniDirectional(Pos_, SpotLight[i].LightPos, SpotLightZNear, shadowMapTexelSize, SpotLight[i].MaxDistance, shadow_object_spot_shadow, shadow_object_spot);
	#endif // USE_POISSON_SAMPLE
			}
	#elif defined(USE_PCF)
			{
				vec2 radiusSquredUV = vec2(PCF_Size_OmniDirectional, PCF_Size_OmniDirectional) * shadowMapTexelSize;
	#if defined(USE_POISSON_SAMPLE)
				lit = PCF_OmniDirectional_PoissonSample(Pos_, SpotLight[i].LightPos, SpotLight[i].MaxDistance, radiusSquredUV, shadow_object_spot_shadow);
	#else // USE_POISSON_SAMPLE
				lit = PCF_OmniDirectional(Pos_, SpotLight[i].LightPos, SpotLight[i].MaxDistance, radiusSquredUV, shadow_object_spot_shadow);
	#endif // USE_POISSON_SAMPLE
			}
	#elif defined(USE_VSM)
			{
				lit = VSM_OmniDirectional(SpotLight[i].LightPos, Pos_, shadow_object_spot, vsmBiasForOmniDirectional);
			}
	#elif defined(USE_ESM)
			{
				lit = ESM_OmniDirectional(SpotLight[i].LightPos, Pos_, SpotLightZNear, SpotLightZFar, SpotLightESM_C, shadow_object_spot);
			}
	#elif defined(USE_EVSM)
			{
				lit = EVSM_OmniDirectional(SpotLight[i].LightPos, Pos_, SpotLightZNear, SpotLightZFar, SpotLightESM_C, shadow_object_spot, evsmBiasForOmniDirectional);
			}
	#else
			{
				lit = IsShadowing(Pos_, SpotLight[i].LightPos, SpotLight[i].MaxDistance, shadow_object_spot_shadow);
			}
	#endif
		}

        if (lit > 0.0)
        {
            jSpotLight light = SpotLight[i];
#if defined(USE_MATERIAL)
			if (UseMaterial > 0)
			{
				light.SpecularLightIntensity = Material.Specular;
				light.SpecularPow = Material.SpecularShiness;
			}
#endif // USE_MATERIAL

			directColor += GetSpotLight(light, normal, Pos_, viewDir) * lit;
        }
    }

	ambientColor *= diffuse.xyz;
	directColor *= 1.0 / 3.14 * diffuse.xyz;

    color = vec4(ambientColor + directColor, diffuse.w);
}