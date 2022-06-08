#version 330 core

precision mediump float;

uniform sampler2D ColorTexture;
uniform int TextureSRGB[1];
uniform int UseTexture;
uniform vec3 WorldSpace_ViewDir_ToSurface;
uniform vec3 WorldSpace_CameraPos;

in vec2 TexCoord_;
in vec4 Color_;
in vec3 LocalPos_;
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

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main()
{
//	vec2 uv = SampleSphericalMap(normalize(LocalPos_)); // make sure to normalize localPos
//	color.xyz = texture(ColorTexture, uv).rgb;
//	color.xyz = Uncharted2Tonemap(color.xyz);
//	color.w = 1.0;

    vec3 TangentSpace_ViewDir_ToSurface_ = TBN_ * normalize(WorldPos_ - WorldSpace_CameraPos);
    //TangentSpace_ViewDir_ToSurface_ = WorldSpace_CameraPos;

    // raytrace box from tangent view dir
    // https://chulin28ho.tistory.com/521
    vec3 pos = vec3(TexCoord_ * 2.0 - 1.0, -1.0);
    vec3 id = 1.0 / (TangentSpace_ViewDir_ToSurface_+0.00001);
    vec3 k = abs(id) - pos * id;
    float kMin = min(min(k.x, k.y), k.z);
    pos += kMin * TangentSpace_ViewDir_ToSurface_;
    
    color.xyz = pos;
    color.w = 1;
    //return;
    vec2 uv = SampleSphericalMap(normalize(pos));
    color.xyz = texture(ColorTexture, uv).rgb;
	color.xyz = Uncharted2Tonemap(color.xyz);
	color.w = 1.0;
}