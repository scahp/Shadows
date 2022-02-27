#version 430 core

#preprocessor

precision mediump float;

#define NUM_CASCADES 3
#define	NUM_VERTICES 9

layout (triangles) in;
layout (triangle_strip, max_vertices=NUM_VERTICES) out;

uniform mat4 CascadeLightVP[NUM_CASCADES];

out vec4 fragPos_;
out int glLayer;

void main()
{
	for(int cascadeIndex=0; cascadeIndex < NUM_CASCADES;++cascadeIndex)
	{
		glLayer = cascadeIndex;
		gl_ViewportIndex = cascadeIndex;
		for(int i=0;i<3;++i)
		{
			fragPos_ = gl_in[i].gl_Position;
			gl_Position = CascadeLightVP[cascadeIndex] * fragPos_;
			EmitVertex();
		}
		EndPrimitive();
	}
}