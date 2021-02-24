#version 330 core
precision mediump float;

uniform sampler2D TextureSampler;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	color = texture(TextureSampler, TexCoord_);
}
