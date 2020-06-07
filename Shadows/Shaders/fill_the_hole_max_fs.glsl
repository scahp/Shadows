#version 330 core

precision mediump float;

uniform sampler2D tex_object;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	float depth0 = texture2D(tex_object, TexCoord_).x;
	float threshold = depth0 + 100.0;

	float depth1 = textureOffset(tex_object, TexCoord_, ivec2(1, 0)).x;
	float depth2 = textureOffset(tex_object, TexCoord_, ivec2(0, -1)).x;
	float depth3 = textureOffset(tex_object, TexCoord_, ivec2(-1, 0)).x;
	float depth4 = textureOffset(tex_object, TexCoord_, ivec2(0, 1)).x;

	float depth5 = textureOffset(tex_object, TexCoord_, ivec2(1, 1)).x;
	float depth6 = textureOffset(tex_object, TexCoord_, ivec2(1, -1)).x;
	float depth7 = textureOffset(tex_object, TexCoord_, ivec2(-1, -1)).x;
	float depth8 = textureOffset(tex_object, TexCoord_, ivec2(-1, 1)).x;

	float depthMax = max(depth1, depth2);
	depthMax = max(depthMax, depth3);
	depthMax = max(depthMax, depth4);
	depthMax = max(depthMax, depth5);
	depthMax = max(depthMax, depth6);
	depthMax = max(depthMax, depth7);
	depthMax = max(depthMax, depth8);

	float propogate = float(threshold < depthMax);
	propogate = clamp(propogate, 0.0, 1.0);

	color.x = mix(depth0, depthMax, propogate);
}