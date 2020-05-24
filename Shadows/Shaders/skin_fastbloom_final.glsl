#version 330 core
// Gaussian Blur Y

precision highp float;

uniform sampler2D tex_object;
uniform sampler2D tex_object2;
uniform float WeightA;
uniform float WeightB;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	vec4 A = texture2D(tex_object, TexCoord_);
	vec4 B = texture2D(tex_object2, TexCoord_);

	color = WeightA * A + WeightB * B;
}