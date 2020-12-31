#version 430 core

precision mediump float;

layout(points) in;
layout(triangle_strip, max_vertices = 6) out;

uniform vec3 Pos[4];
uniform mat4 MVP;
uniform mat4 M;

out vec3 Pos_;

void EmitVert(int InIndex)
{
	gl_Position = MVP * vec4(Pos[InIndex], 1.0);
	Pos_ = (M * vec4(Pos[InIndex], 1.0)).xyz;
	EmitVertex();
}

void main()
{
	EmitVert(0);
	EmitVert(1);
	EmitVert(2);
	EndPrimitive();

	EmitVert(2);
	EmitVert(1);
	EmitVert(3);
	EndPrimitive();
}