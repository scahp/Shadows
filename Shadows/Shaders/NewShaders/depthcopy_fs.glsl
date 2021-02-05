#version 330 core
precision mediump float;

uniform sampler2D TextureSampler;

in vec2 TexCoord_;

void main()
{
	gl_FragDepth = texture(TextureSampler, TexCoord_).x;
}
