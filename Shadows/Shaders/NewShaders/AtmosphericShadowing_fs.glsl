#version 330 core
#preprocessor

precision mediump float;

uniform sampler2D PosSampler;

uniform vec2 PixelSize;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	color = vec4(texture(PosSampler, TexCoord_).xyz, 1.0);
}