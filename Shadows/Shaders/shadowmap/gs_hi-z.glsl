#version 400 core

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec2 TexCoord;

void main() {
	gl_Position = vec4( 1.0, 1.0, 0.5, 1.0 );
	TexCoord = vec2( 1.0, 1.0 );
	EmitVertex();

	gl_Position = vec4(-1.0, 1.0, 0.5, 1.0 );
	TexCoord = vec2( 0.0, 1.0 ); 
	EmitVertex();

	gl_Position = vec4( 1.0,-1.0, 0.5, 1.0 );
	TexCoord = vec2( 1.0, 0.0 ); 
	EmitVertex();

	gl_Position = vec4(-1.0,-1.0, 0.5, 1.0 );
	TexCoord = vec2( 0.0, 0.0 ); 
	EmitVertex();

	EndPrimitive(); 
}
