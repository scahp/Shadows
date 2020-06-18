#version 330 core

precision mediump float;

uniform sampler2D tex_object;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	float sample0 = textureOffset(tex_object, TexCoord_, ivec2(-1, -1)).x;
	float sample1 = textureOffset(tex_object, TexCoord_, ivec2( 0, -1)).x;
	float sample2 = textureOffset(tex_object, TexCoord_, ivec2( 1, -1)).x;

	float sample3 = textureOffset(tex_object, TexCoord_, ivec2(-1, 0)).x;
	float sample4 = textureOffset(tex_object, TexCoord_, ivec2( 0, 0)).x;
	float sample5 = textureOffset(tex_object, TexCoord_, ivec2( 1, 0)).x;

	float sample6 = textureOffset(tex_object, TexCoord_, ivec2(-1, 1)).x;
	float sample7 = textureOffset(tex_object, TexCoord_, ivec2( 0, 1)).x;
	float sample8 = textureOffset(tex_object, TexCoord_, ivec2( 1, 1)).x;

	float result = sample0 + sample1 + sample2;
	result += sample3 + sample4 + sample5;
	result += sample6 + sample7 + sample8;

	result /= 9.0;
	color.x = result;
}