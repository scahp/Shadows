#version 330 core
// Gaussian Blur Y

precision highp float;

uniform float Scale;
uniform float TextureSize;
uniform sampler2D tex_object;
uniform sampler2D tex_object2;		// StrechMap

in vec2 TexCoord_;
out vec4 color;

float packVec2(vec2 Data)
{
	return Data.x * 256.0 * 256.0 + Data.y;
}

vec2 unPackVec2(float Data)
{
	float temp = (Data / (256.0 * 256.0));
	float Second = fract(temp);
	float First = temp - Second;
	Second *= (256.0 * 256.0);
	return vec2(First, Second);
}

void main()
{
	float Step = Scale;
	Step *= texture2D(tex_object2, TexCoord_).y;		// Apply strechMap
	Step *= (1.0 / TextureSize);

	float weights[7] = float[7](
		0.006,
		0.061,
		0.242,
		0.383,
		0.242,
		0.061,
		0.006
		);

	vec2 coords = TexCoord_ - vec2(0.0, Step * 3.0);  // offset = ( 0.0, -3.0 )
	vec4 sum = vec4(0.0);
	vec2 sumAlpha = vec2(0.0);

	for (int i = 0; i < 7; ++i)
	{
		vec4 tap = texture2D(tex_object, coords);
		sum += weights[i] * tap;
		coords += vec2(0.0, Step * 1.0);
	}

	color = sum;
}