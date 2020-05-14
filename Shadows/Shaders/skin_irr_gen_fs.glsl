#version 330 core

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
uniform sampler2DShadow shadow_object;
uniform int UseTexture;
uniform vec3 Eye;
uniform mat4 VP;

in vec2 TexCoord_;
in vec3 Pos_;
out vec4 color;

uniform int NumOfDirectionalLight;
uniform jAmbientLight AmbientLight;

layout(std140) uniform DirectionalLightBlock
{
	jDirectionalLight DirectionalLight[MAX_NUM_OF_DIRECTIONAL_LIGHT];
};

#define SHADOW_BIAS_DIRECTIONAL 0.0001
bool IsInShadowMapSpace(vec3 clipPos)
{
    return (clipPos.x >= 0.0 && clipPos.x <= 1.0 && clipPos.y >= 0.0 && clipPos.y <= 1.0 && clipPos.z >= 0.0 && clipPos.z <= 1.0);
}
float IsShadowing(vec3 lightClipPos, sampler2DShadow shadow_object)
{
    if (IsInShadowMapSpace(lightClipPos))
        return texture(shadow_object, vec3(lightClipPos.xy, lightClipPos.z - SHADOW_BIAS_DIRECTIONAL));

    return 1.0;
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
    vec3 normal = texture2D(tex_object3, TexCoord_).xyz * 2.0 - 1.0;
    normal = normalize(normal);

    // Shadow 
    vec4 tempShadowPos = (ShadowVP * vec4(Pos_, 1.0));
    tempShadowPos /= tempShadowPos.w;
    vec3 ShadowPos = tempShadowPos.xyz * 0.5 + 0.5;        // Transform NDC space coordinate from [-1.0 ~ 1.0] into [0.0 ~ 1.0].
    float Lit = IsShadowing(ShadowPos, shadow_object);
    //////////////////////////////////////////////////////////

	jDirectionalLight light = DirectionalLight[0];

    vec3 LightPos = -light.LightDirection * 500;
    vec3 ToLight = normalize(LightPos - Pos_);

    float roughness = 0.3;
    float rho_s = 0.18;
    float sBRDF = KS_Skin_Specular(normal, ToLight, viewDir, roughness, rho_s); // White
    vec3 specularLight = vec3(sBRDF);

    vec3 albedo = pow(texture2D(tex_object2, TexCoord_).xyz, vec3(2.2));      // to linear space
    vec3 lightForDiffuse = max((vec3(1.0) - specularLight.xyz), vec3(0.0));
    vec3 LightColor = light.Color * Lit;
    vec3 E = clamp(dot(normal, ToLight), 0.0, 1.0) * LightColor * lightForDiffuse; // todo : it should be added for shadow term.
    //color.xyz = specularLight + (albedo * E);
    color.xyz = E;

    //color.xyz = pow(color.xyz, vec3(1.0 / 2.2));    // to sRGB space

    {
        float DistToLight = length(LightPos - Pos_);
        vec4 TSMTap = texture2D(tex_object4, ShadowPos.xy);

        // Find normal on back side of object, Ni
        vec3 Ni = texture2D(tex_object3, TSMTap.yz).xyz * 2.0 - 1.0;
        Ni = normalize(normal);
        float backFacingEst = clamp(-dot(Ni, normal), 0.0, 1.0);
        float thicknessToLight = DistToLight - TSMTap.x;

        float ndotL = dot(normal, ToLight);

        // Set a large distance for surface points facing the light
        if (ndotL > 0.0)
            thicknessToLight = 50.0;

        float correctedThickness = clamp(-ndotL, 0.0, 1.0) * thicknessToLight;
        float finalThickness = mix(thicknessToLight, correctedThickness, backFacingEst);

        // Exponentiate thickness value for storage as 8-bit alpha
        float alpha = exp(finalThickness * -20.0);
        color.w = alpha;
    }
}
