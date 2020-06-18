#version 330 core

precision mediump float;

uniform sampler2D tex_object;		// EdgeDetectionSobel
uniform sampler2D tex_object2;		// LightVolumeTexture
uniform float CoarseTextureWidthInv;
uniform float CoarseTextureHeightInv;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	vec2 gradient = texture(tex_object, TexCoord_).xy;
	
	vec2 offset;
	offset.x = CoarseTextureWidthInv * gradient.y;
	offset.y = CoarseTextureHeightInv * gradient.x;

	float result = 0.0f;

	for (int iSample = -7; iSample < 8; iSample++)
		result += texture(tex_object2, TexCoord_ + offset * iSample).x;

	result *= (1.0 / 15.0);
	color.x = result;
}