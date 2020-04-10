#version 430 core

layout(binding = 0, rgba8) uniform image2D srcTex;
layout(binding = 1, rgba8) uniform image2D destTex;
layout(local_size_x = 16, local_size_y = 16) in;

bool isIn(ivec2 TexPos)
{
	vec4 texColor = imageLoad(srcTex, TexPos);
	vec4 white = vec4(1.0);
	if (texColor.r != white.r || texColor.b != white.b || texColor.g != white.g)
		return false;
	else
		return true;
}

float squaredDistanceBetween(vec2 uv1, vec2 uv2)
{
	vec2 delta = uv1 - uv2;
	float dist = (delta.x * delta.x) + (delta.y * delta.y);
	return dist;
}

void main(void)
{
	ivec2 TexPos = ivec2(gl_GlobalInvocationID.xy);

	float Range = 16.0;
	int iRange = int(Range);
	int HalfRange = iRange / 2;

	bool fragIsIn = isIn(TexPos);
	float squaredDistanceToEdge = (HalfRange * HalfRange) * 2.0;

	ivec2 StartPos = ivec2(TexPos.x - int(HalfRange), TexPos.y - int(HalfRange));
	for (int dx = 0; dx < iRange; ++dx)
	{
		for (int dy = 0; dy < iRange; ++dy)
		{
			ivec2 CurTexPos = StartPos + ivec2(dx, dy);
			
			bool scanIsIn = isIn(CurTexPos);
			if (scanIsIn != fragIsIn)
			{
				float scanDistance = squaredDistanceBetween(TexPos, CurTexPos);
				if (scanDistance < squaredDistanceToEdge)
					squaredDistanceToEdge = scanDistance;
			}
		}
	}

	float normalized = squaredDistanceToEdge / ((HalfRange * HalfRange) * 2.0);
	float distanceToEdge = sqrt(normalized);
	if (fragIsIn)
		distanceToEdge = -distanceToEdge;
	normalized = 0.5 - distanceToEdge;

	float test = float(fragIsIn);

	imageStore(destTex, TexPos, vec4(normalized, normalized, normalized, 1));
	//imageStore(destTex, TexPos, vec4(1, 0, 0, 1));
}
