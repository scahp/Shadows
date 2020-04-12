#version 330 core

precision mediump float;

uniform sampler2D tex_object;
uniform int TextureSRGB[1];
in vec2 TexCoord_;

out vec4 color;

void main()
{
    color = texture2D(tex_object, TexCoord_);

	if (color.a < 0.5)		// Alpha Test Threshold
		discard;

	if (TextureSRGB[0] > 0)
		color.xyz = pow(color.xyz, vec3(2.2));
}