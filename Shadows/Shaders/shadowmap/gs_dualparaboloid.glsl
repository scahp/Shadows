#version 430 core

#preprocessor

precision mediump float;

layout (triangles) in;
layout (triangle_strip, max_vertices=6) out;

uniform float Near;
uniform float Far;

out float ClipDepth;
out float Depth;
out int gl_Layer;

void main()
{
	const float ParaboloidDir[2] = { 1.0f, -1.0f };
	for(int face=0;face<2;++face)
	{
		gl_Layer = face;
		gl_ViewportIndex = face;
		for(int i=0;i<3;++i)
		{
			vec4 PosMV = gl_in[i].gl_Position;
			PosMV.z *= ParaboloidDir[face];

			float len = length(PosMV.xyz);
			PosMV /= len;

			ClipDepth = PosMV.z;

			PosMV.x /= (PosMV.z + 1.0);
			PosMV.y /= (PosMV.z + 1.0);

			PosMV.z = (len - Near) / (Far - Near);
			PosMV.w = 1.0;

			Depth = PosMV.z;
			gl_Position = PosMV;

			EmitVertex();
		}
		EndPrimitive();
	}
}