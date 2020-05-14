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

#define MAX_NUM_OF_DIRECTIONAL_LIGHT 1
#define MAX_NUM_OF_POINT_LIGHT 1
#define MAX_NUM_OF_SPOT_LIGHT 1

uniform sampler2D tex_object2;
uniform sampler2D tex_object3;
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

        //float base = 1.0 - dot(V, H);
        //float exponential = pow(base, 5.0);
        //float test = exponential + 0.028 * (1.0 - exponential);
        //if (exponential + 0.028 * (1.0 - exponential) > 0.01)
        //    return frSpec;
        //return frSpec;
    }
    return result;
}

void main()
{
	vec3 viewDir = normalize(Eye - Pos_);
    vec3 normal = texture2D(tex_object3, TexCoord_).xyz * 2.0 - 1.0;
    normal = normalize(normal);

	jDirectionalLight light = DirectionalLight[0];

    vec3 LightPos = -light.LightDirection * 500;
    vec3 ToLight = normalize(LightPos - Pos_);

    float roughness = 0.3;
    float rho_s = 0.18;
    float sBRDF = KS_Skin_Specular(normal, ToLight, viewDir, roughness, rho_s); // White
    vec3 specularLight = vec3(sBRDF);
    color = vec4(specularLight, 1.0);

    color.xyz = pow(color.xyz, vec3(1.0 / 2.2));
}