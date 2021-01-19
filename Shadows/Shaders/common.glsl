
precision mediump float;

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

struct jPointLight
{
    vec3 LightPos;
    float SpecularPow;
    vec3 Color;
    float MaxDistance;
    vec3 DiffuseLightIntensity;
    vec3 SpecularLightIntensity;
};

struct jSpotLight
{
    vec3 LightPos;
    float MaxDistance;
    vec3 Direction;
    float PenumbraRadian;
    vec3 Color;
    float UmbraRadian;
    vec3 DiffuseLightIntensity;
    float SpecularPow;
    vec3 SpecularLightIntensity;
};

struct jMaterial
{
    vec3 Ambient;
    float Opacity;
    vec3 Diffuse;
    float Reflectivity;
    vec3 Specular;
    float SpecularShiness;
    vec3 Emissive;
    float IndexOfRefraction;		// 1.0 means lights will not refract
};

struct jShadowData
{
	mat4 ShadowVP;
	mat4 ShadowV;
	vec3 LightPos;
	float Near;
	float Far;
};

vec3 TransformPos(mat4 m, vec3 v)
{
    return (m * vec4(v, 1.0)).xyz;
}

vec3 TransformNormal(mat4 m, vec3 v)
{
    return (m * vec4(v, 0.0)).xyz;
}

float WindowingFunction(float value, float maxValue)
{
    return pow(max(0.0, 1.0 - pow(value / maxValue, 4.0)), 2.0);
}

float DistanceAttenuation(float distance, float maxDistance)
{
    const float refDistance = 50.0;
    float attenuation = (refDistance * refDistance) / ((distance * distance) + 1.0);
    return attenuation * WindowingFunction(distance, maxDistance);
}

float DiretionalFalloff(float lightRadian, float penumbraRadian, float umbraRadian)
{
    float t = clamp((cos(lightRadian) - cos(umbraRadian)) / (cos(penumbraRadian) - cos(umbraRadian)), 0.0, 1.0);
    return t * t;
}

vec3 GetAmbientLight(jAmbientLight light)
{
    return light.Color * light.Intensity;
}

vec3 GetDirectionalLightDiffuse(jDirectionalLight light, vec3 normal)
{
    return light.Color * clamp(dot(-light.LightDirection, normal), 0.0, 1.0) * light.DiffuseLightIntensity;
}

vec3 GetDirectionalLightSpecular(jDirectionalLight light, vec3 reflectLightDir, vec3 viewDir)
{
    return light.Color * pow(clamp(dot(reflectLightDir, viewDir), 0.0, 1.0), light.SpecularPow) * light.SpecularLightIntensity;
}

vec3 GetDirectionalLight(jDirectionalLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.LightDirection);
    vec3 reflectLightDir = 2.0 * clamp(dot(lightDir, normal), 0.0, 1.0) * normal - lightDir;
    return (GetDirectionalLightDiffuse(light, normal) + GetDirectionalLightSpecular(light, reflectLightDir, viewDir));
}

vec3 GetPointLightDiffuse(jPointLight light, vec3 normal, vec3 lightDir)
{
    return light.Color * clamp(dot(lightDir, normal), 0.0, 1.0) * light.DiffuseLightIntensity;
}

vec3 GetPointLightSpecular(jPointLight light, vec3 reflectLightDir, vec3 viewDir)
{
    return light.Color * pow(clamp(dot(reflectLightDir, viewDir), 0.0, 1.0), light.SpecularPow) * light.SpecularLightIntensity;
}

vec3 GetPointLight(jPointLight light, vec3 normal, vec3 pixelPos, vec3 viewDir)
{
    vec3 lightDir = light.LightPos - pixelPos;
    float pointLightDistance = length(lightDir);
    lightDir = normalize(lightDir);
    vec3 reflectLightDir = 2.0 * clamp(dot(lightDir, normal), 0.0, 1.0) * normal - lightDir;
   
    return (GetPointLightDiffuse(light, normal, lightDir) + GetPointLightSpecular(light, reflectLightDir, viewDir)) * DistanceAttenuation(pointLightDistance, light.MaxDistance);
}

vec3 GetSpotLightDiffuse(jSpotLight light, vec3 normal, vec3 lightDir)
{
    return light.Color * clamp(dot(lightDir, normal), 0.0, 1.0) * light.DiffuseLightIntensity;
}

vec3 GetSpotLightSpecular(jSpotLight light, vec3 reflectLightDir, vec3 viewDir)
{
	return light.Color * pow(clamp(dot(reflectLightDir, viewDir), 0.0, 1.0), light.SpecularPow) * light.SpecularLightIntensity;
}

vec3 GetSpotLight(jSpotLight light, vec3 normal, vec3 pixelPos, vec3 viewDir)
{
    vec3 lightDir = light.LightPos - pixelPos;
    float distance = length(lightDir);
    lightDir = normalize(lightDir);
    vec3 reflectLightDir = 2.0 * clamp(dot(lightDir, normal), 0.0, 1.0) * normal - lightDir;

    float lightRadian = acos(dot(lightDir, -light.Direction));

    return (GetSpotLightDiffuse(light, normal, lightDir)
     + GetSpotLightSpecular(light, reflectLightDir, viewDir))
      * DistanceAttenuation(distance, light.MaxDistance)
      * DiretionalFalloff(lightRadian, light.PenumbraRadian, light.UmbraRadian);
}

vec4 EncodeFloat(float depth)
{
    const vec4 bitShift = vec4(256 * 256 * 256, 256 * 256, 256, 1.0);
    const vec4 bitMask = vec4(0, 1.0 / 256.0, 1.0 / 256.0, 1.0 / 256.0);
    vec4 comp = fract(depth * bitShift);
    comp -= comp.xxyz * bitMask;
    return comp;
}

float DecodeFloat(vec4 color)
{
    const vec4 bitShift = vec4(1.0 / (256.0 * 256.0 * 256.0), 1.0 / (256.0 * 256.0), 1.0 / 256.0, 1.0);
    return dot(color, bitShift);
}

struct TexArrayUV
{
  int index;
  float u;
  float v;
};

TexArrayUV convert_xyz_to_texarray_uv(vec3 direction)
{
    TexArrayUV result;

    float x = direction.x;
    float y = direction.y;
    float z = direction.z;

    float absX = abs(x);
    float absY = abs(y);
    float absZ = abs(z);

    bool isXPositive = x > 0.0 ? true : false;
    bool isYPositive = y > 0.0 ? true : false;
    bool isZPositive = z > 0.0 ? true : false;

    float maxAxis;
    float uc;
    float vc;

    if (absX >= absY && absX >= absZ)
    {
        // POSITIVE X
        if (isXPositive) 
        {
            // u (0 to 1) goes from +z to -z
            // v (0 to 1) goes from -y to +y
            maxAxis = absX;
            uc = -z;
            vc = y;
            result.index = 0;
        }
        else    // NEGATIVE X
        {
            // u (0 to 1) goes from -z to +z
            // v (0 to 1) goes from -y to +y
            maxAxis = absX;
            uc = z;
            vc = y;
            result.index = 1;
        }
    }

    if (absY >= absX && absY >= absZ)
    {
        // POSITIVE Y
        if (isYPositive) 
        {
            // u (0 to 1) goes from -x to +x
            // v (0 to 1) goes from +z to -z
            maxAxis = absY;
            uc = x;
            vc = -z;
            result.index = 2;
        }
        else // NEGATIVE Y
        {
            // u (0 to 1) goes from -x to +x
            // v (0 to 1) goes from -z to +z
            maxAxis = absY;
            uc = x;
            vc = z;
            result.index = 3;
        }
    }

    if (absZ >= absX && absZ >= absY)
    {
        // POSITIVE Z
        if (isZPositive) 
        {
            // u (0 to 1) goes from -x to +x
            // v (0 to 1) goes from -y to +y
            maxAxis = absZ;
            uc = x;
            vc = y;
            result.index = 4;
        }
        else // NEGATIVE Z
        {
            // u (0 to 1) goes from +x to -x
            // v (0 to 1) goes from -y to +y
            maxAxis = absZ;
            uc = -x;
            vc = y;
            result.index = 5;
        }
    }

    // Convert range from -1 to 1 to 0 to 1
    result.u = 1.0-(0.5 * (uc / maxAxis + 1.0));
    result.v = 0.5 * (vc / maxAxis + 1.0);
    return result;
}

vec2 Convert_TexArrayUV_To_Tex2dUV(TexArrayUV uv)
{
	uv.v = uv.v / 6.0 + float(uv.index) * 1.0 / 6.0;
	return vec2(uv.u, uv.v);
}

vec2 convert_xyz_to_tex2d_uv(vec3 direction)		// TEXTURE SIZE => (N) x (N * 6) 
{
	TexArrayUV result = convert_xyz_to_texarray_uv(direction);
	result.v = result.v / 6.0 + float(result.index) * 1.0 / 6.0;
	return vec2(result.u, result.v);
}

TexArrayUV MakeTexArrayUV(TexArrayUV uv)
{
    //for(int i=0;i<2;++i)        // to cover uv is out of range simultaneously. loop over 2 times.
    {
        if (uv.index == 0)            // Positive X
        {
            if (uv.u > 1.0)
            {
                uv.u = uv.u - 1.0;
                uv.index = 4;
            }
            else if (uv.u < 0.0)
            {
                uv.u = 1.0 + uv.u;
                uv.index = 5;
            }            
            else if (uv.v > 1.0)
            {
                uv.u = 1.0 - uv.u;
                uv.v = 2.0 - uv.v;
                uv.index = 2;
            }
            else if (uv.v < 0.0)
            {
                float t = uv.u;
                uv.u = -uv.v;
                uv.v = t;
                uv.index = 3;
            }
        }
        else if (uv.index == 1)       // Negative X
        {
            if (uv.u > 1.0)
            {
				uv.u = uv.u - 1.0;
                uv.index = 5;
            }
            else if (uv.u < 0.0)
            {
                uv.u = 1.0 + uv.u;
                uv.index = 4;
            }            
            else if (uv.v > 1.0)
            {
                uv.v = uv.v - 1.0;
                uv.index = 2;
            }
            else if (uv.v < 0.0)
            {
                float t = uv.u;
                uv.u = 1.0 + uv.v;
                uv.v = 1.0 - t;
                uv.index = 3;
            }
        }
        else if (uv.index == 2)       // Positive Y
        {
            if (uv.u > 1.0)
            {
                float t = uv.u;
                uv.u = uv.v;
                uv.v = 2.0 - t;
                uv.index = 5;
            }
            else if (uv.u < 0.0)
            {
                float t = uv.u;
                uv.u = 1.0 - uv.v;
                uv.v = 1.0 + t;
                uv.index = 4;
            }            
            else if (uv.v > 1.0)
            {
                uv.u = 1.0 - uv.u;
                uv.v = 2.0 - uv.v;
                uv.index = 0;
            }
            else if (uv.v < 0.0)
            {
                uv.v = 1.0 + uv.v;
                uv.index = 1;
            }
        }
        else if (uv.index == 3)       // Negative Y
        {
            if (uv.u > 1.0)
            {
                float t = uv.u;
                uv.u = 1.0 - uv.v;
                uv.v = t - 1.0;
                uv.index = 1;
            }
            else if (uv.u < 0.0)
            {
                float t = uv.u;
                uv.u = uv.v;
                uv.v = -t;
                uv.index = 0;
            }            
            else if (uv.v > 1.0)
            {
				uv.v = uv.v - 1.0;
                uv.index = 4;
            }
            else if (uv.v < 0.0)
            {
                uv.u = 1.0 - uv.u;
                uv.v = -uv.v;
                uv.index = 5;
            }
        }
        else if (uv.index == 4)       // Positive Z
        {
            if (uv.u > 1.0)
            {
                uv.u = uv.u - 1.0;
                uv.index = 1;
            }
            else if (uv.u < 0.0)
            {
                uv.u = 1.0 + uv.u;
                uv.index = 0;
            }            
            else if (uv.v > 1.0)
            {
				float t = uv.u;
                uv.u = uv.v - 1.0;
				uv.v = 1.0 - t;
                uv.index = 2;
            }
            else if (uv.v < 0.0)
            {
                uv.v = 1.0 + uv.v;
                uv.index = 3;
            }
        }
        else if (uv.index == 5)       // Negative Z
        {
            if (uv.u > 1.0)
            {
				uv.u = uv.u - 1.0;
                uv.index = 0;
            }
            else if (uv.u < 0.0)
            {
                uv.u = 1.0 + uv.u;
                uv.index = 1;
            }            
            else if (uv.v > 1.0)
            {
				float t = uv.u;
                uv.u = 2.0 - uv.v;
				uv.v = t;
                uv.index = 2;
            }
            else if (uv.v < 0.0)
            {
                uv.u = 1.0 - uv.u;
                uv.v = -uv.v;
                uv.index = 3;
            }
        }
    }
    return uv;
}

vec2 make_tex2d_uv(vec2 uv)
{
	float inv6 = 1.0 / 6.0;

	int index = int(floor(uv.y / inv6));// + float(uv.index) * 1.0 / 6.0;
	uv.y -= float(index) * inv6;
	uv.y *= 6.0;

    //for(int i=0;i<2;++i)        // to cover uv is out of range simultaneously. loop over 2 times.
    {
        if (index == 0)            // Positive X
        {
            if (uv.x > 1.0)
            {
                uv.x = 1.0 - uv.x;
                index = 4;
            }
            else if (uv.x < 0.0)
            {
                uv.x = 1.0 + uv.x;
                index = 5;
            }            
            else if (uv.y > 1.0)
            {
                float t = uv.x;
                uv.x = uv.y - 1.0;
                uv.y = 1.0 - t;
                index = 2;
            }
            else if (uv.y < 0.0)
            {
                float t = uv.x;
                uv.x = -uv.y;
                uv.y = t;
                index = 3;
            }
        }
        else if (index == 1)       // Negative X
        {
            if (uv.x > 1.0)
            {
                uv.x = 1.0 - uv.x;
                index = 5;
            }
            else if (uv.x < 0.0)
            {
                uv.x = 1.0 + uv.x;
                index = 4;
            }            
            else if (uv.y > 1.0)
            {
                float t = uv.x;
                uv.x = 2.0 - uv.y;
                uv.y = t;
                index = 2;
            }
            else if (uv.y < 0.0)
            {
                float t = uv.x;
                uv.x = 1.0 + uv.y;
                uv.y = 1.0 - t;
                index = 3;
            }
        }
        else if (index == 2)       // Positive Y
        {
            if (uv.x > 1.0)
            {
                float t = uv.x;
                uv.x = uv.y;
                uv.y = 2.0 - t;
                index = 1;
            }
            else if (uv.x < 0.0)
            {
                float t = uv.x;
                uv.x = 1.0 - uv.y;
                uv.y = 1.0 + t;
                index = 0;
            }            
            else if (uv.y > 1.0)
            {
                uv.x = 1.0 - uv.x;
                uv.y = 2.0 - uv.y;
                index = 5;
            }
            else if (uv.y < 0.0)
            {
                uv.y = 1.0 + uv.y;
                index = 4;
            }
        }
        else if (index == 3)       // Negative Y
        {
            if (uv.x > 1.0)
            {
                float t = uv.x;
                uv.x = 1.0 - uv.y;
                uv.y = 1.0 - t;
                index = 1;
            }
            else if (uv.x < 0.0)
            {
                float t = uv.x;
                uv.x = uv.y;
                uv.y = -t;
                index = 0;
            }            
            else if (uv.y > 1.0)
            {
                uv.y = 1.0 - uv.y;
                index = 4;
            }
            else if (uv.y < 0.0)
            {
                uv.x = 1.0-uv.x;
                uv.y = -uv.y;
                index = 5;
            }
        }
        else if (index == 4)       // Positive Z
        {
            if (uv.x > 1.0)
            {
                uv.x = 1.0 - uv.x;
                index = 1;
            }
            else if (uv.x < 0.0)
            {
                uv.x = 1.0 + uv.x;
                index = 0;
            }            
            else if (uv.y > 1.0)
            {
                uv.y = uv.y - 1.0;
                index = 2;
            }
            else if (uv.y < 0.0)
            {
                uv.y = 1.0 + uv.y;
                index = 3;
            }
        }
        else if (index == 5)       // Negative Z
        {
            if (uv.x > 1.0)
            {
                uv.x = 1.0 - uv.x;
                index = 0;
            }
            else if (uv.x < 0.0)
            {
                uv.x = 1.0 + uv.x;
                index = 1;
            }            
            else if (uv.y > 1.0)
            {
                uv.x = 1.0 - uv.x;
                uv.y = 2.0 - uv.y;
                index = 2;
            }
            else if (uv.y < 0.0)
            {
                uv.x = 1.0 - uv.x;
                uv.y = -uv.y;
                index = 3;
            }
        }
    }

	uv.y = uv.y * inv6 + float(index) * inv6;

    return uv;
}