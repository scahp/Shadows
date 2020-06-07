#version 330 core

precision mediump float;

uniform sampler2D tex_object;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	vec2 depth1 = texture2D(tex_object, TexCoord_).xy;
	vec2 depth2 = textureOffset(tex_object, TexCoord_, ivec2(0, 1)).xy;
	vec2 depth3 = textureOffset(tex_object, TexCoord_, ivec2(1, 1)).xy;
	vec2 depth4 = textureOffset(tex_object, TexCoord_, ivec2(1, 0)).xy;
	vec2 depth5 = textureOffset(tex_object, TexCoord_, ivec2(1, -1)).xy;
	vec2 depth6 = textureOffset(tex_object, TexCoord_, ivec2(0, -1)).xy;
	vec2 depth7 = textureOffset(tex_object, TexCoord_, ivec2(-1, -1)).xy;
	vec2 depth8 = textureOffset(tex_object, TexCoord_, ivec2(-1, 0)).xy;
	vec2 depth9 = textureOffset(tex_object, TexCoord_, ivec2(-1, 1)).xy;

	float minDepth = min(depth1.x, depth2.x);
	minDepth = min(minDepth, depth3.x);
	minDepth = min(minDepth, depth4.x);
	minDepth = min(minDepth, depth5.x);
	minDepth = min(minDepth, depth6.x);
	minDepth = min(minDepth, depth7.x);
	minDepth = min(minDepth, depth8.x);
	minDepth = min(minDepth, depth9.x);

	float maxDepth = max(depth1.y, depth2.y);
	maxDepth = max(maxDepth, depth3.y);
	maxDepth = max(maxDepth, depth4.y);
	maxDepth = max(maxDepth, depth5.y);
	maxDepth = max(maxDepth, depth6.y);
	maxDepth = max(maxDepth, depth7.y);
	maxDepth = max(maxDepth, depth8.y);
	maxDepth = max(maxDepth, depth9.y);

	color.xy = vec2(minDepth, maxDepth);
}