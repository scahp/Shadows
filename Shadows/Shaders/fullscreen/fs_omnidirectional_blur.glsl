#version 330 core

#include "common.glsl"

precision mediump float;

uniform sampler2D tex_object;
uniform vec2 PixelSize;
uniform float IsVertical;
uniform float MaxDist;

#define FILTER_STEP_COUNT 5.0
#define FILTER_SIZE vec2(FILTER_STEP_COUNT, FILTER_STEP_COUNT)
#define COUNT (FILTER_STEP_COUNT * 2.0 + 1.0)

in vec2 TexCoord_;
in int glLayer;

out vec4 color;

void main()
{
	const float invCount = 1.0 / COUNT;
	const float inv6 = 1.0 / 6.0;
	
	vec2 radiusUV = (FILTER_SIZE * PixelSize) / FILTER_STEP_COUNT;
	radiusUV.y *= inv6;
	if (IsVertical > 0.0)
        radiusUV = vec2(0.0, radiusUV.y);
    else
        radiusUV = vec2(radiusUV.x, 0.0);

    vec4 colorTemp = vec4(0);
	for (float x = -FILTER_STEP_COUNT; x <= FILTER_STEP_COUNT; ++x)
	{
		int index = int(TexCoord_.y / inv6);
		vec2 tex = TexCoord_ + vec2(x, x) * radiusUV;
		TexArrayUV uv;
		uv.u = tex.x;
		uv.v = (tex.y - (inv6 * index)) * 6.0;
		uv.index = index;
		uv = MakeTexArrayUV(uv);
		tex = Convert_TexArrayUV_To_Tex2dUV(uv);
		colorTemp += texture(tex_object, tex) * invCount;
	}
	color = colorTemp;
}