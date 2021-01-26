#version 330 core

precision mediump float;

uniform sampler2D ColorSampler;
uniform sampler2D NormalSampler;
uniform sampler2D PosSampler;
uniform sampler2D TexNoise;
uniform sampler2D DepthSampler;

uniform vec3 samples[64];
uniform mat4 Projection;

uniform mat4 V;

in vec2 TexCoord_;

out vec4 color;

const vec2 noiseScale = vec2(1280.0 / 4.0, 720.0 / 4.0); // screen = 1280x720
const float radius = 0.5;
const int kernelSize = 64;
const float bias = 0.0;

vec3 GetViewSpacePos(vec2 uv)
{
	vec3 pos = texture(PosSampler, uv).xyz;
	return (V * vec4(pos, 1.0)).xyz;
}

vec3 GetViewSpaceNormal(vec2 uv)
{
	vec3 normal = texture(NormalSampler, uv).xyz;
	return (V * vec4(normal, 0.0)).xyz;
}

void main()
{
	vec3 posInView = GetViewSpacePos(TexCoord_);
	vec3 normalInView = GetViewSpaceNormal(TexCoord_);

	vec3 randomVec = texture(TexNoise, TexCoord_ * noiseScale).xyz;

	vec3 tangent = normalize(randomVec - normalInView * dot(randomVec, normalInView));
	vec3 bitangent = cross(normalInView, tangent);
	mat3 TBN = mat3(tangent, bitangent, normalInView);

	float occlusion = 0.0;
	for (int i = 0; i < kernelSize; ++i)
	{ 
		// get sample position 
		vec3 sample = TBN * samples[i];

		// From tangent to view-space
		sample = posInView + sample * radius;

		vec4 offset = vec4(sample, 1.0); 
		offset = Projection * offset; // from view to clip-space
		offset.xyz /= offset.w; // perspective divide 
		offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

		float sampleDepth = GetViewSpacePos(offset.xy).z;
		//occlusion += (sampleDepth >= sample.z + bias ? 1.0 : 0.0);

		float rangeCheck = smoothstep(0.0, 1.0, radius / abs(posInView.z - sampleDepth));
		occlusion += (sampleDepth >= sample.z + bias ? 1.0 : 0.0) * rangeCheck;
	}
	occlusion = 1.0 - (occlusion / kernelSize);
	color.r = occlusion;
}