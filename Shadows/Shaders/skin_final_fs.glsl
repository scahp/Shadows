﻿#version 330 core

precision highp float;

uniform sampler2D tex_object;       // Pdt sBRDF
uniform sampler2D tex_object2;
uniform sampler2D tex_object3;
uniform sampler2D tex_object4;
uniform sampler2D tex_object5;
uniform sampler2D tex_object6;
uniform sampler2D tex_object7;      // Albedo
uniform sampler2D tex_object8;      // World Normal
uniform sampler2D tex_object9;      // TSM
uniform sampler2D tex_object10;		// StrechMap
uniform sampler2D tex_object11;		// BlurAlphaDistribution
uniform sampler2D tex_object12;		// SpecularAO
uniform sampler2DShadow shadow_object;

uniform float TextureSize;
uniform float ModelScale;
uniform float PreScatterWeight;
uniform float RoughnessScale;
uniform float SpecularScale;
uniform int EnableTSM;
uniform int VisualizeRangeSeam;
uniform int EnergyConversion;
uniform int SkinShading;

in vec2 TexCoord_;
in vec3 Pos_;
out vec4 color;

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

layout(std140) uniform DirectionalLightShadowMapBlock
{
    mat4 ShadowVP;
    mat4 ShadowV;
    vec3 LightPos;      // Directional Light Pos 임시
    float LightZNear;
    float LightZFar;
    vec2 ShadowMapSize;
};

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1
#define MAX_NUM_OF_POINT_LIGHT 1
#define MAX_NUM_OF_SPOT_LIGHT 1

uniform int NumOfDirectionalLight;
uniform jAmbientLight AmbientLight;
uniform vec3 Eye;

layout(std140) uniform DirectionalLightBlock
{
    jDirectionalLight DirectionalLight[MAX_NUM_OF_DIRECTIONAL_LIGHT];
};

#define SHADOW_BIAS_DIRECTIONAL 0.001
bool IsInShadowMapSpace(vec3 clipPos)
{
    return (clipPos.x >= 0.0 && clipPos.x <= 1.0 && clipPos.y >= 0.0 && clipPos.y <= 1.0 && clipPos.z >= 0.0 && clipPos.z <= 1.0);
}
float IsShadowing(vec3 lightClipPos, sampler2DShadow shadow_texture)
{
    return texture(shadow_object, vec3(lightClipPos.xy, lightClipPos.z - SHADOW_BIAS_DIRECTIONAL));
}

float fresnelReflectance(vec3 H, vec3 V, float F0)
{
    float base = 1.0 - dot(V, H);
    float exponential = pow(base, 5.0);
    return exponential + F0 * (1.0 - exponential);
}

float PHBeckmann(float ndoth, float m)
{
    float alpha = acos(ndoth);
    float ta = tan(alpha);
    float val = 1.0 / (m * m * pow(ndoth, 4.0)) * exp(-(ta * ta) / (m * m));
    return val;
}

float KS_Skin_Specular(
    vec3 N, // Bumped surface normal
    vec3 L, // Points to light
    vec3 V, // Points to eye
    float m,  // Roughness
    float rho_s // Specular brightness
)
{
    float result = 0.0;
    float ndotl = dot(N, L);
    if (ndotl > 0.0)
    {
        vec3 h = L + V; // Unnormalized half-way vector
        vec3 H = normalize(h);
        float ndoth = clamp(dot(N, H), -1.0, 1.0);
        float PH = PHBeckmann(ndoth, m);
        float F = fresnelReflectance(H, V, 0.02777778);
        float frSpec = max((PH * F) / dot(h, h), 0.0);
        result = ndotl * rho_s * frSpec; // BRDF * dot(N,L) * rho_s  
    }
    return result;
}

float packVec2(vec2 Data)
{
    return Data.x * 256.0 * 256.0 + Data.y;
}

vec2 unPackVec2(float Data)
{
    float temp = (Data / (256.0 * 256.0));
    float Second = fract(temp);
    float First = temp - Second;
    Second *= (256.0 * 256.0);
    return vec2(First, Second);
}

void main()
{
    vec2 uv = TexCoord_;
    uv.y = 1.0 - uv.y;

    // Shadow 
    vec4 tempShadowPos = (ShadowVP * vec4(Pos_, 1.0));
    tempShadowPos /= tempShadowPos.w;
    vec3 ShadowPos = tempShadowPos.xyz * 0.5 + 0.5;        // Transform NDC space coordinate from [-1.0 ~ 1.0] into [0.0 ~ 1.0].

    vec4 LinearShadowZ = (ShadowV * vec4(Pos_, 1.0));
    LinearShadowZ /= LinearShadowZ.w;
    ShadowPos.z = -LinearShadowZ.z / LightZFar;

    float Lit = IsShadowing(ShadowPos, shadow_object);

    vec3 specularAO = texture2D(tex_object12, TexCoord_).xyz;   // (specular intensity, roughness, occlusion)
    float rho_s = SpecularScale * specularAO.x;
    float roughness = RoughnessScale * specularAO.y;
    float occlusion = specularAO.z;

    vec3 viewDir = normalize(Eye - Pos_);
    vec3 normal = texture2D(tex_object8, TexCoord_).xyz * 2.0 - 1.0;
    normal = normalize(normal);

    jDirectionalLight light = DirectionalLight[0];
    vec3 LightPos = -light.LightDirection * 300;
    vec3 ToLight = normalize(LightPos - Pos_);
    float ndotL = clamp(dot(normal, ToLight), 0.0, 1.0);
    float LightAtten = 400.0 * 400.0 / dot(LightPos - Pos_, LightPos - Pos_);

    vec3 Irr1;
    vec4 PdtSpecularBRDF = texture2D(tex_object, vec2(ndotL, roughness));
    if (SkinShading == 7)       // PdtSpecularBRDF
    {
        color.xyz = PdtSpecularBRDF.xyz;
        return;
    }
    float sEnergy = rho_s * PdtSpecularBRDF.x;
    float dEnergy = max(1.0 - sEnergy, 0.0);
    if (EnergyConversion <= 0.0)
        dEnergy = 1.0;

    vec3 albedo = pow(texture2D(tex_object7, TexCoord_).xyz, vec3(2.2));
    {
        vec3 LightColor = light.Color * Lit * LightAtten;
        vec3 E = ndotL * LightColor;

        Irr1 = dEnergy * occlusion * E * pow(albedo, vec3(PreScatterWeight));
    }

    if (SkinShading == 1)       // Irradiance1
    {
        color.xyz = Irr1;
        return;
    }

    {
        vec3 BlurWeights[6] = vec3[6](
            vec3(0.233, 0.455, 0.649),
            vec3(0.1, 0.336, 0.344),
            vec3(0.118, 0.198, 0.0),
            vec3(0.133, 0.007, 0.007),
            vec3(0.358, 0.004, 0.0),
            vec3(0.078, 0.0, 0.0)
            );

        vec4 IrrGaussian2 = texture2D(tex_object2, uv);
        vec4 IrrGaussian4 = texture2D(tex_object3, uv);
        vec4 IrrGaussian8 = texture2D(tex_object4, uv);
        vec4 IrrGaussian16 = texture2D(tex_object5, uv);
        vec4 IrrGaussian32 = texture2D(tex_object6, uv);

        if (SkinShading == 2)       // Irradiance2
        {
            color.xyz = IrrGaussian2.xyz;
            return;
        }
        else if (SkinShading == 3)       // Irradiance3
        {
            color.xyz = IrrGaussian4.xyz;
            return;
        }
        else if (SkinShading == 4)       // Irradiance4
        {
            color.xyz = IrrGaussian8.xyz;
            return;
        }
        else if (SkinShading == 5)       // Irradiance5
        {
            color.xyz = IrrGaussian16.xyz;
            return;
        }
        else if (SkinShading == 6)       // Irradiance6
        {
            color.xyz = IrrGaussian32.xyz;
            return;
        }

        color.xyz = vec3(0.0, 0.0, 0.0);
        color.xyz += BlurWeights[0] * Irr1;
        color.xyz += BlurWeights[1] * IrrGaussian2.xyz;
        color.xyz += BlurWeights[2] * IrrGaussian4.xyz;
        color.xyz += BlurWeights[3] * IrrGaussian8.xyz;
        color.xyz += BlurWeights[4] * IrrGaussian16.xyz;
        color.xyz += BlurWeights[5] * IrrGaussian32.xyz;

        vec3 normConst = vec3(0.0);
        for (int i = 0; i < 6; ++i)
            normConst += BlurWeights[i];

        color.xyz /= normConst;

        // Make seamless UV
        float SeamAlpha = texture2D(tex_object11, uv).r;
        if (SeamAlpha < 1.0)
        {
            // Visualize Seam's problems
            if (VisualizeRangeSeam > 0)
            {
                Irr1.xyz = vec3(1.0, 0.0, 0.0);
                color.xyz = vec3(0.0, 1.0, 0.0);
            }

            SeamAlpha = clamp(pow(SeamAlpha, 3), 0.0, 1.0);
            color.xyz = mix(Irr1, color.xyz, SeamAlpha);
        }

        if (SkinShading == 8)       // StretchMap
        {
            color.xyz = vec3(texture2D(tex_object10, uv).xy, 0.0);
            return;
        }

        // TSM
        if (EnableTSM > 0)
        {
            // Compute global scatter from modified TSM
            // TSMtap = (distance to light, u, v)
            vec4 TSMtap = texture2D(tex_object9, ShadowPos.xy);

            float blur2 = max(IrrGaussian2.a, 0.01);
            float blur4 = max(IrrGaussian4.a, 0.01);
            float blur8 = max(IrrGaussian8.a, 0.01);
            float blur16 = max(IrrGaussian16.a, 0.01);

            // Four average thicknesses through the object (in mm)
            vec4 thickness_mm = 1.0 * -(1.0 / 0.2) * log(vec4(blur2, blur4, blur8, blur16));
            vec4 a_values = vec4(0.433, 0.753, 1.412, 2.722);
            vec4 inv_a = -1.0 / (2.0 * a_values * a_values);
            vec4 fades = exp(thickness_mm * thickness_mm * inv_a);
            
            vec2 stretchTap = texture2D(tex_object10, uv).xy;
            float stretchval = 0.5 * (stretchTap.x + stretchTap.y);
            float textureScale = TextureSize * 0.05 / stretchval;
            float blendFactor4 = clamp(textureScale * length(TexCoord_.xy - TSMtap.yz) / (a_values.y * 6.0), 0.0, 1.0);
            float blendFactor5 = clamp(textureScale * length(TexCoord_.xy - TSMtap.yz) / (a_values.z * 6.0), 0.0, 1.0);
            float blendFactor6 = clamp(textureScale * length(TexCoord_.xy - TSMtap.yz) / (a_values.w * 6.0), 0.0, 1.0);

            vec2 TSMUV_For_Blur = TSMtap.yz;
            TSMUV_For_Blur.y = 1.0 - TSMUV_For_Blur.y;

            vec3 TSMColor = vec3(0.0);
            TSMColor += BlurWeights[3] / normConst * fades.y * blendFactor4 * texture2D(tex_object4, TSMUV_For_Blur).xyz;
            TSMColor += BlurWeights[4] / normConst * fades.z * blendFactor5 * texture2D(tex_object5, TSMUV_For_Blur).xyz;
            TSMColor += BlurWeights[5] / normConst * fades.w * blendFactor6 * texture2D(tex_object6, TSMUV_For_Blur).xyz;

            color.xyz += TSMColor;
        }
    }

    color.w = 1.0;

    color.xyz *= pow(albedo, vec3(1.0 - PreScatterWeight));

    float sBRDF = KS_Skin_Specular(normal, ToLight, viewDir, roughness, rho_s); // White
    vec3 specularLight = vec3(sBRDF) * Lit;
    color.xyz = color.xyz + specularLight;

    //color.xyz = pow(color.xyz, vec3(1.0 / 2.2));

    //if (texture2D(tex_object10, uv).z <= 0.0)
    //    color.xyz = texture2D(tex_object2, uv).xyz;
}