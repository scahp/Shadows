#include "common.glsl"

precision mediump float;

#if defined(USE_VSM) || defined(USE_EVSM)

#define VSM_AMOUNT_DIRECTIONAL 0.25
#define VSM_AMOUNT_OMNIDIRECTIONAL 0.25
float Linstep(float min, float max, float v)
{
    return clamp((v - min) / (max - min), 0.0, 1.0);
}

// Example 8-3. Applying a Light-Bleeding Reduction Function
// https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch08.html
float ReduceLightBleeding(float p_max, float Amount)
{
  // Remove the [0, Amount] tail and linearly rescale (Amount, 1].
   return Linstep(Amount, 1.0, p_max);
}
#endif // defined(USE_VSM) || defined(USE_EVSM)

//////////////////////////////////////////////////////////////////////
// Poisson Sample

#if defined(USE_POISSON_SAMPLE)

#define NUM_SAMPLES 17
#define NUM_RINGS 11
#define BLOCKER_SEARCH_NUM_SAMPLES NUM_SAMPLES
#define PCF_NUM_SAMPLES NUM_SAMPLES
#define PI2 (3.14 * 2.0)

vec2 poissonDisk[NUM_SAMPLES];

// https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float Rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

// https://threejs.org/examples/webgl_shadowmap_pcss.html
void InitPoissonSamples( const in vec2 randomSeed ) {
    float ANGLE_STEP = PI2 * float( NUM_RINGS ) / float( NUM_SAMPLES );
    float INV_NUM_SAMPLES = 1.0 / float( NUM_SAMPLES );

    // jsfiddle that shows sample pattern: https://jsfiddle.net/a16ff1p7/
    float angle = Rand( randomSeed ) * PI2;
    float radius = INV_NUM_SAMPLES;
    float radiusStep = radius;

    for( int i = 0; i < NUM_SAMPLES; i ++ ) {
        poissonDisk[i] = vec2( cos( angle ), sin( angle ) ) * pow( radius, 0.75 );
        radius += radiusStep;
        angle += ANGLE_STEP;
    }
}

#endif // USE_POISSON_SAMPLE

//////////////////////////////////////////////////////////////////////
#define BLOCKER_SEARCH_STEP_COUNT 3.0
#define PCF_FILTER_STEP_COUNT 3.0
#define BLOCKER_SEARCH_DIM (BLOCKER_SEARCH_STEP_COUNT * 2.0 + 1.0)
#define BLOCKER_SEARCH_COUNT (BLOCKER_SEARCH_DIM * BLOCKER_SEARCH_DIM)
#define PCF_DIM (PCF_FILTER_STEP_COUNT * 2.0 + 1.0)
#define PCF_COUNT (PCF_DIM * PCF_DIM)

//////////////////////////////////////////////////////////////////////
// Dirctional Light Shadow
#define SHADOW_BIAS_DIRECTIONAL 0.00005
#define SEARCH_RADIUS_DIRECTIONAL 10.0
bool IsInShadowMapSpace(vec2 clipPos)
{
    return (clipPos.x >= 0.0 && clipPos.x <= 1.0 && clipPos.y >= 0.0 && clipPos.y <= 1.0);
}

bool IsInShadowMapSpace(vec3 clipPos)
{
    return (clipPos.x >= 0.0 && clipPos.x <= 1.0 && clipPos.y >= 0.0 && clipPos.y <= 1.0 && clipPos.z >= 0.0 && clipPos.z <= 1.0);
}

// SSM + Directional
float IsShadowing(vec3 lightClipPos, sampler2DShadow shadow_object)
{
    if (IsInShadowMapSpace(lightClipPos))
        return texture(shadow_object, vec3(lightClipPos.xy, lightClipPos.z - SHADOW_BIAS_DIRECTIONAL));

    return 1.0;
}

vec2 SearchRegionRadiusUV(float zEye, float zLightNear, vec2 texelSize)         // Z Shadow Camera Space
{
    return SEARCH_RADIUS_DIRECTIONAL * texelSize * ((zEye - zLightNear) / zEye);
}

vec2 PenumbraRadiusUV(float zReceiver, float zBlocker, vec2 texelSize)
{
	float value = ((zReceiver - zBlocker) / zBlocker);
    return SEARCH_RADIUS_DIRECTIONAL * texelSize * clamp(value, 0.0, 1.0);
}

#if defined(USE_POISSON_SAMPLE)

#if defined(USE_PCF) || defined(USE_PCSS)
// SSM + PCF + Directional + Poisson
float PCF_PoissonSample(vec3 lightClipPos, vec2 radiusUV, sampler2DShadow shadow_object)
{
    float sum = 0.0;
    for(int i = 0; i < PCF_NUM_SAMPLES;++i) 
    {
        vec2 offset = poissonDisk[ i ] * radiusUV;
        vec3 depthPos = lightClipPos + vec3(offset, 0.0);
        sum += IsShadowing(depthPos, shadow_object);
    }
    for(int i = 0; i < PCF_NUM_SAMPLES;++i) 
    {
        vec2 offset = -poissonDisk[ i ].yx * radiusUV;
        vec3 depthPos = lightClipPos + vec3(offset, 0.0);
        sum += IsShadowing(depthPos, shadow_object);
    }
    return sum / ( 2.0 * float( PCF_NUM_SAMPLES ) );
}
#endif  // USE_PCF || USE_PCSS

#if defined(USE_PCSS)
// SSM + PCSS Blocker + Directional + Poisson
void FindBlocker_PoissonSample(out float accumBlockerDepth, out float numBlockers, vec3 lightClipPos, vec2 searchRegionRadiusUV, sampler2D shadow_object)
{
    for(int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) 
    {
        vec2 offset = poissonDisk[ i ] * searchRegionRadiusUV;
        vec3 depthPos = lightClipPos + vec3(offset, 0.0);
        if (IsInShadowMapSpace(depthPos))
        {
			float shadowMapDepth = texture(shadow_object, depthPos.xy).r;
			if (lightClipPos.z >= shadowMapDepth + SHADOW_BIAS_DIRECTIONAL)
			{
				accumBlockerDepth += shadowMapDepth;
				++numBlockers;
			}
        }
    }
}

// SSM + PCSS + Directional + Poisson
float PCSS_PoissonSample(vec3 lightClipPos, float shadowCameraDepth, sampler2D shadow_object_test, sampler2DShadow shadow_object, float zLightNear, vec2 texelSize)
{
    if (!IsInShadowMapSpace(lightClipPos))
        return 1.0;

    // 1. Blocker Search
    float accumBlockerDepth = 0.0;
    float numBlockers = 0.0;
    vec2 searchRegionRadiusUV = SearchRegionRadiusUV(shadowCameraDepth, zLightNear, texelSize);
    FindBlocker_PoissonSample(accumBlockerDepth, numBlockers, lightClipPos, searchRegionRadiusUV, shadow_object_test);

    // early out
    if (numBlockers == 0.0)
        return 1.0;
    else if (numBlockers >= BLOCKER_SEARCH_NUM_SAMPLES)
        return 0.0;

    // 2. Penumbra size
    float avgBlockerDepth = accumBlockerDepth / numBlockers;
    vec2 penumbraRadiusUV = PenumbraRadiusUV(lightClipPos.z, avgBlockerDepth, texelSize);

    // 3. PCF Filtering
    return PCF_PoissonSample(lightClipPos, penumbraRadiusUV, shadow_object);
}
#endif // USE_PCSS

#else   // USE_POISSON_SAMPLE

#if defined(USE_PCF) || defined(USE_PCSS)
// SSM + PCF + Directional
float PCF(vec3 lightClipPos, vec2 radiusUV, sampler2DShadow shadow_object)
{
    float sum = 0.0;
    float pcf_count = 0.0;
    vec2 stepUV = radiusUV / PCF_FILTER_STEP_COUNT;
    for (float x = -PCF_FILTER_STEP_COUNT; x <= PCF_FILTER_STEP_COUNT; ++x)
	{
		for (float y = -PCF_FILTER_STEP_COUNT; y <= PCF_FILTER_STEP_COUNT; ++y)
		{
            vec2 offset = vec2(x, y) * stepUV;
            vec3 depthPos = lightClipPos + vec3(offset, 0.0);
            pcf_count += IsShadowing(depthPos, shadow_object);
        }
    }

    return (pcf_count / PCF_COUNT);
}
#endif  // USE_PCSS || USE_PCSS

#if defined(USE_PCSS)
// SSM + PCSS Blocker + Directional
void FindBlocker(out float accumBlockerDepth, out float numBlockers, vec3 lightClipPos, vec2 searchRegionRadiusUV, sampler2D shadow_object)
{
    vec2 stepUV = searchRegionRadiusUV / BLOCKER_SEARCH_STEP_COUNT;
	for(float x = -BLOCKER_SEARCH_STEP_COUNT; x <= BLOCKER_SEARCH_STEP_COUNT; ++x)
    {
		for(float y = -BLOCKER_SEARCH_STEP_COUNT; y <= BLOCKER_SEARCH_STEP_COUNT; ++y)
        {
            vec2 offset = vec2(x, y) * stepUV;
            vec3 depthPos = lightClipPos + vec3(offset, 0.0);
            if (IsInShadowMapSpace(depthPos))
            {
				float shadowMapDepth = texture(shadow_object, depthPos.xy).r;
				if (lightClipPos.z >= shadowMapDepth + SHADOW_BIAS_DIRECTIONAL)
				{
					accumBlockerDepth += shadowMapDepth;
					++numBlockers;
				}
            }
        }
    }
}

// SSM + PCSS + Directional
float PCSS(vec3 lightClipPos, float shadowCameraDepth, sampler2D shadow_object_test, sampler2DShadow shadow_object, float zLightNear, vec2 texelSize)
{
    if (!IsInShadowMapSpace(lightClipPos))
        return 1.0;

    // 1. Blocker Search
    float accumBlockerDepth = 0.0;
    float numBlockers = 0.0;
    vec2 searchRegionRadiusUV = SearchRegionRadiusUV(shadowCameraDepth, zLightNear, texelSize);
    FindBlocker(accumBlockerDepth, numBlockers, lightClipPos, searchRegionRadiusUV, shadow_object_test);

    // early out
    if (numBlockers == 0.0)
        return 1.0;
    else if (numBlockers >= BLOCKER_SEARCH_COUNT)
        return 0.0;

    // 2. Penumbra size
    float avgBlockerDepth = accumBlockerDepth / numBlockers;
    vec2 penumbraRadiusUV = PenumbraRadiusUV(lightClipPos.z, avgBlockerDepth, texelSize);

    // 3. PCF Filtering
    return PCF(lightClipPos, penumbraRadiusUV, shadow_object);
}
#endif // USE_PCSS

#endif  // USE_POISSON_SAMPLE

#if defined(USE_VSM)
// VSM + Directional
float VSM(vec3 lightClipPos, vec3 lightPos, vec3 pos, sampler2D shadow_object)
{
    float lit = 1.0;
    if (IsInShadowMapSpace(lightClipPos))
    {
        vec3 toLight = lightPos - pos;
        float distFromLight = sqrt(dot(toLight, toLight));

        vec2 moments = texture(shadow_object, lightClipPos.xy).xy;
        float E_x2 = moments.y;
        float Ex_2 = moments.x * moments.x;
        float variance = E_x2 - Ex_2;    
        float mD = (moments.x - distFromLight);
        float mD_2 = mD * mD;
        float p = variance / (variance + mD_2);
        p = ReduceLightBleeding(p, VSM_AMOUNT_DIRECTIONAL);
        lit = max(p, float(distFromLight <= moments.x));
    }

    return lit;
}
#endif // USE_VSM

#if defined(USE_ESM)
// ESM + Directional
float ESM(vec3 lightClipPos, vec3 lightPos, vec3 pos, float near, float far, float ESM_C, sampler2D shadow_object)
{
    float lit = 1.0;
    if (IsInShadowMapSpace(lightClipPos))
    {
        vec3 toLight = lightPos - pos;
        float distFromLight = (sqrt(dot(toLight, toLight)) - near) / (far - near);
        float expCZ = texture(shadow_object, lightClipPos.xy).r;
        lit = clamp(exp(-distFromLight * ESM_C) * expCZ, 0.0, 1.0);
    }
    return lit;
}
#endif  // USE_ESM

#if defined(USE_EVSM)
// EVSM + Directional
float EVSM(vec3 lightClipPos, vec3 lightPos, vec3 pos, float near, float far, float ESM_C, sampler2D shadow_object)
{
    float lit = 1.0;
    if (IsInShadowMapSpace(lightClipPos))
    {
        vec3 toLight = lightPos - pos;
        float distFromLight = (sqrt(dot(toLight, toLight)) - near) / (far - near);
        float ed = exp(distFromLight * ESM_C);

        vec2 moments = texture(shadow_object, lightClipPos.xy).xy;
        float E_x2 = moments.y;
        float Ex_2 = moments.x * moments.x;
        float variance = E_x2 - Ex_2;    
        float mD = (moments.x - ed);
        float mD_2 = mD * mD;
        float p = variance / (variance + mD_2);
        p = ReduceLightBleeding(p, VSM_AMOUNT_DIRECTIONAL);
        lit = max(p, float(ed <= moments.x));
    }
    return lit;
}
#endif // USE_EVSM

///////////////////////////////////////////////////////////////////////////////////////////
// OmniDirectional Light Shadow
#define SHADOW_BIAS_OMNIDIRECTIONAL -0.005
#define SEARCH_RADIUS_OMNIDIRECTIONAL 0.05
// SSM + OmniDirectional
float IsShadowing(vec3 pos, vec3 lightPos, float maxDist, sampler2DShadow shadow_object)
{
	vec3 lightDir = pos - lightPos;
	float dist = sqrt(dot(lightDir, lightDir)) / maxDist + SHADOW_BIAS_OMNIDIRECTIONAL;

	TexArrayUV result = convert_xyz_to_texarray_uv(normalize(lightDir));
	return texture(shadow_object, vec3(Convert_TexArrayUV_To_Tex2dUV(result), dist));
}

vec2 SearchRegionRadius_OmniDirectional(float zEye, float zLightNear)         // Z Shadow Camera Space
{
    return SEARCH_RADIUS_OMNIDIRECTIONAL * vec2(1.0, 1.0) * ((zEye - zLightNear) / zEye);
}

vec2 PenumbraRadius_OmniDirectional(float zReceiver, float zBlocker)
{
	float value = ((zReceiver - zBlocker) / zBlocker);
    return SEARCH_RADIUS_OMNIDIRECTIONAL * vec2(1.0, 1.0) * clamp(value, 0.0, 1.0);
}

#if defined(USE_POISSON_SAMPLE)

#if defined(USE_PCF) || defined(USE_PCSS)
// SSM + PCF + OmiDirectional + Poisson
float PCF_OmniDirectional_PoissonSample(TexArrayUV result, float dist, vec2 radiusUV, sampler2DShadow shadow_object)
{
    float sum = 0.0;
    for(int i = 0; i < PCF_NUM_SAMPLES;++i) 
    {
        vec2 offset = poissonDisk[ i ] * radiusUV;
        TexArrayUV temp = result;
        temp.u += offset.x;
        temp.v += offset.y;
        temp = MakeTexArrayUV(temp);
        sum += texture(shadow_object, vec3(Convert_TexArrayUV_To_Tex2dUV(temp), dist));
    }
    for(int i = 0; i < PCF_NUM_SAMPLES;++i) 
    {
        vec2 offset = -poissonDisk[ i ].yx * radiusUV;
        TexArrayUV temp = result;
        temp.u += offset.x;
        temp.v += offset.y;
        temp = MakeTexArrayUV(temp);
		sum += texture(shadow_object, vec3(Convert_TexArrayUV_To_Tex2dUV(temp), dist));
    }
    return sum / ( 2.0 * float( PCF_NUM_SAMPLES ) );
}
float PCF_OmniDirectional_PoissonSample(vec3 pos, vec3 lightPos, float maxDist, vec2 radiusSquredUV, sampler2DShadow shadow_object)
{
    vec3 lightDir = pos - lightPos;
    float dist = sqrt(dot(lightDir, lightDir)) / maxDist + SHADOW_BIAS_OMNIDIRECTIONAL;

    TexArrayUV coord = convert_xyz_to_texarray_uv(normalize(lightDir));

    return PCF_OmniDirectional_PoissonSample(coord, dist, radiusSquredUV, shadow_object);
}
#endif  // USE_PCF || USE_PCSS

#if defined(USE_PCSS)
// SSM + PCSS Blocker + OmniDirectional + Poisson
void FindBlocker_OmniDirectional_PoissonSample(out float accumBlockerDepth, out float numBlockers, TexArrayUV pos, float distNormalized, vec2 searchRegionRadius, sampler2D shadow_object)
{
    for(int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; ++i) 
    {
        vec2 offset = poissonDisk[ i ] * searchRegionRadius;
        TexArrayUV temp = pos;
        temp.u += offset.x;
        temp.v += offset.y;
        temp = MakeTexArrayUV(temp);

        // if (!IsInShadowMapSpace(vec2(temp.u, temp.v)))
        //     continue;

        float shadowValue = texture(shadow_object, Convert_TexArrayUV_To_Tex2dUV(temp)).r;
        if (shadowValue <= distNormalized)
        {
            accumBlockerDepth += shadowValue;
            ++numBlockers;
        }
    }
}
// SSM + PCSS + OmniDirectional + Poisson
float PCSS_OmniDirectional_PoissonSample(vec3 pos, vec3 lightPos, float zLightNear, vec2 texelSize, float maxDist, sampler2DShadow shadow_object, sampler2D shadow_object_test)
{
    vec3 lightDir = pos - lightPos;
    float distSqured = dot(lightDir, lightDir);
	float dist = sqrt(distSqured);
    float distNormalized = dist / maxDist + SHADOW_BIAS_OMNIDIRECTIONAL;
    TexArrayUV result = convert_xyz_to_texarray_uv(normalize(lightDir));

    // 1. Blocker Search
    float accumBlockerDepth = 0.0;
    float numBlockers = 0.0;
    vec2 searchRegionRadius = SearchRegionRadius_OmniDirectional(dist, zLightNear);
    FindBlocker_OmniDirectional_PoissonSample(accumBlockerDepth, numBlockers, result, distNormalized, searchRegionRadius, shadow_object_test);

    if (numBlockers == 0.0)
        return 1.0;
    else if (numBlockers >= BLOCKER_SEARCH_COUNT)
       return 0.0;

    // 2. Penumbra size
    float avgBlockerDepthWorld = accumBlockerDepth / numBlockers;
    vec2 penumbraRadius = PenumbraRadius_OmniDirectional(distNormalized, avgBlockerDepthWorld);

    // // 3. Filtering
    return PCF_OmniDirectional_PoissonSample(result, distNormalized, penumbraRadius, shadow_object);
}
#endif // USE_PCSS

#else   // USE_POISSON_SAMPLE

#if defined(USE_PCF) || defined(USE_PCSS)
// SSM + PCF + OmiDirectional
float PCF_OmniDirectional(TexArrayUV result, float dist, vec2 radiusUV, sampler2DShadow shadow_object)
{
    float sum = 0.0;
    float pcf_count = 0.0;
    vec2 stepUV = radiusUV / PCF_FILTER_STEP_COUNT;
    for (float x = -PCF_FILTER_STEP_COUNT; x <= PCF_FILTER_STEP_COUNT; ++x)
	{
		for (float y = -PCF_FILTER_STEP_COUNT; y <= PCF_FILTER_STEP_COUNT; ++y)
		{
            vec2 offset = vec2(x, y) * stepUV;

            TexArrayUV temp = result;
            temp.u += offset.x;
            temp.v += offset.y;
            temp = MakeTexArrayUV(temp);
            pcf_count += texture(shadow_object, vec3(Convert_TexArrayUV_To_Tex2dUV(temp), dist));
        }
    }

    return (pcf_count / PCF_COUNT);
}

float PCF_OmniDirectional(vec3 pos, vec3 lightPos, float maxDist, vec2 radiusSquredUV, sampler2DShadow shadow_object)
{
    vec3 lightDir = pos - lightPos;
    float dist = sqrt(dot(lightDir, lightDir)) / maxDist + SHADOW_BIAS_OMNIDIRECTIONAL;

    TexArrayUV coord = convert_xyz_to_texarray_uv(normalize(lightDir));

    return PCF_OmniDirectional(coord, dist, radiusSquredUV, shadow_object);
}
#endif  // USE_PCF || USE_PCSS

#if defined(USE_PCSS)
// SSM + PCSS Blocker + OmniDirectional
void FindBlocker_OmniDirectional(out float accumBlockerDepth, out float numBlockers, TexArrayUV pos, float distNormalized, vec2 searchRegionRadius, sampler2D shadow_object)
{
    vec2 stepUV = searchRegionRadius / BLOCKER_SEARCH_STEP_COUNT;
	for(float x = -BLOCKER_SEARCH_STEP_COUNT; x <= BLOCKER_SEARCH_STEP_COUNT; ++x)
    {
		for(float y = -BLOCKER_SEARCH_STEP_COUNT; y <= BLOCKER_SEARCH_STEP_COUNT; ++y)
        {
            vec2 offset = vec2(x, y) * stepUV;
            TexArrayUV temp = pos;
            temp.u += offset.x;
            temp.v += offset.y;
            temp = MakeTexArrayUV(temp);

            // if (!IsInShadowMapSpace(vec2(temp.u, temp.v)))
            //     continue;

            float shadowValue = texture(shadow_object, Convert_TexArrayUV_To_Tex2dUV(temp)).r;
            if (shadowValue <= distNormalized)
            {
                accumBlockerDepth += shadowValue;
                ++numBlockers;
            }
        }
    }
}
// SSM + PCSS + OmniDirectional
float PCSS_OmniDirectional(vec3 pos, vec3 lightPos, float zLightNear, vec2 texelSize, float maxDist, sampler2DShadow shadow_object, sampler2D shadow_object_test)
{
    vec3 lightDir = pos - lightPos;
	float distSqured = dot(lightDir, lightDir);
	float dist = sqrt(distSqured);
	float distNormalized = dist / maxDist + SHADOW_BIAS_OMNIDIRECTIONAL;
    TexArrayUV result = convert_xyz_to_texarray_uv(normalize(lightDir));

    // 1. Blocker Search
    float accumBlockerDepth = 0.0;
    float numBlockers = 0.0;
    vec2 searchRegionRadius = SearchRegionRadius_OmniDirectional(dist, zLightNear);
    FindBlocker_OmniDirectional(accumBlockerDepth, numBlockers, result, distNormalized, searchRegionRadius, shadow_object_test);

    if (numBlockers == 0.0)
        return 1.0;
    else if (numBlockers >= BLOCKER_SEARCH_COUNT)
       return 0.0;

    // 2. Penumbra size
    float avgBlockerDepthWorld = accumBlockerDepth / numBlockers;
	vec2 penumbraRadius = PenumbraRadius_OmniDirectional(distNormalized, avgBlockerDepthWorld);

    // // 3. Filtering
    return PCF_OmniDirectional(result, distNormalized, penumbraRadius, shadow_object);
}
#endif	//USE_PCSS

#endif  // USE_POISSON_SAMPLE

#if defined(USE_VSM)
// VSM + OmniDirectional
float VSM_OmniDirectional(vec3 lightPos, vec3 pos, sampler2D shadow_object, float biasDistance)
{
    vec3 toLight = (lightPos - pos);
    float distFromLight = sqrt(dot(toLight, toLight));
    TexArrayUV result = convert_xyz_to_texarray_uv(normalize(-toLight));

    vec2 moments = texture(shadow_object, Convert_TexArrayUV_To_Tex2dUV(result)).yx;   // OmniDirectional ShadowMap saves (dist * dist, dist, 0.0, 1.0)
    float E_x2 = moments.y;
    float Ex_2 = moments.x * moments.x;
    float variance = E_x2 - Ex_2;
    float mD = (moments.x - distFromLight);

    // To alleviate artifacts that happend on the boundary of the textures, added distance bias.
    if (abs(mD) < biasDistance)
        mD = 0.0;

    float falloff = biasDistance;

    float mD_2 = mD * mD;
    float p = variance / (variance + mD_2) + falloff;
    p = ReduceLightBleeding(p, VSM_AMOUNT_OMNIDIRECTIONAL);
    return max(p, float(distFromLight <= moments.x));
}
#endif // USE_VSM

#if defined(USE_ESM)
// ESM + OmniDirectional
float ESM_OmniDirectional(vec3 lightPos, vec3 pos, float near, float far, float ESM_C, sampler2D shadow_object)
{
    vec3 toLight = (lightPos - pos);
    TexArrayUV result = convert_xyz_to_texarray_uv(normalize(-toLight));
    float expCZ = texture(shadow_object, Convert_TexArrayUV_To_Tex2dUV(result)).r;
    float distFromLight = (sqrt(dot(toLight, toLight)) - near) / (far - near);
    return clamp(exp(-distFromLight * ESM_C) * expCZ, 0.0, 1.0);
}
#endif  // USE_ESM

#if defined(USE_EVSM)
// EVSM + OmniDirectional
float EVSM_OmniDirectional(vec3 lightPos, vec3 pos, float near, float far, float ESM_C, sampler2D shadow_object, float biasDistance)
{
    vec3 toLight = (lightPos - pos);
    float distFromLight = (sqrt(dot(toLight, toLight)) - near) / (far - near);
    float ed = exp(distFromLight * ESM_C);
    TexArrayUV result = convert_xyz_to_texarray_uv(normalize(-toLight));

    vec2 moments = texture(shadow_object, Convert_TexArrayUV_To_Tex2dUV(result)).yx;   // OmniDirectional ShadowMap saves (dist * dist, dist, 0.0, 1.0)
    float E_x2 = moments.y;
    float Ex_2 = moments.x * moments.x;
    float variance = E_x2 - Ex_2;
    float mD = (moments.x - ed);

    // To alleviate artifacts that happend on the boundary of the textures, added distance bias.
    if (abs(mD) < biasDistance)
        mD = 0.0;

    float falloff = biasDistance;

    float mD_2 = mD * mD;
    float p = variance / (variance + mD_2) + falloff;
    p = ReduceLightBleeding(p, VSM_AMOUNT_OMNIDIRECTIONAL);
    return max(p, float(ed <= moments.x));
}
#endif // USE_EVSM
