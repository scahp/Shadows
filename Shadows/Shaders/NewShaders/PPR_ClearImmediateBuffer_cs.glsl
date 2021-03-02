#version 430 core

#define SAMPLE_INTERVAL_X 1
#define SAMPLE_INTERVAL_Y 1

layout(local_size_x = SAMPLE_INTERVAL_X, local_size_y = SAMPLE_INTERVAL_Y) in;

layout(binding = 0, r32ui) writeonly uniform uimage2D ImmediateBufferImage;

precision mediump float;

uniform uint ClearValue;

void main()
{
	ivec2 VPos = ivec2(gl_GlobalInvocationID.xy);		// Position in screen space
	imageStore(ImmediateBufferImage, VPos, uvec4(ClearValue));
}
