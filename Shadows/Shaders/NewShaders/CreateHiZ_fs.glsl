#version 330 core

precision highp float;

in vec2 TexCoord_;
out vec4 color;

uniform sampler2D DepthSampler;

// Radio of resolution of current rendertarget compare to DepthSampler.
uniform vec2 DepthRadio;	// DepthRadio = LastDepthSampleSize / DepthSampleSize
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

	bool needExtraSampleX = DepthRadio.x > 2.0;
	bool needExtraSampleY = DepthRadio.y > 2.0;

	minDepth = needExtraSampleX ? min(minDepth, min(GetDepthOffset(readTexCoord, ivec2(2, 0)), GetDepthOffset(readTexCoord, ivec2(2, 1)))) : minDepth;
	minDepth = needExtraSampleY ? min(minDepth, min(GetDepthOffset(readTexCoord, ivec2(0, 2)), GetDepthOffset(readTexCoord, ivec2(1, 2)))) : minDepth;
	minDepth = (needExtraSampleX && needExtraSampleY) ? min(minDepth, GetDepthOffset(readTexCoord, ivec2(2, 2))) : minDepth;

	gl_FragDepth = minDepth;
}
