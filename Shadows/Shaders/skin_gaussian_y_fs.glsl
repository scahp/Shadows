#version 330 core
// Gaussian Blur Y

precision highp float;

uniform float Scale;
uniform float TextureSize;
uniform sampler2D tex_object;
uniform sampler2D tex_object2;		// StrechMap

in vec2 TexCoord_;

out vec4 color;

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

	vec4 tap0 = texture2D(tex_object, coords);
	sum = weights[0] * tap0;
	coords += vec2(0.0, Step * 1.0); // offset =  0.0, -2.0

	vec4 tap1 = texture2D(tex_object, coords);
	sum += weights[1] * tap1;
	coords += vec2(0.0, Step * 1.0); // offset =  0.0, -1.0

	vec4 tap2 = texture2D(tex_object, coords);
	sum += weights[2] * tap2;
	coords += vec2(0.0, Step * 1.0); // offset = 0.0, 0.0

	vec4 tap3 = texture2D(tex_object, coords);
	sum += weights[3] * tap3;
	coords += vec2(0.0, Step * 1.0); // offset = 0.0, 1.0

	vec4 tap4 = texture2D(tex_object, coords);
	sum += weights[4] * tap4;
	coords += vec2(0.0, Step * 1.0); // offset = 0.0, 2.0

	vec4 tap5 = texture2D(tex_object, coords);
	sum += weights[5] * tap5;
	coords += vec2(0.0, Step * 1.0); // offset = 0.0, 3.0

	vec4 tap6 = texture2D(tex_object, coords);
	sum += weights[6] * tap6;

	color = sum;
}