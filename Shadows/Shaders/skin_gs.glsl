#version 430 core

#preprocessor

precision mediump float;

layout(triangles) in;
layout(line_strip, max_vertices = 24) out;

uniform sampler2D tex_object3;
uniform mat4 VP;

in vec3 Pos2_[];
in vec2 TexCoord2_[];

out vec3 Pos_;
out vec2 TexCoord_;

void main()
{
	// normal check
	for (int i = 0; i < 3; ++i)
	{
		vec3 normal = texture2D(tex_object3, TexCoord2_[i]).xyz * 2 - 1.0;
		normal = normal * 10.0;

		Pos_ = Pos2_[i];
		TexCoord_ = TexCoord2_[i];

		gl_Position = VP * vec4(Pos_, 1.0);
		EmitVertex();

		gl_Position = VP * vec4(Pos_ + normal, 1.0);
		EmitVertex();

		EndPrimitive();
	}
}