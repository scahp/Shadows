#version 430 core

precision highp float;

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

uniform sampler2D tex_object2;      // Albedo
uniform sampler2D tex_object3;      // World Normal
uniform sampler2D tex_object4;      // TSM
uniform sampler2D tex_object5;      // Stretch
uniform sampler2D tex_object6;      // PdtBRDFBackerTarget
uniform sampler2D tex_object7;      // SpecularAO
uniform sampler2DShadow shadow_object;
uniform int UseTexture;
uniform vec3 Eye;
uniform mat4 VP;
uniform float ModelScale;

in vec2 TexCoord_;
in vec3 Pos_;
in vec4 VPos_;
out vec4 color;

uniform int NumOfDirectionalLight;
uniform jAmbientLight AmbientLight;
uniform float PreScatterWeight;
uniform float RoughnessScale;
uniform float SpecularScale;
uniform int EnableTSM;


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
    return texture(shadow_texture, vec3(lightClipPos.xy, lightClipPos.z - SHADOW_BIAS_DIRECTIONAL));
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

void main()
{
	vec3 viewDir = normalize(Eye - Pos_);
    vec3 normalRaw = texture2D(tex_object3, TexCoord_).xyz;
    vec3 normal = normalRaw * 2.0 - 1.0;
    normal = normalize(normal);

    // Shadow 
    vec4 tempShadowPos = (ShadowVP * vec4(Pos_, 1.0));
    tempShadowPos /= tempShadowPos.w;
    vec3 ShadowPos = tempShadowPos.xyz * 0.5 + 0.5;        // Transform NDC space coordinate from [-1.0 ~ 1.0] into [0.0 ~ 1.0].

    float vZ = (VPos_.z / VPos_.w);
    vZ = -vZ / LightZFar;
    ShadowPos.z = vZ;

    float Lit = IsShadowing(ShadowPos, shadow_object);
    //////////////////////////////////////////////////////////

	jDirectionalLight light = DirectionalLight[0];

    vec3 LightPos = -light.LightDirection * 300;
    vec3 ToLight = normalize(LightPos - Pos_);
    float LightAtten = 400.0 * 400.0 / dot(LightPos - Pos_, LightPos - Pos_);

    vec3 specularAO = texture2D(tex_object7, TexCoord_).xyz;   // (specular intensity, roughness, occlusion)
    float rho_s = SpecularScale * specularAO.x;
    float roughness = RoughnessScale * specularAO.y;
    float occlusion = specularAO.z;

    float ndotL = clamp(dot(normal, ToLight), 0.0, 1.0);
    float sEnergy = rho_s * texture2D(tex_object6, vec2(ndotL, roughness)).x;
    float dEnergy = max(1.0 - sEnergy, 0.0);
    //dEnergy = 1.0;

    vec3 albedo = pow(texture2D(tex_object2, TexCoord_).xyz, vec3(2.2));      // to linear space
    vec3 LightColor = light.Color * Lit * LightAtten;
    vec3 E = clamp(dot(normal, ToLight), 0.0, 1.0) * LightColor;
    color.xyz = dEnergy * occlusion * E * pow(albedo, vec3(PreScatterWeight));

    // TSM Alpha
    float TSMAlpha = 0.0;
    if (EnableTSM > 0)
    {
        float DistToLight = length(LightPos - Pos_);
        vec4 TSMTap = texture2D(tex_object4, ShadowPos.xy);

        // Find normal on back side of object, Ni
        vec3 Ni = texture2D(tex_object3, TSMTap.yz).xyz * 2.0 - 1.0;
        Ni = normalize(normal);
        float backFacingEst = clamp(-dot(Ni, normal), 0.0, 1.0);
        float thicknessToLight = (DistToLight - TSMTap.x) / ModelScale;

        float ndotL = dot(normal, ToLight);

        // Set a large distance for surface points facing the light
        float bias = -0.3;
        if (ndotL > 0.0 + bias)
            thicknessToLight = 5000.0;

        float correctedThickness = clamp(-ndotL, 0.0, 1.0) * thicknessToLight;
        float finalThickness = mix(thicknessToLight, correctedThickness, backFacingEst);

        // Exponentiate thickness value for storage as 8-bit alpha
        TSMAlpha = exp(finalThickness * -20.0);
    }
    
    color.w = TSMAlpha;
}
