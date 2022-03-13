#version 330 core

precision mediump float;

uniform sampler2D tex_object;
uniform sampler2D tex_object7;		// Ambient Occlusion map

uniform int TextureSRGB[1];
uniform int UseTexture;
uniform float AmbientOcclusionScale;

in vec2 TexCoord_;
in vec4 Color_;

out vec4 color;

float ApplyAmbientOcclusion(vec2 uv)
{
	// To adjust cos(a) in shader, AmbientMap has radian.
	return cos(texture2D(tex_object7, uv).x * AmbientOcclusionScale);
}

void main()
{
	if (UseTexture > 0)
	{
		color = texture2D(tex_object, TexCoord_);
		if (TextureSRGB[0] > 0)
			color.xyz = pow(color.xyz, vec3(2.2));
	}
	else
	{
		color = Color_;
	}

	float LightIntensity = ApplyAmbientOcclusion(TexCoord_);
	color.xyz *= vec3(LightIntensity);
}