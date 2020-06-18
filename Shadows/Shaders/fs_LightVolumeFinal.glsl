#version 330 core
precision mediump float;

uniform sampler2D tex_object;
uniform sampler2D tex_object2;

in vec2 TexCoord_;
out vec4 FragColor;

void main()
{
	vec4 Color = texture(tex_object, TexCoord_);
	float LightBlend = texture(tex_object2, TexCoord_).x;
	FragColor = LightBlend * 0.75 + Color;
	//FragColor = vec4(vec3(LightBlend), 1.0);
	//FragColor = Color;
}
