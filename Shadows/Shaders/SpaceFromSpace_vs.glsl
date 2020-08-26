#version 330 core
precision mediump float;

layout(location = 0) in float VertID;

out vec2 TexCoord_;
out vec3 PosW_;

uniform mat4 VP;
uniform float Depth;

void main()
{
	int vert = int(VertID);

	mat4 InverseVP = inverse(VP);

	TexCoord_ = vec2((vert << 1) & 2, vert & 2);
	gl_Position = vec4(TexCoord_ * vec2(2.0, 2.0) - vec2(1.0, 1.0), 0.999, 1.0);

	vec4 temp = InverseVP * gl_Position;
	temp /= temp.w;
	PosW_ = temp.xyz;
}
