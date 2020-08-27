#version 330 core
precision mediump float;

uniform sampler2D tex_object;
uniform float Exposure;

in vec2 TexCoord_;
out vec4 FragColor;

void main()
{
	vec4 TexColor = texture(tex_object, TexCoord_);
	FragColor = 1.0 - exp(TexColor * -Exposure);
}