#version 330 core

precision highp float;

#define MAX_STEPS 2000

uniform sampler2D tex_object;		// MainScene DepthBuffer
uniform sampler2D tex_object2;		// ShadowMap World
uniform sampler2D tex_object3;		// ShadowMap Scaled Opt (Min/Max Final)
uniform sampler2D tex_object4;		// ShadowMap Fill the hole depth

uniform mat4 CameraVPInv;
uniform mat4 CameraVInv;
uniform mat4 CameraPInv;
uniform vec3 EyePos;
uniform vec3 EyeForward;
uniform float CameraNear;
uniform float CameraFar;
uniform vec3 LightPos;
uniform vec3 LightForward;
uniform mat4 LightVP;
uniform vec3 LightRight;
uniform vec3 LightUp;
uniform float CoarseDepthTexelSize;

in vec2 TexCoord_;
out vec4 color;

void main()
{
    float SceneDepth = texture2D(tex_object, TexCoord_).x;

    vec4 clipPos;
    clipPos.x = 2.0 * TexCoord_.x - 1.0;
    clipPos.y = 2.0 * TexCoord_.y - 1.0;
    clipPos.z = 2.0 * SceneDepth - 1.0;
    clipPos.w = 1.0;

    vec4 posWS = CameraVPInv * clipPos;
    posWS /= posWS.w;

	float DebugZ = posWS.z;

	bool debugWS = false;
	if (debugWS)
	{
		color.xyz = vec3(posWS.x, posWS.y, posWS.z);
		color.w = 1.0;
		return;
	}

    vec3 vecForward = normalize(posWS.xyz - EyePos.xyz);
    float traceDistance = dot(posWS.xyz - (EyePos.xyz + vecForward * CameraNear), vecForward);
	traceDistance = clamp(traceDistance, 0.0, 2500.0); // Far trace distance

	posWS.xyz = EyePos.xyz + vecForward * CameraNear;
    vecForward *= 2.0 * (1.0 / 4.0);

    int stepsNum = int(min(traceDistance / length(vecForward), float(MAX_STEPS)));

    float step = length(vecForward);
    float scale = step * 0.001; // Set base brightness factor
    vec4 shadowUV;
    vec3 coordinates;

    // NearPlane에서 위치를 얻어낸다. 이 좌표의 x, y를 텍스쳐 좌표, z는 월드의 z값.
	float jitter = 1.0;
	vec3 curPosition = posWS.xyz;
    shadowUV = LightVP * vec4(curPosition, 1.0);
    coordinates = shadowUV.xyz / shadowUV.w;
	coordinates.xy = (coordinates.xy + vec2(1.0)) * 0.5;
    coordinates.z = dot(curPosition - LightPos, LightForward);

    // NearPlane에서 Forward 방향으로 (한 스탭 + 스탭 나아간 위치)를 얻어낸다. 이 좌표의 x, y를 텍스쳐 좌표, z는 월드의 z값.
	curPosition = posWS.xyz + vecForward;
    shadowUV = LightVP * vec4(curPosition, 1.0);
    vec3 coordinateEnd = shadowUV.xyz / shadowUV.w;
	coordinateEnd.xy = (coordinateEnd.xy + vec2(1.0)) * 0.5;
    coordinateEnd.z = dot(curPosition - LightPos, LightForward);

    vec3 coordinateDelta = coordinateEnd - coordinates;

    vec2 vecForwardProjection;
    vecForwardProjection.x = dot(LightRight, vecForward);
    vecForwardProjection.y = dot(LightUp, vecForward);

	// Calculate coarse step size
	float longStepScale = int(CoarseDepthTexelSize / length(vecForwardProjection));
	longStepScale = max(longStepScale, 1);
	float longStepScale_1 = longStepScale - 1;
	float longStepsNum = 0;
	float realStepsNum = 0;

	float sampleFine;
	float light = 0.0;
	float coordinateZ_end;
	bool optimizeOn = true;

	int i = 0;
	for (; i < stepsNum; i++)
	{
		// 텍스쳐 UV 영역을 벗어나는 부분 회피
		if ((coordinates.x > 1.0 || coordinates.x < 0.0) || (coordinates.y > 1.0 || coordinates.y < 0.0))
		{
			if (optimizeOn)
			{
				coordinates += coordinateDelta * (1.0 + longStepScale_1);
				i += int(longStepScale_1);
			}
			else
			{
				coordinates += coordinateDelta;
			}
			continue;
		}

		// Min/Max Texture Fetch (SM_WIDHT / 16)
		vec2 sampleMinMax = textureLod(tex_object3, coordinates.xy, 0).xy;

		// ShadowMap WorldZ Fetch (SM_WIDTH)
		float ShadowMapWorld = texture2D(tex_object2, coordinates.xy).x;
		sampleFine = float(ShadowMapWorld > coordinates.z);

		// Fill the hole Texture Fetch (SM_WIDHT / 16)
		float zStart = textureLod(tex_object4, coordinates.xy, 0).x;

		// Light Volume Attenuation
		const float transactionScale = 100.0;

		float attenuation = (coordinates.z - zStart) / ((sampleMinMax.y + transactionScale) - zStart);
		attenuation = clamp(attenuation, 0.0, 1.0);
		attenuation = 1.0 - attenuation;
		attenuation *= attenuation;

		float attenuation2 = ((zStart + transactionScale) - coordinates.z) * (1.0 / transactionScale);
		attenuation2 = 1.0 - clamp(attenuation2, 0.0, 1.0);
		attenuation *= attenuation2;

		// Fill the hole texture 와 현재 Depth 중 현재 Depth가 더 크다는 말은 ShadowMap에서 거리다 더 말다는 뜻이며
		// 이 것은 Hole 이 채워져서 막혔다는 의미가 됨. 그래서 Density를 더 높혀줘서 잘 보이도록 함.
		float density = float(zStart < coordinates.z);
		density *= 10.0 * attenuation;
		//density += 0.25;
		sampleFine *= density;

		if (optimizeOn)
		{
			// 2048/128 에 비례한 텍스쳐 크기를 고려한, Min/Max Depth 값으로 Coarse Step 진행 가능 여부 체크
			coordinateZ_end = coordinates.z + coordinateDelta.z * longStepScale;

			// 현재 Min/Max 타일 영역의 Min 값과 비교하여 Light, Shadow 여부 파악
			// MinZ < CurZ < MaxZ 의 경우는 Min/Max 타일 영역 내에서 isLight, isShadow 여부가 변경될 수 있기 때문에 그냥 보통 Step을 진행하지만 
			// CurZ < MinZ, CurZ > MAxZ 의 경우는 타일 영역 전체의 극소, 극대 값 범위 밖이기 때문에 isLight, isShadow 가 확실하다고 볼 수 있음.

			float comparisonValue = max(coordinates.z, coordinateZ_end);
			float isLight = float(comparisonValue < sampleMinMax.x); // .x stores min depth values

			comparisonValue = min(coordinates.z, coordinateZ_end);
			float isShadow = float(comparisonValue > sampleMinMax.y); // .y stores max depth values

			// We can perform coarse step if all samples are in light or shadow
			float isLongStep = isLight + isShadow;

			longStepsNum += isLongStep;
			realStepsNum += 1.0;

			light += scale * sampleFine * (1.0 + isLongStep * longStepScale_1);		// longStepScale should be >= 1 if we use a coarse step
			coordinates += coordinateDelta * (1.0 + isLongStep * longStepScale_1);
			i += int(isLongStep * longStepScale_1);
		}
		else
		{
			light += scale * sampleFine;
			coordinates += coordinateDelta;
		}
	}

	light -= scale * sampleFine * (i - stepsNum);

	color = vec4(vec3(light), 1.0);
}