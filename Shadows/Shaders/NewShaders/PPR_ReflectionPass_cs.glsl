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

uniform int DebugWithNormalMap;
uniform int DebugReflectionOnly;

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

// NormalMap을 적용
vec2 GetAppliedNormalMap(vec2 InScreenCoord)
{
	ivec2 vpos = ivec2(gl_GlobalInvocationID.xy);		// Position in screen space

	// Get Current pixel's WorldPosition
	vec3 WorldPos = texture(PosSampler, vec2(vpos.x, vpos.y) / (ScreenSize)).xyz;

	// Make ray from 'CameraWorldPosition' to 'Current pixel's WorldPosition'
	vec3 Origin = CameraWorldPos;
	vec3 Dir = (WorldPos - CameraWorldPos);

	// Get a intersection point between Plane and Ray.
	vec3 IntersectPoint = vec3(0.0, 0.0, 0.0);
	if (IntersectionPlaneAndRay(IntersectPoint, Plane, Origin, Dir))
	{
		// Fetch reflected pixel's WorldPosition
		vec3 ReflectedWorldPos = texture(PosSampler, vec2(InScreenCoord.x, InScreenCoord.y) / (ScreenSize)).xyz;

		// Fetch normal from NormalMap
		vec3 normal = normalize(texture(NormalSampler, vec2(vpos.x, vpos.y) / (ScreenSize)).xyz);
		
		// Distance between ReflectedWorldPosition and IntersectionPoint(this point will be on the plane)
		float reflectedDistance = distance(ReflectedWorldPos, IntersectPoint);

		// Make new reflection vector from WorldPosition to ReflectingPosition.
		vec3 newRelfectionVector = Dir + 2 * dot(normal, -Dir) * normal;
		newRelfectionVector = normalize(newRelfectionVector);

		// Make new reflecting WorldPosition and transform it NDC then transform to screen coordinate.
		vec4 ProjectedNewReflectingPos = WorldToScreen * vec4(WorldPos + newRelfectionVector * reflectedDistance, 1.0);
		ProjectedNewReflectingPos /= ProjectedNewReflectingPos.w;
		vec2 ProjectedUV = (ProjectedNewReflectingPos.xy + vec2(1.0)) * 0.5;
		return (ProjectedUV * ScreenSize);
	}

	return InScreenCoord;
}

// 'intermediate buffer'를 인코딩하기 위한 상수
// 하위 2비트는 좌표계 인덱스를 위해 할당
#define PPR_CLEAR_VALUE (0xffffffff)
#define PPR_PREDICTED_DIST_MULTIPLIER (8)
#define PPR_PREDICTED_OFFSET_RANGE_X (2048)
#define PPR_PREDICTED_DIST_OFFSET_X (PPR_PREDICTED_OFFSET_RANGE_X)
#define PPR_PREDICTED_DIST_OFFSET_Y (0)

// 인코딩된 좌표계 인덱스를 기반으로 좌표계를 디코딩함
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

// Offset의 정수와 소수를 언패킹(Unpacking)함.
vec2 PPR_UnpackValue(uint v, const bool isY)
{
	// unpack fract part
	// 소수 부분 언패킹
	float _fract = (float(v % uint(PPR_PREDICTED_DIST_MULTIPLIER)) + 0.5) / float(PPR_PREDICTED_DIST_MULTIPLIER) - 0.5;
	v /= PPR_PREDICTED_DIST_MULTIPLIER;

	// unpack whole part
	// 정수 부분 언패킹
	float _whole = int(v) - (isY ? PPR_PREDICTED_DIST_OFFSET_Y : PPR_PREDICTED_DIST_OFFSET_X);
	return vec2(_whole, _fract);
}

// 'intermediate buffer'에서 값을 읽어 디코딩
void PPR_DecodeIntermediateBufferValue(uint value, out vec2 distFilteredWhole, out vec2 distFilteredFract, out mat2 packingBasis)
{
	distFilteredWhole = vec2(0.0);
	distFilteredFract = vec2(0.0);
	packingBasis = mat2(1, 0, 0, 1);
	if (value != uint(PPR_CLEAR_VALUE))
	{
		uint settFullValueRange = uint(2 * PPR_PREDICTED_OFFSET_RANGE_X * PPR_PREDICTED_DIST_MULTIPLIER);

		// 좌표계 기저를 디코딩
		packingBasis = PPR_DecodePackingBasisIndexTwoBits(value);
		value /= 4;

		// 좌표계 기저에 맞게 y와 그에 수직인 x를 따라 Offset을 디코딩
		vec2 x = PPR_UnpackValue(value & (settFullValueRange - uint(1)), false);
		vec2 y = PPR_UnpackValue(value / settFullValueRange, true);

		// 결과
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

// color-bleeding을 예방하기 위해서 필터링 된 것과 필터링 되지 않은 컬러를 샘플링하여 조합니다.
// 현재 구현은 단순하여서 아마 결과에 왜곡이 있는 artifact(hole-filling 으로 만들어진)가 어느정도 다시보이게 됩니다.
// Anti-bleeding 솔루션은 artifact를 줄이기 위해서 더 연구를 할 수있습니다.
// 고해상도 반사에서 주의할 점은, 필터링을 건너뛰어서, anti-bleeding 문제가 필요없게 합니다.
vec3 PPR_FixColorBleeding(const vec3 colorFiltered, const vec3 colorUnfiltered)
{
	// 컬러를 YCoCg 로 변환, chrominance 정규화
	vec3 ycocgFiltered = RGB_to_YCoCg(colorFiltered);
	vec3 ycocgUnfiltered = RGB_to_YCoCg(colorUnfiltered);
	ycocgFiltered.yz /= max(0.0001, ycocgFiltered.x);
	ycocgUnfiltered.yz /= max(0.0001, ycocgUnfiltered.x);

	// luma/chroma 를 위한 픽셀 샘플링 요소를 별도로 계산
	float lumaPixelSamplingFactor = clamp(3.0 * abs(ycocgFiltered.x - ycocgUnfiltered.x), 0.0, 1.0);
	float chromaPixelSamplingFactor = clamp(1.4 * length(ycocgFiltered.yz - ycocgUnfiltered.yz), 0.0, 1.0);
	
	// 결과 색상을 YCoCg 공간으로 구성
	// 필터링 된 것과 필터링 되지 않은 것 사이의 보간 (luma/chroma separately)
	float resultY = mix(ycocgFiltered.x, ycocgUnfiltered.x, lumaPixelSamplingFactor);
	vec2 resultCoCg = mix(ycocgFiltered.yz, ycocgUnfiltered.yz, chromaPixelSamplingFactor);
	vec3 ycocgResult = vec3(resultY, resultCoCg * resultY);
	
	// 컬러를 RGB 공간으로 다시 변환
	return YCoCg_to_RGB(ycocgResult);
}

// 'Reflection pass' 구현.
vec4 PPR_ReflectionPass(
	const ivec2 vpos, const bool enableHolesFilling, const bool enableFiltering, 
	const bool applyNormalMap, const bool enableFilterBleedingReduction
)
{
	ivec2 vposread = vpos;
	
	// Hole filling 수행
	// Hole filling을 수행하면, Hole을 채울 근처에 있는 픽셀을 찾습니다. 
	// 이것을 하기 위해서 간단히 변수를 조작하여 컴퓨터 쉐이더의 결과가 이웃과 유사하게 할 것입니다.
	vec2 holesOffset = vec2(0.0);
	if (enableHolesFilling)
	{
		uint v0 = imageLoad(ImmediateBufferImage, vpos).x;
		{
			// 'intermediate buffer' 에서 이웃 데이터를 읽음.
			const ivec2 holeOffset1 = ivec2(1, 0);
			const ivec2 holeOffset2 = ivec2(0, 1);
			const ivec2 holeOffset3 = ivec2(1, 1);
			const ivec2 holeOffset4 = ivec2(-1, 0);
			uint v1 = imageLoad(ImmediateBufferImage, ivec2(vpos.x + holeOffset1.x, vpos.y + holeOffset1.y)).x;
			uint v2 = imageLoad(ImmediateBufferImage, ivec2(vpos.x + holeOffset2.x, vpos.y + holeOffset2.y)).x;
			uint v3 = imageLoad(ImmediateBufferImage, ivec2(vpos.x + holeOffset3.x, vpos.y + holeOffset3.y)).x;
			uint v4 = imageLoad(ImmediateBufferImage, ivec2(vpos.x + holeOffset4.x, vpos.y + holeOffset4.y)).x;
			
			// 반사거리가 가장 가까운 이웃을 얻음
			uint minv = min(min(min(v0, v1), min(v2, v3)), v4);

			// 'intermediate buffer'에서 현재 픽셀에 데이터가 없다고 판단되거나 이웃이 현재 픽셀의 반사보다 더 아주 더 큰 값을 가지고 있으면 Hole fill을 시작
			bool allowHoleFill = true;
			if (uint(PPR_CLEAR_VALUE) != v0)
			{
				// 현재 위치의 Intermediate 데이터를 얻음. Offset과 Offset의 좌표계 (이 좌표계는 Offset 의 방향중 더 큰곳을 Y축으로 잡는 좌표계로 총 4가지가 있음)
				vec2 d0_filtered_whole;		// Offset 정수 부분
				vec2 d0_filtered_fract;		// Offset 소수 부분
				mat2 d0_packingBasis;		// 사용하는 좌표 축

				// 이웃중 가장 가까운 이웃의 Intermediate 데이터를 얻음.
				vec2 dmin_filtered_whole;	// Offset 정수 부분
				vec2 dmin_filtered_fract;	// Offset 소수 부분
				mat2 dmin_packingBasis;		// 사용하는 좌표 축

				PPR_DecodeIntermediateBufferValue(v0, d0_filtered_whole, d0_filtered_fract, d0_packingBasis);
				PPR_DecodeIntermediateBufferValue(minv, dmin_filtered_whole, dmin_filtered_fract, dmin_packingBasis);
				vec2 d0_offset = d0_packingBasis * (d0_filtered_whole + d0_filtered_fract);
				vec2 dmin_offset = dmin_packingBasis * (dmin_filtered_whole + dmin_filtered_fract);

				// 현재 위치와 반사거리가 가장 가까운 이웃의 Offset 을 복원 함
				vec2 diff = d0_offset - dmin_offset;

				// 이웃과 나의 Offset 차이가 minDist 거리보다 작다면, Hole fill을 하지 않음.
				const float minDist = 6;
				allowHoleFill = dot(diff, diff) > minDist * minDist;
			}
			// hole fill allowed, so apply selected neighbor's parameters
			// Hole fill이 허용되어 이웃의 파라메터가 선택되어짐
			if (allowHoleFill)
			{
				if (minv == v1) vposread = vpos + holeOffset1;
				if (minv == v2) vposread = vpos + holeOffset2;
				if (minv == v3) vposread = vpos + holeOffset3;
				if (minv == v4) vposread = vpos + holeOffset4;

				// Hole offset 은 화면 공간에서 Hole fill 된 픽셀과 원래 현재 픽셀 위치의 Offset.
				holesOffset = vposread - vpos;
			}
		}
	}

	// 필터링된 혹은 필터링 되지 않은 샘플의 Offset를 얻음.
	vec2 predictedDist = vec2(0.0);
	vec2 predictedDistUnfiltered = vec2(0.0);
	{
		uint v0 = imageLoad(ImmediateBufferImage, vposread).x;

		// 현재 위치의 'intermediate buffer' 데이터를 얻음. (Hole fill을 적용된 현재위치)
		vec2 decodedWhole;
		vec2 decodedFract;
		mat2 decodedPackingBasis;
		{
			PPR_DecodeIntermediateBufferValue(v0, decodedWhole, decodedFract, decodedPackingBasis);
			predictedDist = decodedPackingBasis * (decodedWhole + decodedFract);
			// 축 정렬이 되지 않은 Offset의 경우는 이웃 픽셀을 샘플링 할 수 있기 때문에 소수 부분은 필터링 되지 않은 샘플을 위해 무시함.
			// -> 축 적렬이 되지 않은 Offset이란 말이 무슨 뜻이냐?
			// -> 여튼 predictedDistUnfiltered 이것은 소수부가 없는 Offset임.
			predictedDistUnfiltered = decodedPackingBasis * decodedWhole;
		}

		// predicted offsets에 hole offset이 포함됨
		if (uint(PPR_CLEAR_VALUE) != v0)
		{
			// Hole fill을 해서, 이웃 픽셀의 반사 정보를 가져온 경우.
			// 이웃 픽셀 기준 반사가 아닌 현재 픽셀을 기준으로 반사 정보를 얻기 위해서 holeOffset 만큼 이동시킴.
			vec2 dir = normalize(predictedDist);
			predictedDistUnfiltered -= vec2(holesOffset.x, holesOffset.y);
			predictedDist -= 2 * dir * dot(dir, holesOffset);
		}
	}
	bool AllComponentZero = (predictedDist.x == 0.0) && (predictedDist.y == 0.0);
	if (AllComponentZero)
	{
		return vec4(0.0);
	}

	// 필터링 된 것과 필터링 되지 않은 컬러를 샘플링함
	// sample filtered and non-filtered color
	vec2 targetCrd = vpos + 0.5 - predictedDist;
	vec2 targetCrdUnfiltered = vpos + 0.5 - predictedDistUnfiltered;

	// 노멀맵 적용
	// Apply NormalMap
	if (applyNormalMap)
	{
		targetCrd = GetAppliedNormalMap(targetCrd);
		targetCrdUnfiltered = GetAppliedNormalMap(targetCrdUnfiltered);
	}

	vec3 colorFiltered = texture(SceneColorLinearSampler, targetCrd * (1.0 / ScreenSize)).xyz;
	vec3 colorUnfiltered = texture(SceneColorPointSampler, targetCrdUnfiltered * (1.0 / ScreenSize)).xyz;

	// 필터링 된것과 필터링 되지 않은 컬러를 조합
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
		result = PPR_ReflectionPass(VPos, true, true, (DebugWithNormalMap != 0), false);

	// Add SceneColor to result
	if (DebugReflectionOnly == 0)
		result += texture(SceneColorPointSampler, UV);

	imageStore(ResultImage, VPos, result);
}
