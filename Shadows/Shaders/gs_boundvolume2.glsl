#version 430 core

#preprocessor

precision mediump float;

layout(points) in;
layout(triangle_strip, max_vertices = 24) out;

uniform mat4 MVP;
uniform vec3 Pos;
uniform vec3 BoxMin;
uniform vec3 BoxMax;

bool MakeBoundBox(out vec4 boundingBox[8])
{
	boundingBox[0] = MVP * vec4(Pos + vec3(BoxMax.x, BoxMax.y, BoxMax.z), 1);	// +++
	boundingBox[1] = MVP * vec4(Pos + vec3(BoxMin.x, BoxMax.y, BoxMax.z), 1);	// -++
	boundingBox[2] = MVP * vec4(Pos + vec3(BoxMax.x, BoxMin.y, BoxMax.z), 1);	// +-+
	boundingBox[3] = MVP * vec4(Pos + vec3(BoxMin.x, BoxMin.y, BoxMax.z), 1);	// --+
	boundingBox[4] = MVP * vec4(Pos + vec3(BoxMax.x, BoxMax.y, BoxMin.z), 1);	// ++-
	boundingBox[5] = MVP * vec4(Pos + vec3(BoxMin.x, BoxMax.y, BoxMin.z), 1);	// -+-
	boundingBox[6] = MVP * vec4(Pos + vec3(BoxMax.x, BoxMin.y, BoxMin.z), 1);	// +--
	boundingBox[7] = MVP * vec4(Pos + vec3(BoxMin.x, BoxMin.y, BoxMin.z), 1);	// ---

	int outOfBound[6] = int[6](0, 0, 0, 0, 0, 0);

	for (int i = 0; i < 8; ++i)
	{
		if (boundingBox[i].x > boundingBox[i].w) ++outOfBound[0];
		if (boundingBox[i].x < -boundingBox[i].w) ++outOfBound[1];
		if (boundingBox[i].y > boundingBox[i].w) ++outOfBound[2];
		if (boundingBox[i].y < -boundingBox[i].w) ++outOfBound[3];
		if (boundingBox[i].z > boundingBox[i].w) ++outOfBound[4];
		if (boundingBox[i].z < -boundingBox[i].w) ++outOfBound[5];
	}

	for (int i = 0; i < 6; ++i)
	{
		if (outOfBound[i] >= 8)
			return false;
	}

	return true;
}

void main()
{
	vec4 boundingBox[8];
	if (MakeBoundBox(boundingBox))
	{
		// z +
		gl_Position = boundingBox[1];
		EmitVertex();
		gl_Position = boundingBox[0];
		EmitVertex();
		gl_Position = boundingBox[3];
		EmitVertex();
		gl_Position = boundingBox[2];
		EmitVertex();
		EndPrimitive();

		// z -
		gl_Position = boundingBox[5];
		EmitVertex();
		gl_Position = boundingBox[4];
		EmitVertex();
		gl_Position = boundingBox[7];
		EmitVertex();
		gl_Position = boundingBox[6];
		EmitVertex();
		EndPrimitive();

		// x +
		gl_Position = boundingBox[4];
		EmitVertex();
		gl_Position = boundingBox[0];
		EmitVertex();
		gl_Position = boundingBox[6];
		EmitVertex();
		gl_Position = boundingBox[2];
		EmitVertex();
		EndPrimitive();

		// x -
		gl_Position = boundingBox[1];
		EmitVertex();
		gl_Position = boundingBox[5];
		EmitVertex();
		gl_Position = boundingBox[3];
		EmitVertex();
		gl_Position = boundingBox[7];
		EmitVertex();
		EndPrimitive();

		// y +
		gl_Position = boundingBox[1];
		EmitVertex();
		gl_Position = boundingBox[0];
		EmitVertex();
		gl_Position = boundingBox[5];
		EmitVertex();
		gl_Position = boundingBox[4];
		EmitVertex();
		EndPrimitive();

		// y -
		gl_Position = boundingBox[7];
		EmitVertex();
		gl_Position = boundingBox[6];
		EmitVertex();
		gl_Position = boundingBox[3];
		EmitVertex();
		gl_Position = boundingBox[2];
		EmitVertex();
		EndPrimitive();
	}
}