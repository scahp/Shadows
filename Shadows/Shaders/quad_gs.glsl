#version 430 core

precision mediump float;

layout(points) in;
layout(triangle_strip, max_vertices = 6) out;

uniform vec3 Pos[4];
uniform mat4 MVP;

out vec2 UV_;

void EmitVert(int InIndex)
{
	gl_Position = MVP * vec4(Pos[InIndex], 1.0);
	EmitVertex();
}

void main()
{
	// Triangle 0
	gl_Position = MVP * vec4(Pos[0], 1.0);
	UV_ = vec2(0.0, 0.0);
	EmitVertex();

	gl_Position = MVP * vec4(Pos[1], 1.0);
	UV_ = vec2(1.0, 0.0);
	EmitVertex();

	gl_Position = MVP * vec4(Pos[2], 1.0);
	UV_ = vec2(0.0, 1.0);
	EmitVertex();
	
	EndPrimitive();

	// Triangle 1
	gl_Position = MVP * vec4(Pos[2], 1.0);
	UV_ = vec2(0.0, 1.0);
	EmitVertex();

	gl_Position = MVP * vec4(Pos[1], 1.0);
	UV_ = vec2(1.0, 0.0);
	EmitVertex();

	gl_Position = MVP * vec4(Pos[3], 1.0);
	UV_ = vec2(1.0, 1.0);
	EmitVertex();

	EndPrimitive();
}