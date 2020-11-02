#version 330 core
precision mediump float;

layout(location = 0) in float VertID;

uniform vec4 NoiseXform[4];
uniform vec4 ScaleBias;

out vec4 TexCoord0_;
out vec4 TexCoord1_;
out vec4 DiffuseColor_;
out vec4 SpecularColor_;

void main()
{
	int vert = int(VertID);
	vec2 TexCoord = vec2((vert << 1) & 2, vert & 2);
	vec4 Position = vec4(TexCoord.xy * vec2(2.0, 2.0) - vec2(1.0, 1.0), 0.5, 1.0);

	vec4 texCoord4 = vec4(TexCoord, 0.0, 1.0);

	vec4 result = vec4(0.0, 0.0, 0.0, 1.0);
	result.x = dot(texCoord4, NoiseXform[0]);
	result.y = dot(texCoord4, NoiseXform[1]);

	TexCoord0_ = result;

	result.x = dot(texCoord4, NoiseXform[2]);
	result.y = dot(texCoord4, NoiseXform[3]);

	TexCoord1_ = result;

	DiffuseColor_ = ScaleBias.xxzz;
	SpecularColor_ = ScaleBias.yyzz;

	gl_Position = Position;
}