#version 330 core

precision mediump float;

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

	bool debugWS = false;
	if (debugWS)
	{
		color.xyz = vec3(posWS.x, posWS.y, posWS.z);
		color.w = 1.0;
		return;
	}

    vec3 vecForward = normalize(posWS.xyz - EyePos.xyz);
    float traceDistance = dot(posWS.xyz - (EyePos.xyz + vecForward * CameraNear), vecForward);
	//color.xyz = vec3(traceDistance);
	//return;
	traceDistance = clamp(traceDistance, 0.0, 2500.0); // Far trace distance

	posWS.xyz = EyePos.xyz + vecForward * CameraNear;

    //// Optimization
    //float dotViewLight = dot(vecForward, LightForward);
    //vecForward *= exp(dotViewLight * dotViewLight);

    //vecForward *= g_rSamplingRate * 2.0;
    vecForward *= 2.0 * (1.0 / 4.0);

    int stepsNum = int(min(traceDistance / length(vecForward), float(MAX_STEPS)));

    float step = length(vecForward);
    float scale = step * 0.0005; // Set base brightness factor
    vec4 shadowUV;
    vec3 coordinates;

    // Calculate coordinate delta ( coordinate step in ligh space )
    // NearPlane에서 Forward 방향으로 스탭 * jitter 나아간 위치를 얻어낸다. 이 좌표의 x, y를 텍스쳐 좌표, z는 월드의 z값.
	float jitter = 1.0;
	vec3 curPosition = posWS.xyz + vecForward * jitter;
    shadowUV = LightVP * vec4(curPosition, 1.0);
    coordinates = shadowUV.xyz / shadowUV.w;
    //coordinates.x = (coordinates.x + 1.0) * 0.5;
    //coordinates.y = (1.0 - coordinates.y) * 0.5;
	coordinates.xy = (coordinates.xy + vec2(1.0)) * 0.5;
    coordinates.z = dot(curPosition - LightPos, LightForward);

    // NearPlane에서 Forward 방향으로 (한 스탭 + 스탭 * jitter 나아간 위치)를 얻어낸다. 이 좌표의 x, y를 텍스쳐 좌표, z는 월드의 z값.
    curPosition = posWS.xyz + vecForward * (1.0 + jitter);
    shadowUV = LightVP * vec4(curPosition, 1.0);
    vec3 coordinateEnd = shadowUV.xyz / shadowUV.w;
    //coordinateEnd.x = (coordinateEnd.x + 1.0) * 0.5;
    //coordinateEnd.y = (1.0 - coordinateEnd.y) * 0.5;
	coordinateEnd.xy = (coordinateEnd.xy + vec2(1.0)) * 0.5;
    coordinateEnd.z = dot(curPosition - LightPos, LightForward);

    vec3 coordinateDelta = coordinateEnd - coordinates;

    //vec2 vecForwardProjection;
    //vecForwardProjection.x = dot(LightRight, vecForward);
    //vecForwardProjection.y = dot(LightUp, vecForward);

	float sampleFine;
	vec2 sampleMinMax;
	float light = 0.0;

	int cnt = 0;
	float TTtest = 0.0;
	for (int i = 0; i < stepsNum; i++)
	{
		// 현재 위치에서의 Min, Max 값을 얻어온다.
		//sampleMinMax = textureLod(tex_object3, coordinates.xy, 0).xy;

		// Use point sampling. Linear sampling can cause the whole coarse step being incorrect
		// 현재 위치에서 Shadow 가 드리우는 정도를 fetch 해옴
		float haha = texture2D(tex_object2, coordinates.xy).x;
		float test = float(haha > coordinates.z);
		sampleFine = test;

		//float zStart = textureLod(tex_object4, coordinates.xy, 0).x;

		//const float transactionScale = 100.0f;

		//// Add some attenuation for smooth light fading out
		//float attenuation = (coordinates.z - zStart) / ((sampleMinMax.y + transactionScale) - zStart);
		//attenuation = clamp(attenuation, 0.0, 1.0);
		//attenuation = 1.0 - attenuation;
		//attenuation *= attenuation;

		//float attenuation2 = ((zStart + transactionScale) - coordinates.z) * (1.0 / transactionScale);
		//attenuation2 = 1.0 - clamp(attenuation2, 0.0, 1.0);

		//attenuation *= attenuation2;

		////// Use this value to incerase light factor for "indoor" areas
		//float density = float(texture(tex_object4, coordinates.xy).x > coordinates.z);
		//////float density = s1.SampleCmpLevelZero(samplerPoint_Greater, coordinates.xy, coordinates.z);
		//density *= 10.0 * attenuation;
		//density += 0.25f;
		//sampleFine *= density;

		//coordinateZ_end = coordinates.z + coordinateDelta.z * longStepScale;

		//float comparisonValue = max(coordinates.z, coordinateZ_end);
		//float isLight = comparisonValue < sampleMinMax.x; // .x stores min depth values

		//comparisonValue = min(coordinates.z, coordinateZ_end);
		//float isShadow = comparisonValue > sampleMinMax.y; // .y stores max depth values

		//// We can perform coarse step if all samples are in light or shadow
		//isLongStep = isLight + isShadow;

		//longStepsNum += isLongStep;
		//realStepsNum += 1.0;

		//if (useZOptimizations)
		//{
		//	light += scale * sampleFine * (1.0 + isLongStep * longStepScale_1); // longStepScale should be >= 1 if we use a coarse step

		//	coordinates += coordinateDelta * (1.0 + isLongStep * longStepScale_1);
		//	i += isLongStep * longStepScale_1;
		//}
		//else

		{
			light += scale * sampleFine;
			coordinates += coordinateDelta;

			if (sampleFine > 0)
			{
				cnt++;
				TTtest = haha;
			}
		}

		////if (sampleFine > 0.0)
		//{
		//	light += sampleFine;
		//	color = vec4(vec3(haha, coordinates.z, coordinates.x), coordinates.y);
		//	return;
		//}
	}

	color = vec4(vec3(light), cnt);
}