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
	//color.xyz = vec3(traceDistance);
	//return;
	traceDistance = clamp(traceDistance, 0.0, 2500.0); // Far trace distance

	//vec3 HAHA = EyePos.xyz + vecForward * 350.0;
	//vec3 HH = (HAHA - posWS.xyz);
	//if (sqrt(dot(HH, HH)) < 10.0)
	//{
	//	color.xyz = vec3(0.0, 0.0, 1.0);
	//	return;
	//}

	posWS.xyz = EyePos.xyz + vecForward * CameraNear;

    //// Optimization
    //float dotViewLight = dot(vecForward, LightForward);
    //vecForward *= exp(dotViewLight * dotViewLight);

    //vecForward *= g_rSamplingRate * 2.0;
    vecForward *= 2.0 * (1.0 / 4.0);

    int stepsNum = int(min(traceDistance / length(vecForward), float(MAX_STEPS)));

    float step = length(vecForward);
    float scale = step * 0.001; // Set base brightness factor
    vec4 shadowUV;
    vec3 coordinates;

    // Calculate coordinate delta ( coordinate step in ligh space )
    // NearPlane���� Forward �������� ���� * jitter ���ư� ��ġ�� ����. �� ��ǥ�� x, y�� �ؽ��� ��ǥ, z�� ������ z��.
	float jitter = 1.0;
	vec3 curPosition = posWS.xyz;// +vecForward * jitter;
    shadowUV = LightVP * vec4(curPosition, 1.0);
    coordinates = shadowUV.xyz / shadowUV.w;
	coordinates.xy = (coordinates.xy + vec2(1.0)) * 0.5;
    coordinates.z = dot(curPosition - LightPos, LightForward);

    // NearPlane���� Forward �������� (�� ���� + ���� * jitter ���ư� ��ġ)�� ����. �� ��ǥ�� x, y�� �ؽ��� ��ǥ, z�� ������ z��.
	curPosition = posWS.xyz + vecForward;// *(1.0 + jitter);
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

	float l = length(vecForward);
	vec3 coordinatesStart;
	vec3 coordinatesEnd;

	float coordinateZ_end;

	bool optimizeOn = true;

	int i = 0;
	for (; i < stepsNum; i++)
	{
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

		vec2 sampleMinMax = textureLod(tex_object3, coordinates.xy, 0).xy;

		float ShadowMapWorld = texture2D(tex_object2, coordinates.xy).x;
		sampleFine = float(ShadowMapWorld > coordinates.z);

		float zStart = textureLod(tex_object4, coordinates.xy, 0).x;
		const float transactionScale = 100.0;

		// Add some attenuation for smooth light fading out
		float attenuation = (coordinates.z - zStart) / ((sampleMinMax.y + transactionScale) - zStart);
		attenuation = clamp(attenuation, 0.0, 1.0);
		attenuation = 1.0 - attenuation;
		attenuation *= attenuation;

		float attenuation2 = ((zStart + transactionScale) - coordinates.z) * (1.0 / transactionScale);
		attenuation2 = 1.0 - clamp(attenuation2, 0.0, 1.0);
		attenuation *= attenuation2;

		float density = float(zStart < coordinates.z);
		density *= 10.0 * attenuation;
		//density += 0.25;
		sampleFine *= density;

		if (optimizeOn)
		{
			coordinateZ_end = coordinates.z + coordinateDelta.z * longStepScale;
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
	return;
}