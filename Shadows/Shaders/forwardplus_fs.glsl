#version 430 core

precision mediump float;

#define TILE_SIZE 32

struct PointLight 
{
	vec3 Pos;
	float Radius;
	vec4 Color;
};

struct VisibleIndex 
{
	int index;
};

layout(std430, binding = 0) buffer LightBuffer
{
	PointLight LightData[];
} lightBuffer;

layout(std430, binding = 1) buffer VisibleLightIndicesBuffer
{
	VisibleIndex data[];
} visibleLightIndicesBuffer;

uniform int numOfTilesX;

in vec3 Pos_;
in vec4 Color_;

out vec4 color;

void main()
{
	ivec2 pixelLocation = ivec2(gl_FragCoord.xy);
	ivec2 tileID = pixelLocation / ivec2(TILE_SIZE, TILE_SIZE);
	uint tileIndex = tileID.y * numOfTilesX + tileID.x;

	color = vec4(Color_) * vec4(0.2, 0.2, 0.2, 1.0);

	uint offset = tileIndex * 1024;
	for (uint i = 0; i < 1024 && visibleLightIndicesBuffer.data[offset + i].index != -1; ++i)
	{
		uint lightIndex = visibleLightIndicesBuffer.data[offset + i].index;
		PointLight light = lightBuffer.LightData[lightIndex];

		float dist = distance(light.Pos, Pos_);
		if (dist < light.Radius)
		{
			float distSq = dist * dist;
			const float LightIntensity = 100.0;
			color += (light.Color * (LightIntensity / distSq));
		}
	}
}