#version 330 core

precision mediump float;

uniform sampler2D tex_object;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	float depth0 = texture2D(tex_object, TexCoord_).x;
	float threshold = depth0 - 100.0;

	float depth1 = textureOffset(tex_object, TexCoord_, ivec2(1, 0)).x;
	float depth2 = textureOffset(tex_object, TexCoord_, ivec2(0, -1)).x;
	float depth3 = textureOffset(tex_object, TexCoord_, ivec2(-1, 0)).x;
	float depth4 = textureOffset(tex_object, TexCoord_, ivec2(0, 1)).x;

	float depthMin = min(depth1, depth2);
	depthMin = min(depthMin, depth3);
	depthMin = min(depthMin, depth4);

	depth1 = textureOffset(tex_object, TexCoord_, ivec2(1, 1)).x;
	depth2 = textureOffset(tex_object, TexCoord_, ivec2(1, -1)).x;
	depth3 = textureOffset(tex_object, TexCoord_, ivec2(-1, -1)).x;
	depth4 = textureOffset(tex_object, TexCoord_, ivec2(-1, 1)).x;

	depthMin = min(depthMin, depth1);
	depthMin = min(depthMin, depth2);
	depthMin = min(depthMin, depth3);
	depthMin = min(depthMin, depth4);

	float propogate = float(threshold > depthMin);

	color.x = mix(depth0, depthMin, propogate);
}