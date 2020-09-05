#version 330 core

precision highp float;

uniform sampler2D tex_object;
uniform int TextureSRGB[1];
uniform vec3 ToLight;
uniform float g;
uniform float g2;

uniform vec3 InvWaveLength;	// 1 / pow(wavelength, 4)

uniform vec3 CameraPos;
uniform float CameraHeight;

uniform float OuterRadius;
uniform float InnerRadius;
uniform float KrESun;
uniform float KmESun;
uniform float Kr4PI;
uniform float Km4PI;
uniform float Scale;							// 1 / (fOuterRadius - fInnerRadius)
uniform float AverageScaleDepth;
uniform float ScaleOverAverageScaleDepth;		// Scale / AverageScaleDepth
uniform vec3 ScatterColor;

float scale(float cos)
{
	float x = 1.0 - cos;
	return AverageScaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

// Returns the near intersection point of a line and a sphere
float getNearIntersection(vec3 v3Pos, vec3 v3Ray, float fDistance2, float fRadius2)
{
	float B = 2.0 * dot(v3Pos, v3Ray);
	float C = fDistance2 - fRadius2;
	float fDet = max(0.0, B * B - 4.0 * C);
	return 0.5 * (-B - sqrt(fDet));
}

// Returns the far intersection point of a line and a sphere
float getFarIntersection(vec3 v3Pos, vec3 v3Ray, float fDistance2, float fRadius2)
{
	float B = 2.0 * dot(v3Pos, v3Ray);
	float C = fDistance2 - fRadius2;
	float fDet = max(0.0, B * B - 4.0 * C);
	return 0.5 * (-B + sqrt(fDet));
}

// Calculates the Rayleigh phase function
float getRayleighPhase(float cos2)
{
	//return 1.0;
	return 0.75 + 0.75 * cos2;
}

// Calculates the Mie phase function
float getMiePhase(float cos, float cos2, float g, float g2)
{
	return 1.5 * ((1.0 - g2) / (2.0 + g2)) * (1.0 + cos2) / pow(1.0 + g2 - 2.0 * g * cos, 1.5);
}

in vec2 TexCoord_;
in vec3 Normal_;
out vec4 color;

void main()
{
	// todo : It something wired, because if I use normalized Normal vector, Gradient layers appeare in atmsphere
	//        But In Ground shader, if I didn't use normalized Normal, Patch patterns appear in atmospher...
	//		  It should be resolve.
	vec3 Normal = normalize(Normal_);
	vec3 PosW = Normal * InnerRadius;

	vec3 ToVertexFromCam = PosW - CameraPos;
	float Far = length(ToVertexFromCam);
	ToVertexFromCam /= Far;

	vec3 PosWNorm = normalize(PosW);

	// View direction의 반직선과 대기 영역을 나타내는 Sphere간 충돌 지점 중 가까운 그리고 먼 거리 구함
	float Near = getNearIntersection(CameraPos, ToVertexFromCam, CameraHeight * CameraHeight, OuterRadius * OuterRadius);

	vec3 RayStart = CameraPos + ToVertexFromCam * Near;
	Far -= Near;

	// 대기 가장 상단 부분의 대기 밀도 exp(-h/H0)
	float Depth = exp((InnerRadius - OuterRadius) / AverageScaleDepth);
	float CameraAngle = dot(-ToVertexFromCam, PosWNorm);
	float LightAngle = dot(ToLight, PosWNorm);
	float CameraScale = scale(CameraAngle);
	float LightScale = scale(LightAngle);

	// 대기 상단에서의 Optical depth. (대기 상단에서는 대기 밀도가 거의 0에 가깝다. exp(-1)이 대기 최상단인데 대략 0.3678...)
	float CameraOffset = Depth * CameraScale;

	// 두 방향 (Camera방향과 라이트 방향)의 scale function 합산. 여기에 고도만 곱하면, 해당 고도에서의 Optical depth 임.
	float Temp = (LightScale + CameraScale);

	const int Samples = 3;
	
	// 대기를 통과하는 거리를 fSamples 수만큼 나눈다, 이 샘플 숫자만큼 적분을 근사함
	float SampleLength = Far / float(Samples);
	float ScaledLength = SampleLength * Scale;
	vec3 SampleRay = ToVertexFromCam * SampleLength;

	// 샘플 구간의 중심점을 기준으로 적분함, 루프를 돌면서 계속 증가
	vec3 SamplePoint = RayStart + SampleRay * 0.5;

	vec3 CalculartedColor = vec3(0.0);
	vec3 Attenuate;
	for (int i = 0; i < Samples; ++i)
	{
		// 현재 샘플 포인트의 고도
		float Height = length(SamplePoint);

		// 현재 샘플 포인트의 대기 밀도 exp(-h/H0)
		float Depth = exp(ScaleOverAverageScaleDepth * (InnerRadius - Height));

		// 현재 고도에서의 Optical depth 에서 대기 최상단에서의 Optical depth(대략 0.3678...)를 빼줌.
		// 이것 때문에 대기의 상단에 있는 경우 Optical depth를 0이 되어서 In-scattering 되는 양을 0으로 만들어 줌.
		float Scatter = Depth * Temp - CameraOffset;
		Attenuate = exp(-Scatter * (InvWaveLength * Kr4PI + Km4PI)) * ScatterColor;

		// 현재 샘플포인트가 커버하는 구간에서의 In-scaterring 값을 구함
		CalculartedColor += Attenuate * (Depth * ScaledLength);

		// 다음 샘플포인트 위치로 이동
		SamplePoint += SampleRay;
	}

	// 레이리 스케터링 결과를 저장
	vec3 RayleighColor = CalculartedColor * (InvWaveLength * KrESun + KmESun);

	// 미 스케터링 결과를 저장
	vec3 MieColor = Attenuate;

	vec3 diffuseColor = texture(tex_object, TexCoord_).xyz;

	// 레이리 스케터링과 미 스케터링 각각에 맞는 위상함수를 적용함.
	color = vec4(RayleighColor + diffuseColor.xyz * MieColor, 1.0);
}
