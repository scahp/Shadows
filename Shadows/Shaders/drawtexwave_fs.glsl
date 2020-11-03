#version 330 core
precision mediump float;

uniform sampler2D tex_object;		// CosineLUT
uniform sampler2D tex_object2;		// Noise

uniform vec4 Coef[4];

uniform vec4 ReScale;

in vec3 Pos_;
in vec4 Uv0_; // Ripple texture coords
in vec4 Uv1_; // Ripple texture coords
in vec4 Uv2_; // Ripple texture coords
in vec4 Uv3_; // Ripple texture coords

out vec4 FragColor;

void main()
{
	vec4 cos0 = texture(tex_object, vec2(Uv0_.xy));
	vec4 cos1 = texture(tex_object, vec2(Uv1_.xy));
	vec4 cos2 = texture(tex_object, vec2(Uv2_.xy));
	vec4 cos3 = texture(tex_object, vec2(Uv3_.xy));

	cos0.xy = (cos0.xy - vec2(0.5)) * 2.0;
	cos1.xy = (cos1.xy - vec2(0.5)) * 2.0;
	cos2.xy = (cos2.xy - vec2(0.5)) * 2.0;
	cos3.xy = (cos3.xy - vec2(0.5)) * 2.0;

	// Equation 14 번
	// Coef : Dir.x * normalScale, Dir.y * normalScale, 1, 1
	// cos(D * (uv) + phase) * Coef
	// = cos(D * (uv) + phase) * (Dir.xy * normalScale)
	// 여기서 Dir.xy 에 w와 A가 곱해져야 하는데 빠져있는 상태
	vec4 result = cos0 * Coef[0] + cos1 * Coef[1] + cos2 * Coef[2] + cos3 * Coef[3];

	//FragColor = result * ReScale + ReScale;
	FragColor = result;
}