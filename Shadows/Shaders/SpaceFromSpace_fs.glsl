#version 330 core
precision mediump float;

uniform sampler2D tex_object;

in vec2 TexCoord_;
out vec4 FragColor;

in vec3 PosW_;
uniform vec3 CameraPos;
uniform vec3 ToLight;

// Calculates the Mie phase function
float getMiePhase(float cos, float cos2, float g, float g2)
{
	return 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + cos2) / pow(1.0 + g2 - 2.0 * g * cos, 1.5);
}

void main()
{
	vec3 Direction_ = CameraPos - PosW_;
	float test = -0.999999;
	float cos = dot(ToLight, Direction_) / length(Direction_);
	float cos2 = cos * cos;
	vec4 color = vec4(vec3(getMiePhase(cos, cos2, test, test * test)), 1.0);

	FragColor = clamp(color, vec4(0.0), vec4(1.0));
}