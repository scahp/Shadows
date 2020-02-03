#version 400 core

layout(points) in;
layout(points, max_vertices = 1) out;

#preprocessor

flat in int Visible[1];

layout(stream=0) out vec3 OutVisible;

void main()
{
	if (Visible[0] != 0)
	{
		OutVisible = vec3(1.0);
		EmitStreamVertex(0);
	}
}