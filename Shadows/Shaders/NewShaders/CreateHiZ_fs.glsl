#version 330 core

precision highp float;

in vec2 TexCoord_;
out vec4 color;

uniform sampler2D DepthSampler;

// Ratio of resolution of current rendertarget compare to DepthSampler.
uniform vec2 DepthRatio;	// DepthRatio = LastDepthSampleSize / DepthSampleSize
uniform vec2 ScreenSize;
uniform int PrevMipLevel;
uniform int MipLevel;

float GetDepthOffset(vec2 InTexCoord, ivec2 InOffset)
{
	return textureLodOffset(DepthSampler, InTexCoord, PrevMipLevel, InOffset).x;
}

void main()
{
	ivec2 screenCoord = ivec2(int(floor(gl_FragCoord.x)), int((gl_FragCoord.y)));
	vec2 readTexCoord = vec2(float(screenCoord.x << 1), float(screenCoord.y << 1)) / ScreenSize;

	vec4 depthSamples;
	depthSamples.x = GetDepthOffset(readTexCoord, ivec2(0, 0));
	depthSamples.y = GetDepthOffset(readTexCoord, ivec2(1, 0));
	depthSamples.z = GetDepthOffset(readTexCoord, ivec2(1, 1));
	depthSamples.w = GetDepthOffset(readTexCoord, ivec2(0, 1));

	float minDepth = min(depthSamples.x, min(depthSamples.y, min(depthSamples.z, depthSamples.w)));

	// To avoid losing sample's to create HiZ which is not power of 2. you need extra sample that is at both down and right of texture.
	//  - When 'level 0' have 5x5 tiles, then 'level 1' will have 2.5 x 2.5.
	//    So if you need fetch all texel in texture, you need to fetch one more texel toward down and right side.
	bool needExtraSampleX = DepthRatio.x > 2.0;
	bool needExtraSampleY = DepthRatio.y > 2.0;

	minDepth = needExtraSampleX ? min(minDepth, min(GetDepthOffset(readTexCoord, ivec2(2, 0)), GetDepthOffset(readTexCoord, ivec2(2, 1)))) : minDepth;
	minDepth = needExtraSampleY ? min(minDepth, min(GetDepthOffset(readTexCoord, ivec2(0, 2)), GetDepthOffset(readTexCoord, ivec2(1, 2)))) : minDepth;
	minDepth = (needExtraSampleX && needExtraSampleY) ? min(minDepth, GetDepthOffset(readTexCoord, ivec2(2, 2))) : minDepth;

	gl_FragDepth = minDepth;
}
