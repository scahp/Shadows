#version 430 core
// Gaussian Blur X

precision highp float;

uniform float Scale;
uniform float TextureSize;
uniform sampler2D tex_object;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	float Step = Scale;
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

	vec2 coords = TexCoord_ - vec2(Step * 3.0, 0.0);  // offset = ( -3.0, 0.0 )
	vec4 sum = vec4(0.0);
	vec2 sumAlpha = vec2(0.0);

	for (int i = 0; i < 7; ++i)
	{
		vec4 tap = texture2D(tex_object, coords);
		sum += weights[i] * tap;
		coords += vec2(Step * 1.0, 0.0);
	}

	color = sum;
}