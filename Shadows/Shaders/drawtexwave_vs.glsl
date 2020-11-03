#version 330 core
precision mediump float;

layout(location = 0) in float VertID;

uniform vec4 UTrans[4];

struct VertOut
{
	vec4 Position;
	vec4 Uv0; // Ripple texture coords
	vec4 Uv1; // Ripple texture coords
	vec4 Uv2; // Ripple texture coords
	vec4 Uv3; // Ripple texture coords
};

VertOut Func(vec4 position, vec2 texCoord, vec4 rot0, vec4 rot1, vec4 rot2, vec4 rot3)
{
	VertOut Out;
	Out.Position = position;

	vec4 uv = vec4(0.0f, 0.0f, 0.0f, 1.f);
	vec4 texCoord4 = vec4(texCoord, 0.0, 1.0);

	uv.x = dot(texCoord4, rot0);
	Out.Uv0 = uv;

	uv.x = dot(texCoord4, rot1);
	Out.Uv1 = uv;

	uv.x = dot(texCoord4, rot2);
	Out.Uv2 = uv;

	uv.x = dot(texCoord4, rot3);
	Out.Uv3 = uv;

	return Out;
}

out vec3 Pos_;
out vec4 Uv0_; // Ripple texture coords
out vec4 Uv1_; // Ripple texture coords
out vec4 Uv2_; // Ripple texture coords
out vec4 Uv3_; // Ripple texture coords

void main()
{
	int vert = int(VertID);
	vec2 TexCoord = vec2((vert << 1) & 2, vert & 2);
	vec4 Position = vec4(TexCoord.xy * vec2(2.0, 2.0) - vec2(1.0, 1.0), 0.5, 1.0);

	// UTrans : RotScale.x, RotScale.y, 0, Phase
	// Cos 내부 dir.xy를 RotScale.xy로 나타냄, 실제 dir과 rotscale의 차이는 rotscale이 정규화된 사이즈라는 것임
	// -> w와 A는 어디서 곱해졌을까?
	// -> cos 내부에 들어가는 dir을 특수 처리 한 것인가?
	VertOut result = Func(Position, TexCoord, UTrans[0], UTrans[1], UTrans[2], UTrans[3]);

	Pos_ = result.Position.xyz;
	Uv0_ = result.Uv0;
	Uv1_ = result.Uv1;
	Uv2_ = result.Uv2;
	Uv3_ = result.Uv3;

	gl_Position = result.Position;
}