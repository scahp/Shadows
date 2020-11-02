#version 330 core
precision mediump float;

uniform sampler2D tex_object2;		// Noise

in vec4 TexCoord0_;
in vec4 TexCoord1_;
in vec4 DiffuseColor_;
in vec4 SpecularColor_;

out vec4 FragColor;

void main()
{
	vec4 noise0 = texture(tex_object2, TexCoord0_.xy);
	vec4 noise1 = texture(tex_object2, TexCoord1_.xy);

	vec4 result;
	result = noise0 - vec4(0.5) + noise1 - vec4(0.5);
	result.a = noise0.a + noise1.a;
	FragColor = result * DiffuseColor_ + SpecularColor_;
}
