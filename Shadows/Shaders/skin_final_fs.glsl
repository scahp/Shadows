#version 330 core

precision highp float;

uniform sampler2D tex_object;
uniform sampler2D tex_object2;
uniform sampler2D tex_object3;
uniform sampler2D tex_object4;
uniform sampler2D tex_object5;
uniform sampler2D tex_object6;
uniform sampler2D tex_object7;      // Albedo
uniform sampler2D tex_object8;      // World Normal
uniform sampler2D tex_object9;      // TSM
uniform sampler2D tex_object10;		// StrechMap
uniform sampler2DShadow shadow_object; 

uniform float TextureSize;
uniform float ModelScale;

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
    vec2 uv = TexCoord_;
    uv.y = 1.0 - uv.y;

    vec3 Irr1;
    {
        vec3 viewDir = normalize(Eye - Pos_);
        vec3 normal = texture2D(tex_object8, TexCoord_).xyz * 2.0 - 1.0;
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

        Irr1 = E;
    }

    {
        vec3 BlurWieghts[6] = vec3[6](
            vec3(0.233, 0.455, 0.649),
            vec3(0.1, 0.336, 0.344),
            vec3(0.118, 0.198, 0.0),
            vec3(0.133, 0.007, 0.007),
            vec3(0.358, 0.004, 0.0),
            vec3(0.078, 0.0, 0.0)
            );

        color.xyz = vec3(0.0, 0.0, 0.0);
        color.xyz += BlurWieghts[0] * texture2D(tex_object, uv).xyz;        // 최대 해상도를 얻기위해서 요건 여기서 실시간으로 계산해도 될듯.
        color.xyz += BlurWieghts[1] * texture2D(tex_object2, uv).xyz;
        color.xyz += BlurWieghts[2] * texture2D(tex_object3, uv).xyz;
        color.xyz += BlurWieghts[3] * texture2D(tex_object4, uv).xyz;
        color.xyz += BlurWieghts[4] * texture2D(tex_object5, uv).xyz;
        color.xyz += BlurWieghts[5] * texture2D(tex_object6, uv).xyz;

        // TSM
        {
            // Shadow 
            vec4 tempShadowPos = (ShadowVP * vec4(Pos_, 1.0));
            tempShadowPos /= tempShadowPos.w;
            vec3 ShadowPos = tempShadowPos.xyz * 0.5 + 0.5;        // Transform NDC space coordinate from [-1.0 ~ 1.0] into [0.0 ~ 1.0].

            // Compute global scatter from modified TSM
            // TSMtap = (distance to light, u, v)
            vec4 TSMtap = texture2D(tex_object9, ShadowPos.xy);

            // Four average thicknesses through the object (in mm)
            vec4 blur2 = texture2D(tex_object2, uv);
            vec4 blur4 = texture2D(tex_object3, uv);
            vec4 blur8 = texture2D(tex_object4, uv);
            vec4 blur16 = texture2D(tex_object5, uv);

            vec4 thickness_mm = 1.0 * -(1.0 / 0.2) * log(vec4(blur2.w, blur4.w, blur8.w, blur16.w));
            vec2 stretchTap = texture2D(tex_object10, uv).xy;
            float stretchval = 0.5 * (stretchTap.x + stretchTap.y);
            vec4 a_values = vec4(0.433, 0.753, 1.412, 2.722);
            vec4 inv_a = -1.0 / (2.0 * a_values * a_values);
            vec4 fades = exp(thickness_mm * thickness_mm * inv_a);
            float textureScale = 1024 * 0.1 / stretchval;
            float blendFactor4 = clamp(textureScale * length(TexCoord_.xy - TSMtap.yz) / (a_values.y * 6.0), 0.0, 1.0);
            float blendFactor5 = clamp(textureScale * length(TexCoord_.xy - TSMtap.yz) / (a_values.z * 6.0), 0.0, 1.0);
            float blendFactor6 = clamp(textureScale * length(TexCoord_.xy - TSMtap.yz) / (a_values.w * 6.0), 0.0, 1.0);

            float normConst = 1.0;  // 현재는 weight 총합이 1 인것을 쓰므로 1로 고정

            vec2 TSMUV_For_Blur = TSMtap.yz;
            TSMUV_For_Blur.y = 1.0 - TSMUV_For_Blur.y;

            color.xyz += BlurWieghts[3] / normConst * fades.y * blendFactor4 * texture2D(tex_object4, TSMUV_For_Blur).xyz;
            color.xyz += BlurWieghts[4] / normConst * fades.z * blendFactor5 * texture2D(tex_object5, TSMUV_For_Blur).xyz;
            color.xyz += BlurWieghts[5] / normConst * fades.w * blendFactor6 * texture2D(tex_object6, TSMUV_For_Blur).xyz;
        }
    }

    color.w = 1.0;

    //color.xyz = texture2D(tex_object, uv).xyz;
    color.xyz *= pow(texture2D(tex_object7, TexCoord_).xyz, vec3(2.2));
    //color.xyz = color.xxx;
    //color.xyz = color.yyy;
    //color.xyz = color.zzz;

    //{
    //    vec3 viewDir = normalize(Eye - Pos_);
    //    vec3 normal = texture2D(tex_object8, TexCoord_).xyz * 2.0 - 1.0;
    //    normal = normalize(normal);

    //    jDirectionalLight light = DirectionalLight[0];

    //    vec3 LightPos = -light.LightDirection * 500;
    //    vec3 ToLight = normalize(LightPos - Pos_);

    //    float roughness = 0.3;
    //    float rho_s = 0.18;
    //    float sBRDF = KS_Skin_Specular(normal, ToLight, viewDir, roughness, rho_s); // White
    //    vec3 specularLight = vec3(sBRDF);
    //    color.xyz = specularLight + (vec3(1.0) - specularLight) * color.xyz;
    //}

    color.xyz = pow(color.xyz, vec3(1.0 / 2.2));

    //if (texture2D(tex_object10, uv).z <= 0.0)
    //    color.xyz = texture2D(tex_object2, uv).xyz;
}