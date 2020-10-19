#version 330 core
precision mediump float;

uniform sampler2D tex_object;

in vec2 TexCoord_;
out vec4 FragColor;

void main()
{
	FragColor = texture(tex_object, TexCoord_);
}
