#version 330 core

precision mediump float;

uniform sampler2D ColorTexture;
uniform int TextureSRGB[1];
uniform int UseTexture;

in vec2 TexCoord_;
in vec4 Color_;
in vec3 LocalPos_;

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
	vec2 uv = SampleSphericalMap(normalize(LocalPos_)); // make sure to normalize localPos
	color.xyz = texture(ColorTexture, uv).rgb;
	color.xyz = Uncharted2Tonemap(color.xyz);
	color.w = 1.0;
}