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

	cos0 = (cos0 - vec4(0.5)) * 2.0;
	cos1 = (cos1 - vec4(0.5)) * 2.0;
	cos2 = (cos2 - vec4(0.5)) * 2.0;
	cos3 = (cos3 - vec4(0.5)) * 2.0;

	vec4 result = cos0 * Coef[0] + cos1 * Coef[1] + cos2 * Coef[2] + cos3 * Coef[3];

	FragColor = result * ReScale + ReScale;
}
