#version 430 core

layout(local_size_x = 1, local_size_y = 1) in;

layout(binding = 0, rgba32f) readonly uniform image2D SceneColorImage;
layout(binding = 1, r32ui) readonly uniform uimage2D ImmediateBufferImage;
layout(binding = 2, rgba32f) uniform image2D ResultImage;

precision mediump float;

uniform vec2 ScreenSize;

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

// 'Reflection pass' implementation.
vec4 PPR_ReflectionPass(
	const ivec2 vpos
	//StructuredBuffer<uint> srvIntermediateBuffer, 
	//Texture2D srvColor,
	//const bool enableHolesFilling, const bool enableFiltering, const bool enableFilterBleedingReduction
)
{
	ivec2 vposread = vpos;
	// perform holes filling.
	// If we're dealing with a hole then find a closeby pixel that will be used
	// to fill the hole. In order to do this simply manipulate variable so that
	// compute shader result would be similar to the neighbor result.
	vec2 holesOffset = vec2(0.0);

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
		return vec4(0.0, 1.0, 0.0, 1.0);
	}
	// sample filtered and non-filtered color
	vec2 targetCrd = vpos + 0.5 - predictedDist;
	vec2 targetCrdUnfiltered = vpos + 0.5 - predictedDistUnfiltered;
	//vec3 colorFiltered = srvColor.SampleLevel(smpLinear, targetCrd * globalConstants.resolution.zw, 0).xyz;
	//vec3 colorUnfiltered = srvColor.SampleLevel(smpPoint, targetCrdUnfiltered * globalConstants.resolution.zw, 0).xyz;
	vec3 colorUnfiltered = imageLoad(SceneColorImage, ivec2(targetCrdUnfiltered)).xyz;

	// combine filtered and non-filtered colors
	vec3 colorResult;
	{
		colorResult = colorUnfiltered;
	}
	//
	return vec4(colorResult, 1);
}

void main()
{
	ivec2 VPos = ivec2(gl_GlobalInvocationID.xy);		// Position in screen space
	vec4 result = PPR_ReflectionPass(VPos);
	imageStore(ResultImage, VPos, result);	
}
