#version 330 core
#preprocessor

precision mediump float;

#define EDGE_THRESHOLD_MIN 0.0312
#define EDGE_THRESHOLD_MAX 0.125
#define ITERATIONS 5
#define SUBPIXEL_QUALITY 0.75

uniform sampler2D InputSampler;

uniform vec2 PixelSize;

in vec2 TexCoord_;
out vec4 color;

float rgb2luma(vec3 rgb) 
{
	return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}

float QUALITY(int i)
{
	const float steps[7] = float[7](1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);
	return steps[i];
}

void FindPixelOffset(out float finalOffset, float stepLength, float lumaLocalAverage, float gradientScaled, float lumaCenter, bool isHorizontal)
{
	// UV 를 올바른 방향으로 반픽셀 이동시킴
	vec2 currentUV = TexCoord_;
	if (isHorizontal)
		currentUV.y += stepLength * 0.5;
	else
		currentUV.x += stepLength * 0.5;

	// 오른쪽 방향으로 offset (각 반복 스탭에 대해) 을 계산.
	vec2 offset = isHorizontal ? vec2(PixelSize.x, 0.0) : vec2(0.0, PixelSize.y);

	// 가장자리의 각 측면을 직교하여 탐색하기 위해서 UV를 계산. QUALITY 는 스탭을 더 빠르게 함.
	vec2 uv1 = currentUV - offset;
	vec2 uv2 = currentUV + offset;

	// 탐색하는 세그먼트의 양쪽 끝에서 lumas를 읽고, delta 를 계산하고 로컬 평균 luma에 기록
	float lumaEnd1 = rgb2luma(texture(InputSampler, uv1).rgb);
	float lumaEnd2 = rgb2luma(texture(InputSampler, uv2).rgb);
	lumaEnd1 -= lumaLocalAverage;
	lumaEnd2 -= lumaLocalAverage;

	// 현재 끝점에서 luma delta가 로컬 변화도 보다 크면, 우리는 가장자리의 측면에 도달한 것임
	bool reached1 = abs(lumaEnd1) >= gradientScaled;
	bool reached2 = abs(lumaEnd2) >= gradientScaled;
	bool reachedBoth = reached1 && reached2;

	// 측면에 도달하지 못했다면, 우리는 계속해서 이 방향으로 탐색함.
	if (!reached1)
		uv1 -= offset;

	if (!reached2)
		uv2 += offset;

	// 만약 양 방향 모두 측면에 도달하지 않았다면, 계속해서 탐색
	if (!reachedBoth)
	{
		for (int i = 2; i < ITERATIONS; i++)
		{
			// 필요하다면, 첫번재 방향의 luma를 읽음, delta 계산
			if (!reached1)
			{
				lumaEnd1 = rgb2luma(texture(InputSampler, uv1).rgb);
				lumaEnd1 = lumaEnd1 - lumaLocalAverage;
			}

			// 필요하다면, 반대 방향의 luma를 읽음, delta 계산
			if (!reached2)
			{
				lumaEnd2 = rgb2luma(texture(InputSampler, uv2).rgb);
				lumaEnd2 = lumaEnd2 - lumaLocalAverage;
			}

			// 만약 현재 끝점의 luma delta가 로컬 변화도 보다 크다면, 우리는 가장자리 측면에 도달한 것임
			reached1 = abs(lumaEnd1) >= gradientScaled;
			reached2 = abs(lumaEnd2) >= gradientScaled;
			reachedBoth = reached1 && reached2;

			// 만약 측면에 도달하지 않았다면, 우리는 이 방향으로 계속 탐색, 
			// 가변 품질로 진행 (역주 : 스탭을 반복에 따라 조정하겠다는 의미)
			if (!reached1)
				uv1 -= offset * QUALITY(i);

			if (!reached2)
				uv2 += offset * QUALITY(i);

			// 두 측면에 도착했다면, 탐색을 중단
			if (reachedBoth) { break; }
		}
	}

	// 양 끝 가장자리까지의 거리를 계산
	float distance1 = isHorizontal ? (TexCoord_.x - uv1.x) : (TexCoord_.y - uv1.y);
	float distance2 = isHorizontal ? (uv2.x - TexCoord_.x) : (uv2.y - TexCoord_.y);

	// 어떤 방향의 가장자리의 끝이 더 가깝나?
	bool isDirection1 = distance1 < distance2;
	float distanceFinal = min(distance1, distance2);

	// 가장자리의 길이
	float edgeThickness = (distance1 + distance2);

	// UV offset: 가장자리의 측면까지 가장 가까운 방향으로 읽음
	float pixelOffset = -distanceFinal / edgeThickness + 0.5;

	// 중앙 luma가 로별 평균보다 더 작나?
	bool isLumaCenterSmaller = lumaCenter < lumaLocalAverage;

	// 만약 중앙 luma가 그것의 이웃보다 더 작다면, 양 끝의 delta luma가 양수여야 합니다. (같은 변형)
	// (가장자리의 측면과 더 가까운 방향으로)
	bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0) != isLumaCenterSmaller;

	// 만약 luma 차이가 잘못된 경우, offset 을 적용하지 않음
	finalOffset = correctVariation ? pixelOffset : 0.0;
}

void SubPixelShifting(out float finalOffset, float lumaCenter, float lumaRange, float lumaDownUp, float lumaLeftRight, float lumaLeftCorners, float lumaRightCorners)
{
	// 3x3 이웃에 대한 luma의 전체 가중 평균
	float lumaAverage = (1.0 / 12.0) * (2.0 * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners);

	// 3x3 이웃의 루마 범위에 대한, 전역 평균과 중앙 luma 간의 delta의 비율
	float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter) / lumaRange, 0.0, 1.0);

	// 이 delta에 기반한 sub-pixel의 offset 계산, 아래 2줄의 식은 [0~SUBPIXEL_QUALITY] 까지 곡선형태로 증가하는 식
	float subPixelOffset2 = (-2.0 * subPixelOffset1 + 3.0) * subPixelOffset1 * subPixelOffset1;
	float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;

	// 두 offset 중 더 큰것을 고름
	finalOffset = max(finalOffset, subPixelOffsetFinal);
}

void main()
{
	vec3 colorCenter = texture(InputSampler, TexCoord_).rgb;

	// 현재 픽셀의 luma
	float lumaCenter = rgb2luma(colorCenter);

	// 현재 픽셀 중심으로 4방향에 대한 luma를 얻음
	// Luma at the four direct neighbours of the current fragment.
	float lumaDown = rgb2luma(textureOffset(InputSampler, TexCoord_, ivec2(0, -1)).rgb);
	float lumaUp = rgb2luma(textureOffset(InputSampler, TexCoord_, ivec2(0, 1)).rgb);
	float lumaLeft = rgb2luma(textureOffset(InputSampler, TexCoord_, ivec2(-1, 0)).rgb);
	float lumaRight = rgb2luma(textureOffset(InputSampler, TexCoord_, ivec2(1, 0)).rgb);

	// 현재 픽셀과 그 주변 픽셀의 luma 중 가장 큰 값을 구함.
	float lumaMin = min(lumaCenter, min(min(lumaDown, lumaUp), min(lumaLeft, lumaRight)));
	float lumaMax = max(lumaCenter, max(max(lumaDown, lumaUp), max(lumaLeft, lumaRight)));

	// delta 계산
	float lumaRange = lumaMax - lumaMin;

	// 만약 luma 차이가 임계값보다 작다면(또는 우리가 정말 어두운 영역내에 있다면), 
	// 우리는 가장자리에 있지 않는 것이므로, AA를 수행하지 않는다.
	if (lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX)) {
		color = vec4(colorCenter, 1.0);
		return;
	}

	// 남은 4개의 코너 lumas 를 얻음
	float lumaDownLeft = rgb2luma(textureOffset(InputSampler, TexCoord_, ivec2(-1, -1)).rgb);
	float lumaUpRight = rgb2luma(textureOffset(InputSampler, TexCoord_, ivec2(1, 1)).rgb);
	float lumaUpLeft = rgb2luma(textureOffset(InputSampler, TexCoord_, ivec2(-1, 1)).rgb);
	float lumaDownRight = rgb2luma(textureOffset(InputSampler, TexCoord_, ivec2(1, -1)).rgb);

	// 4 개의 가장자리의 lumas를 조합(같은 값으로 추후 계산을 위해 중간 변수로 사용)
	float lumaDownUp = lumaDown + lumaUp;
	float lumaLeftRight = lumaLeft + lumaRight;

	// 코너도 동일
	float lumaLeftCorners = lumaDownLeft + lumaUpLeft;
	float lumaDownCorners = lumaDownLeft + lumaDownRight;
	float lumaRightCorners = lumaDownRight + lumaUpRight;
	float lumaUpCorners = lumaUpRight + lumaUpLeft;

	// 수평과 수직 축을 따르는 변화도의 추정치 계산
	float edgeHorizontal = abs(-2.0 * lumaLeft + lumaLeftCorners) + abs(-2.0 * lumaCenter + lumaDownUp) * 2.0 + abs(-2.0 * lumaRight + lumaRightCorners);
	float edgeVertical = abs(-2.0 * lumaUp + lumaUpCorners) + abs(-2.0 * lumaCenter + lumaLeftRight) * 2.0 + abs(-2.0 * lumaDown + lumaDownCorners);

	// 로컬 가장자리가 수평인가? 수직인가?
	bool isHorizontal = (edgeHorizontal >= edgeVertical);

	// 로컬 가장자리의 반대방향으로 2개의 이웃 텍셀을 선택함
	float luma1 = isHorizontal ? lumaDown : lumaLeft;
	float luma2 = isHorizontal ? lumaUp : lumaRight;

	// 이 방향으로 변화도를 계산
	float gradient1 = luma1 - lumaCenter;
	float gradient2 = luma2 - lumaCenter;

	// 어느 방향이 변화가 더 가파른가?
	bool is1Steepest = abs(gradient1) >= abs(gradient2);

	// 해당 방향의 변화도, 정규화
	float gradientScaled = 0.25 * max(abs(gradient1), abs(gradient2));

	// 가장자리의 방향에 따라서 스텝 크기(한 픽셀)를 선택
	float stepLength = isHorizontal ? PixelSize.y : PixelSize.x;

	// 올바른 방향의 평균 luma
	float lumaLocalAverage = 0.0;

	if (is1Steepest)
	{
		// 방향을 바꿈
		stepLength = -stepLength;
		lumaLocalAverage = 0.5 * (luma1 + lumaCenter);
	}
	else
	{
		lumaLocalAverage = 0.5 * (luma2 + lumaCenter);
	}

	float finalOffset = 0.0;
	FindPixelOffset(finalOffset, stepLength, lumaLocalAverage, gradientScaled, lumaCenter, isHorizontal);

	// Sub-pixel shifting
	SubPixelShifting(finalOffset, lumaCenter, lumaRange, lumaDownUp, lumaLeftRight, lumaLeftCorners, lumaRightCorners);

	// 최종 UV 좌표 계산
	vec2 finalUv = TexCoord_;
	if (isHorizontal)
		finalUv.y += finalOffset * stepLength;
	else
		finalUv.x += finalOffset * stepLength;

	// 새로운 UV 좌표에서 컬러를 읽고 사용함
	vec3 finalColor = texture(InputSampler, finalUv).rgb;
	color = vec4(finalColor, 1.0);
}