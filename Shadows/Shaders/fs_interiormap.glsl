#version 330 core

precision mediump float;

uniform sampler2D ColorTexture;
uniform vec3 WorldSpace_CameraPos;

in vec2 TexCoord_;
in mat3 TBN_;
in vec3 WorldPos_;

out vec4 color;

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;		// Shoulder Strength
	float B = 0.50;		// Linear Strength
	float C = 0.10;		// Linear Angle
	float D = 0.20;		// Toe Strength
	float E = 0.02;		// Toe Numerator
	float F = 0.30;		// Toe Denominator
	float W = 11.2;		// Linear White Point Value

	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

// https://learnopengl.com/PBR/IBL/Diffuse-irradiance
// Equirectangular map
const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

vec3 GetEnvCube(vec2 uv, vec3 TangentSpace_ViewDir_ToSurface_)
{
    // raytrace box from tangent view dir https://chulin28ho.tistory.com/521
    TangentSpace_ViewDir_ToSurface_.z = abs(TangentSpace_ViewDir_ToSurface_.z);
    
    vec3 pos = vec3(uv * 2.0 - 1.0, -1.0);
    vec3 id = 1.0 / (TangentSpace_ViewDir_ToSurface_ + 0.00001);
    vec3 k = abs(id) - pos * id;
    float kMin = min(min(k.x, k.y), k.z);
    pos += kMin * TangentSpace_ViewDir_ToSurface_;

    return texture(ColorTexture, SampleSphericalMap(normalize(pos))).rgb;
}

void main()
{
    vec3 TangentSpace_ViewDir_ToSurface_ = TBN_ * normalize(WorldPos_ - WorldSpace_CameraPos);
    
    color.xyz = GetEnvCube(TexCoord_, TangentSpace_ViewDir_ToSurface_);
    color.xyz = Uncharted2Tonemap(color.xyz);
    color.w = 1.0;
}