#version 330 core

precision mediump float;

uniform sampler2D tex_object;
uniform int TextureSRGB[1];
uniform int UseOutline;
in vec2 TexCoord_;

out vec4 color;

void main()
{
    color = texture2D(tex_object, TexCoord_);
	
	if (UseOutline > 0)
	{
		if (color.r < 0.4)
			discard;
		
		if (color.r < 0.47) 
			color = vec4(1.0, 0.0, 0.0, 1.0);
		else 
			color = vec4(1.0, 1.0, 1.0, 1.0);
	}
	else
	{
		if (color.r < 0.5)
			discard;

		color = vec4(1, 1, 1, 1);
	}
	
	if (TextureSRGB[0] > 0)
		color.xyz = pow(color.xyz, vec3(2.2));
}