#version 330 core
precision mediump float;

uniform sampler2D ColorSampler;

in vec2 TexCoord_;
out vec4 color;

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
vec3 ACESFilmTonemap(vec3 x)
{
	float a = 2.51f;
	float b = 0.03f;
	float c = 2.43f;
	float d = 0.59f;
	float e = 0.14f;
	return clamp((x * (a * x + b)) / (x * (c * x + d) + e), vec3(0.0), vec3(1.0));
}

void main()
{
	color = vec4(texture(ColorSampler, TexCoord_).xyz, 1.0);
	color.xyz = ACESFilmTonemap(color.xyz);

	color.xyz = pow(color.xyz, vec3(1.0 / 2.2));		// from Linear color to sRGB
}