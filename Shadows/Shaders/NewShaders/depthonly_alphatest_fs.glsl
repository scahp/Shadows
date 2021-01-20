#version 330 core

uniform sampler2D DiffuseSampler;
uniform int UseOpacitySampler;

in vec2 TexCoord_;

void main()
{
	if (UseOpacitySampler > 0)
	{
		float Opacity = texture(DiffuseSampler, TexCoord_).a;
		if (Opacity <= 0.1)
			discard;
	}
}