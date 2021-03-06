#version 430 core

layout(local_size_x = 1, local_size_y = 1) in;

layout(binding = 0, rgba32f) readonly uniform image2D WorldPosImage;
layout(binding = 1, r32ui) uniform uimage2D ImmediateBufferImage;

precision mediump float;

uniform vec2 ScreenSize;
uniform mat4 WorldToScreen;
uniform vec4 Plane;

// 'intermediate buffer'를 인코딩하기 위한 상수
// 하위 2비트는 좌표계 인덱스를 위해 할당
#define PPR_CLEAR_VALUE (0xffffffff)
#define PPR_PREDICTED_DIST_MULTIPLIER (8)
#define PPR_PREDICTED_OFFSET_RANGE_X (2048)
#define PPR_PREDICTED_DIST_OFFSET_X (PPR_PREDICTED_OFFSET_RANGE_X)
#define PPR_PREDICTED_DIST_OFFSET_Y (0)

// offset을 기반으로 좌표계 인덱스 계산
// -> 더 긴쪽이 y 가 되도록 결정하며 총 4개의 좌표계가 있음.
uint PPR_GetPackingBasisIndexTwoBits(const vec2 offset)
{
	if (abs(offset.x) >= abs(offset.y))
		return uint(offset.x >= 0 ? 0 : 1);
	return uint(offset.y >= 0 ? 2 : 3);
}

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

// Offset의 정수와 소수를 패킹(Packing)함.
uint PPR_PackValue(const float _whole, float _fract, const bool isY)
{
	uint result = uint(0);
	// 정부 부분 패킹
	result += uint(_whole + (isY ? PPR_PREDICTED_DIST_OFFSET_Y : PPR_PREDICTED_DIST_OFFSET_X));
	result *= PPR_PREDICTED_DIST_MULTIPLIER;
	
	// 소수 부분 패킹
	_fract *= PPR_PREDICTED_DIST_MULTIPLIER;
	result += uint(min(floor(_fract + 0.5), PPR_PREDICTED_DIST_MULTIPLIER - 1));
	return result;
}

// Encode offset for 'intermediate buffer' storage
// 'intermediate buffer' 저장소를 위해 Offset을 인코딩 함
uint PPR_EncodeIntermediateBufferValue(const vec2 offset)
{
	// Offset의 y축이 더 긴 좌표계의 인덱스 선택
	uint packingBasisIndex = PPR_GetPackingBasisIndexTwoBits(offset);
	mat2 packingBasisSnappedMatrix = PPR_DecodePackingBasisIndexTwoBits(packingBasisIndex);
	
	// Offset을 정수부와 소수부로 분리함
	vec2 _whole = floor(offset + 0.5);
	vec2 _fract = offset - _whole;

	// 필터링된 결과가 움직이는 동안 'swimming'을 만드는 것을 피하기 위해서 소수 부분을 미러링 함.
	vec2 dir = normalize(offset);
	_fract -= 2 * dir * dot(dir, _fract);

	// 두 부분을 변환하여 좌표계 기저(basis)에 맞춤
	_whole = packingBasisSnappedMatrix * _whole;
	_fract = packingBasisSnappedMatrix * _fract;
	
	// 소수부를 0..1 범위로 둠
	_fract *= 0.707;
	_fract += 0.5;

	// 결과 인코딩
	uint result = uint(0);
	result += PPR_PackValue(_whole.y, _fract.y, true);
	result *= 2 * PPR_PREDICTED_OFFSET_RANGE_X * PPR_PREDICTED_DIST_MULTIPLIER;
	result += PPR_PackValue(_whole.x, _fract.x, false);
	result *= 4;
	result += packingBasisIndex;
	return result;
}

// 'intermediate buffer'로 프로젝션된 결과를 기록합니다.
// 'originalPixelVpos' 로 부터 'mirroredWorldPos'로 픽셀을 프로젝션 시킴.
// Shape의 주어진 위치에 프로젝션된 픽셀이 다른 Shape에 의해 가려지지 않는 것을 보장한 뒤에 'projection pass'에서 이 함수가 호출됩니다.
void PPR_ProjectionPassWrite(ivec2 originalPixelVpos, vec3 mirroredWorldPos)
{
	vec4 projPosOrig = WorldToScreen * vec4(mirroredWorldPos, 1);
	vec4 projPos = projPosOrig / projPosOrig.w;

	bool AllComponentLessThanOne = (abs(projPos.x) < 1.0) && (abs(projPos.y) < 1.0);
	if (AllComponentLessThanOne)
	{
		vec2 targetCrd = (projPos.xy * vec2(0.5, 0.5) + 0.5) * ScreenSize;
		vec2 offset = targetCrd - (originalPixelVpos + 0.5);
		uint originalValue = uint(0);
		uint valueToWrite = PPR_EncodeIntermediateBufferValue(offset);
		imageAtomicMin(ImmediateBufferImage, ivec2(int(targetCrd.x), int(targetCrd.y)), valueToWrite);
	}
}

void main()
{
	ivec2 VPos = ivec2(gl_GlobalInvocationID.xy);		// Position in screen space
	vec3 WorldPos = imageLoad(WorldPosImage, VPos).xyz;

	float dist = dot(Plane.xyz, WorldPos) - Plane.w;

	// Upper-side of the plane is skip
	if (dist < 0.0)
		return;

	vec3 MirroredWorldPos = WorldPos + 2.0 * dist * (-Plane.xyz);
	PPR_ProjectionPassWrite(VPos, MirroredWorldPos);
}
