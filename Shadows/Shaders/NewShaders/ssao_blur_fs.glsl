#version 330 core

precision mediump float;

uniform sampler2D SSAO_Input;
uniform float SSAO_Input_Width;
uniform float SSAO_Input_Height;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	vec2 texelSize = 1.0 / vec2(SSAO_Input_Width, SSAO_Input_Height);
	float result = 0.0;
	for (int x = -2; x < 2; ++x)
	{ 
		for (int y = -2; y < 2; ++y)
		{ 
			vec2 offset = vec2(float(x), float(y)) * texelSize; 
			result += texture(SSAO_Input, TexCoord_ + offset).r;
		} 
	} 
	color.x = result / (4.0 * 4.0);
}