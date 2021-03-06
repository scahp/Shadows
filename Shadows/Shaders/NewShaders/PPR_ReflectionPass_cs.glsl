#version 430 core

layout(local_size_x = 1, local_size_y = 1) in;

precision mediump float;

layout(binding = 0, r32ui) readonly uniform uimage2D ImmediateBufferImage;
layout(binding = 1, rgba32f) uniform image2D ResultImage;

uniform sampler2D SceneColorPointSampler;
uniform sampler2D SceneColorLinearSampler;
uniform sampler2D NormalSampler;
uniform sampler2D PosSampler;

uniform vec2 ScreenSize;
uniform vec3 CameraWorldPos;
uniform mat4 WorldToScreen;
uniform vec4 Plane;

bool IntersectionPlaneAndRay(out vec3 OutIntersectPoint, vec4 InPlane, vec3 InRayOrigin, vec3 InRayDir)
{
	vec3 normal = InPlane.xyz;
	float dist = InPlane.w;

	float t = 0.0;
	vec3 IntersectPoint = vec3(0.0, 0.0, 0.0);
	float temp = dot(normal, InRayDir);
	if (abs(temp) > 0.0001)
	{
		t = (dist - dot(normal, InRayOrigin)) / temp;
		if ((0.0 <= t) && (1.0 >= t))
		{
			OutIntersectPoint = InRayOrigin + InRayDir * t;
			return true;
		}
	}
	return false;
}

vec2 GetAppliedNormalMap(vec2 InScreenCoord)
{
	ivec2 vpos = ivec2(gl_GlobalInvocationID.xy);		// Position in screen space

	vec3 WorldPos = texture(PosSampler, vec2(vpos.x, vpos.y) / (ScreenSize)).xyz;

	vec3 Origin = CameraWorldPos;
	vec3 Dir = (WorldPos - CameraWorldPos);

	vec3 IntersectPoint = vec3(0.0, 0.0, 0.0);
	if (IntersectionPlaneAndRay(IntersectPoint, Plane, Origin, Dir))
	{
		vec3 ReflectedWorldPos = texture(PosSampler, vec2(InScreenCoord.x, InScreenCoord.y) / (ScreenSize)).xyz;
		vec3 normal = normalize(texture(NormalSampler, vec2(vpos.x, vpos.y) / (ScreenSize)).xyz);
		float reflectedDistance = distance(ReflectedWorldPos, IntersectPoint);

		vec3 NewRelfectVector = Dir + 2 * dot(normal, -Dir) * normal;
		NewRelfectVector = normalize(NewRelfectVector);

		vec4 ProjectedNewReflectingPos = WorldToScreen * vec4(WorldPos + NewRelfectVector * reflectedDistance, 1.0);
		ProjectedNewReflectingPos /= ProjectedNewReflectingPos.w;
		vec2 ProjectedUV = (ProjectedNewReflectingPos.xy + vec2(1.0)) * 0.5;

		ProjectedUV = clamp(ProjectedUV, vec2(0.0), vec2(1.0));
		return (ProjectedUV * ScreenSize);
	}

	return InScreenCoord;
}

// Constants for 'intermediate buffer' values encoding.
// Lowest two bits reserved for coordinate system index.
#define PPR_CLEAR_VALUE (0xffffffff)
#define PPR_PREDICTED_DIST_MULTIPLIER (8)
#define PPR_PREDICTED_OFFSET_RANGE_X (2048)
#define PPR_PREDICTED_DIST_OFFSET_X (PPR_PREDICTED_OFFSET_RANGE_X)
#define PPR_PREDICTED_DIST_OFFSET_Y (0)
// Calculate coordinate system index based on offset
uint PPR_GetPackingBasisIndexTwoBits(const vec2 offset)
{
	if (abs(offset.x) >= abs(offset.y))
		return uint(offset.x >= 0 ? 0 : 1);
	return uint(offset.y >= 0 ? 2 : 3);
}
// Decode coordinate system based on encoded coordSystem index
mat2 PPR_DecodePackingBasisIndexTwoBits(uint packingBasisIndex)
{
	vec2 basis = vec2(0.0);
	packingBasisIndex &= 3;
	basis.x += uint(0) == packingBasisIndex ? 1 : 0;
	basis.x += uint(1) == packingBasisIndex ? -1 : 0;
	basis.y += uint(2) == packingBasisIndex ? 1 : 0;
	basis.y += uint(3) == packingBasisIndex ? -1 : 0;
	return mat2(vec2(basis.y, -basis.x), basis.xy);
}

// Pack integer and fract offset value
uint PPR_PackValue(const float _whole, float _fract, const bool isY)
{
	uint result = uint(0);
	// pack whole part
	result += uint(_whole + (isY ? PPR_PREDICTED_DIST_OFFSET_Y : PPR_PREDICTED_DIST_OFFSET_X));
	result *= PPR_PREDICTED_DIST_MULTIPLIER;
	// pack fract part
	_fract *= PPR_PREDICTED_DIST_MULTIPLIER;
	result += uint(min(floor(_fract + 0.5), PPR_PREDICTED_DIST_MULTIPLIER - 1));
	//
	return result;
}
// Unpack integer and fract offset value
vec2 PPR_UnpackValue(uint v, const bool isY)
{
	// unpack fract part
	float _fract = (float(v % uint(PPR_PREDICTED_DIST_MULTIPLIER)) + 0.5) / float(PPR_PREDICTED_DIST_MULTIPLIER) - 0.5;
	v /= PPR_PREDICTED_DIST_MULTIPLIER;
	// unpack whole part
	float _whole = int(v) - (isY ? PPR_PREDICTED_DIST_OFFSET_Y : PPR_PREDICTED_DIST_OFFSET_X);
	//
	return vec2(_whole, _fract);
}

// Encode offset for 'intermediate buffer' storage
uint PPR_EncodeIntermediateBufferValue(const vec2 offset)
{
	// build snapped basis
	uint packingBasisIndex = PPR_GetPackingBasisIndexTwoBits(offset);
	mat2 packingBasisSnappedMatrix = PPR_DecodePackingBasisIndexTwoBits(packingBasisIndex);
	// decompose offset to _whole and _fract parts
	vec2 _whole = floor(offset + 0.5);
	vec2 _fract = offset - _whole;
	// mirror _fract part to avoid filtered result 'swimming' under motion
	vec2 dir = normalize(offset);
	_fract -= 2 * dir * dot(dir, _fract);
	// transform both parts to snapped basis
	_whole = packingBasisSnappedMatrix * _whole;
	_fract = packingBasisSnappedMatrix * _fract;
	// put _fract part in 0..1 range
	_fract *= 0.707;
	_fract += 0.5;
	// encode result
	uint result = uint(0);
	result += PPR_PackValue(_whole.y, _fract.y, true);
	result *= 2 * PPR_PREDICTED_OFFSET_RANGE_X * PPR_PREDICTED_DIST_MULTIPLIER;
	result += PPR_PackValue(_whole.x, _fract.x, false);
	result *= 4;
	result += packingBasisIndex;
	//
	return result;
}

// Decode value read from 'intermediate buffer'
void PPR_DecodeIntermediateBufferValue(uint value, out vec2 distFilteredWhole, out vec2 distFilteredFract, out mat2 packingBasis)
{
	distFilteredWhole = vec2(0.0);
	distFilteredFract = vec2(0.0);
	packingBasis = mat2(1, 0, 0, 1);
	if (value != uint(PPR_CLEAR_VALUE))
	{
		uint settFullValueRange = uint(2 * PPR_PREDICTED_OFFSET_RANGE_X * PPR_PREDICTED_DIST_MULTIPLIER);
		// decode packing basis
		packingBasis = PPR_DecodePackingBasisIndexTwoBits(value);
		value /= 4;
		// decode offsets along (y) and perpendicular (x) to snapped basis
		vec2 x = PPR_UnpackValue(value & (settFullValueRange - uint(1)), false);
		vec2 y = PPR_UnpackValue(value / settFullValueRange, true);
		// output result
		distFilteredWhole = vec2(x.x, y.x);
		distFilteredFract = vec2(x.y, y.y);
		distFilteredFract /= 0.707;
	}
}

// https://www.shadertoy.com/view/ltjBWG
const mat3 RGBToYCoCgMatrix = mat3(0.25, 0.5, -0.25, 0.5, 0.0, 0.5, 0.25, -0.5, -0.25);
const mat3 YCoCgToRGBMatrix = mat3(1.0, 1.0, 1.0, 1.0, 0.0, -1.0, -1.0, 1.0, -1.0);
vec3 RGB_to_YCoCg(vec3 InRGB)
{
	return RGBToYCoCgMatrix * InRGB;
}
vec3 YCoCg_to_RGB(vec3 InYCoCg)
{
	return YCoCgToRGBMatrix * InYCoCg;
}

// Combine filtered and non-filtered color sample to prevent color-bleeding.
// Current implementation is rather naive and may result in distortion artifacts
// (created by holes-filling) to become visible again at some extent.
// Anti-bleeding solution might use some further research to reduce artifacts.
// Note that for high resolution reflections, filtering might be skipped, making
// anti-bleeding solution unneeded.
vec3 PPR_FixColorBleeding(const vec3 colorFiltered, const vec3 colorUnfiltered)
{
	// transform color to YCoCg, normalize chrominance
	//vec3 ycocgFiltered = mul(RGB_to_YCoCg(), colorFiltered);
	//vec3 ycocgUnfiltered = mul(RGB_to_YCoCg(), colorUnfiltered);
	vec3 ycocgFiltered = RGB_to_YCoCg(colorFiltered);
	vec3 ycocgUnfiltered = RGB_to_YCoCg(colorUnfiltered);
	ycocgFiltered.yz /= max(0.0001, ycocgFiltered.x);
	ycocgUnfiltered.yz /= max(0.0001, ycocgUnfiltered.x);
	// calculate pixel sampling factors for luma/chroma separately
	float lumaPixelSamplingFactor = clamp(3.0 * abs(ycocgFiltered.x - ycocgUnfiltered.x), 0.0, 1.0);
	float chromaPixelSamplingFactor = clamp(1.4 * length(ycocgFiltered.yz - ycocgUnfiltered.yz), 0.0, 1.0);
	// build result color YCoCg space
	// interpolate between filtered and nonFiltered colors (luma/chroma separately)
	float resultY = mix(ycocgFiltered.x, ycocgUnfiltered.x, lumaPixelSamplingFactor);
	vec2 resultCoCg = mix(ycocgFiltered.yz, ycocgUnfiltered.yz, chromaPixelSamplingFactor);
	vec3 ycocgResult = vec3(resultY, resultCoCg * resultY);
	// transform color back to RGB space
	// return mul( YCoCg_to_RGB(), ycocgResult );
	return YCoCg_to_RGB(ycocgResult);
}

// 'Reflection pass' implementation.
vec4 PPR_ReflectionPass(
	const ivec2 vpos,
	//StructuredBuffer<uint> srvIntermediateBuffer, 
	//Texture2D srvColor,
	const bool enableHolesFilling, const bool enableFiltering, const bool applyNormalMap, const bool enableFilterBleedingReduction
)
{
	ivec2 vposread = vpos;
	// perform holes filling.
	// If we're dealing with a hole then find a closeby pixel that will be used
	// to fill the hole. In order to do this simply manipulate variable so that
	// compute shader result would be similar to the neighbor result.
	vec2 holesOffset = vec2(0.0);
	if (enableHolesFilling)
	{
		uint v0 = imageLoad(ImmediateBufferImage, vpos).x;
		{
			// read neighbors 'intermediate buffer' data
			const ivec2 holeOffset1 = ivec2(1, 0);
			const ivec2 holeOffset2 = ivec2(0, 1);
			const ivec2 holeOffset3 = ivec2(1, 1);
			const ivec2 holeOffset4 = ivec2(-1, 0);
			uint v1 = imageLoad(ImmediateBufferImage, ivec2(vpos.x + holeOffset1.x, vpos.y + holeOffset1.y)).x;
			uint v2 = imageLoad(ImmediateBufferImage, ivec2(vpos.x + holeOffset2.x, vpos.y + holeOffset2.y)).x;
			uint v3 = imageLoad(ImmediateBufferImage, ivec2(vpos.x + holeOffset3.x, vpos.y + holeOffset3.y)).x;
			uint v4 = imageLoad(ImmediateBufferImage, ivec2(vpos.x + holeOffset4.x, vpos.y + holeOffset4.y)).x;
			// get neighbor closest reflection distance
			uint minv = min(min(min(v0, v1), min(v2, v3)), v4);

			// allow hole fill if we don't have any 'intermediate buffer' data for current pixel,
			// or any neighbor has reflecion significantly closer than current pixel's reflection
			bool allowHoleFill = true;
			if (uint(PPR_CLEAR_VALUE) != v0)
			{
				vec2 d0_filtered_whole;
				vec2 d0_filtered_fract;
				mat2 d0_packingBasis;
				vec2 dmin_filtered_whole;
				vec2 dmin_filtered_fract;
				mat2 dmin_packingBasis;
				PPR_DecodeIntermediateBufferValue(v0, d0_filtered_whole, d0_filtered_fract, d0_packingBasis);
				PPR_DecodeIntermediateBufferValue(minv, dmin_filtered_whole, dmin_filtered_fract, dmin_packingBasis);
				vec2 d0_offset = d0_packingBasis * (d0_filtered_whole + d0_filtered_fract);
				vec2 dmin_offset = dmin_packingBasis * (dmin_filtered_whole + dmin_filtered_fract);
				vec2 diff = d0_offset - dmin_offset;
				const float minDist = 6;
				allowHoleFill = dot(diff, diff) > minDist * minDist;
			}
			// hole fill allowed, so apply selected neighbor's parameters
			if (allowHoleFill)
			{
				if (minv == v1) vposread = vpos + holeOffset1;
				if (minv == v2) vposread = vpos + holeOffset2;
				if (minv == v3) vposread = vpos + holeOffset3;
				if (minv == v4) vposread = vpos + holeOffset4;
				holesOffset = vposread - vpos;
			}
		}
	}

	// obtain offsets for filtered and non-filtered samples
	vec2 predictedDist = vec2(0.0);
	vec2 predictedDistUnfiltered = vec2(0.0);
	{
		//uint v0 = srvIntermediateBuffer[vposread.x + vposread.y * int(ScreenSize.x)];
		uint v0 = imageLoad(ImmediateBufferImage, vposread).x;

		// decode offsets
		vec2 decodedWhole;
		vec2 decodedFract;
		mat2 decodedPackingBasis;
		{
			PPR_DecodeIntermediateBufferValue(v0, decodedWhole, decodedFract, decodedPackingBasis);
			predictedDist = decodedPackingBasis * (decodedWhole + decodedFract);
			// fractional part ignored for unfiltered sample, as it could end up
			// sampling neighboring pixel in case of non axis aligned offsets.
			predictedDistUnfiltered = decodedPackingBasis * decodedWhole;
		}
		// include holes offset in predicted offsets
		if (uint(PPR_CLEAR_VALUE) != v0)
		{
			vec2 dir = normalize(predictedDist);
			predictedDistUnfiltered -= vec2(holesOffset.x, holesOffset.y);
			predictedDist -= 2 * dir * dot(dir, holesOffset);
		}
	}
	// exit if reflection offset not present
	//if (all(predictedDist == 0))
	bool AllComponentZero = (predictedDist.x == 0.0) && (predictedDist.y == 0.0);
	if (AllComponentZero)
	{
		return vec4(0.0);
	}

	// sample filtered and non-filtered color
	vec2 targetCrd = vpos + 0.5 - predictedDist;
	vec2 targetCrdUnfiltered = vpos + 0.5 - predictedDistUnfiltered;

	// Apply NormalMap
	if (applyNormalMap)
	{
		targetCrd = GetAppliedNormalMap(targetCrd);
		targetCrdUnfiltered = GetAppliedNormalMap(targetCrdUnfiltered);
	}

	//vec3 colorFiltered = srvColor.SampleLevel(smpLinear, targetCrd * globalConstants.resolution.zw, 0).xyz;
	//vec3 colorUnfiltered = srvColor.SampleLevel(smpPoint, targetCrdUnfiltered * globalConstants.resolution.zw, 0).xyz;
	vec3 colorFiltered = texture(SceneColorLinearSampler, targetCrd * (1.0 / ScreenSize)).xyz;
	vec3 colorUnfiltered = texture(SceneColorPointSampler, targetCrdUnfiltered * (1.0 / ScreenSize)).xyz;

	// combine filtered and non-filtered colors
	vec3 colorResult;
	if (enableFiltering)
	{
		if (enableFilterBleedingReduction)
			colorResult = PPR_FixColorBleeding(colorFiltered, colorUnfiltered);
		else
			colorResult = colorFiltered;
	}
	else
	{
		colorResult = colorUnfiltered;
	}
	
	return vec4(colorResult, 1);
}

float GetReflectionMask(vec2 uv)
{
	return texture(NormalSampler, uv).w;
}

void main()
{
	ivec2 VPos = ivec2(gl_GlobalInvocationID.xy);		// Position in screen space
	vec2 UV = vec2(VPos) * (1.0 / ScreenSize);

	float reflectionMask = GetReflectionMask(UV);	// should be fetched from texture 

	vec4 result = vec4(0.0);
	if (reflectionMask != 0.0)
		result = PPR_ReflectionPass(VPos, true, true, true, false);

	// Add SceneColor to result
	result += texture(SceneColorPointSampler, UV);

	imageStore(ResultImage, VPos, result);
}
