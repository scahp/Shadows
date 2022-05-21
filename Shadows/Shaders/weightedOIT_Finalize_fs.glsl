#version 330 core
precision mediump float;

uniform sampler2D tex_object;
uniform sampler2D tex_object2;

in vec2 TexCoord_;
out vec4 FragColor;

void main()
{
	vec4 accum = texture(tex_object, TexCoord_);
	float r = texture(tex_object2, TexCoord_).r;

	FragColor = vec4(accum.rgb / clamp(accum.a, 1e-4, 5e4), r);
}
