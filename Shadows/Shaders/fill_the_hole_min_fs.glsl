#version 330 core

precision mediump float;

uniform sampler2D tex_object;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	float depth0 = texture2D(tex_object, TexCoord_).y;
	float threshold = depth0 - 100.0;

	float depth1 = textureOffset(tex_object, TexCoord_, ivec2(1, 0)).y;
	float depth2 = textureOffset(tex_object, TexCoord_, ivec2(0, -1)).y;
	float depth3 = textureOffset(tex_object, TexCoord_, ivec2(-1, 0)).y;
	float depth4 = textureOffset(tex_object, TexCoord_, ivec2(0, 1)).y;

	float depthMin = min(depth1, depth2);
	depthMin = min(depthMin, depth3);
	depthMin = min(depthMin, depth4);

	depth1 = textureOffset(tex_object, TexCoord_, ivec2(1, 1)).y;
	depth2 = textureOffset(tex_object, TexCoord_, ivec2(1, -1)).y;
	depth3 = textureOffset(tex_object, TexCoord_, ivec2(-1, -1)).y;
	depth4 = textureOffset(tex_object, TexCoord_, ivec2(-1, 1)).y;

	depthMin = min(depthMin, depth1);
	depthMin = min(depthMin, depth2);
	depthMin = min(depthMin, depth3);
	depthMin = min(depthMin, depth4);

	float propogate = float(threshold > depthMin);

	color.x = mix(depth0, depthMin, propogate);
}