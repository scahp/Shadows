#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;

uniform mat4 MVP;

out vec3 TexCoord_;

void main()
{
	TexCoord_ = Pos;
	//gl_Position = MVP * vec4(Pos.x, Pos.y, Pos.z, 1.0);
	gl_Position = MVP * vec4(-Pos.x, Pos.y, Pos.z, 0.0);	// infinityfar
}