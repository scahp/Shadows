#version 330 core

precision highp float;

uniform int TextureSRGB[1];
uniform vec3 ToLight;
uniform float g;
uniform float g2;

in vec3 Normal_;
out vec4 color;

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

float scale(float cos)
{
	float x = 1.0 - cos;
	return AverageScaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

void main()
{
	// todo : It something wired, because if I use normalized Normal vector, Gradient layers appeare in atmsphere
	//        But In Ground shader, if I didn't use normalized Normal, Patch patterns appear in atmospher...
	//		  It should be resolve.
	//vec3 Normal = normalize(Normal_);
	vec3 Normal = Normal_;
	vec3 PosW = Normal * OuterRadius;

	vec3 ToVertexFromCam = PosW - CameraPos;
	float Far = length(ToVertexFromCam);
	ToVertexFromCam /= Far;

	vec3 RayStart = CameraPos;

	float Height = length(RayStart);
	float Depth = exp(ScaleOverAverageScaleDepth * (InnerRadius - CameraHeight));
	float StartAngle = dot(ToVertexFromCam, RayStart) / Height;
	float StartOffset = Depth * scale(StartAngle);

	const int Samples = 3;
	float SampleLength = Far / float(Samples);
	float ScaledLength = SampleLength * Scale;
	vec3 SampleRay = ToVertexFromCam * SampleLength;
	vec3 SamplePoint = RayStart + SampleRay * 0.5;

	vec3 CalculartedColor = vec3(0.0);
	for (int i = 0; i < Samples; ++i)
	{
		float Height = length(SamplePoint);
		float Depth = exp(ScaleOverAverageScaleDepth * (InnerRadius - Height));
		float LightAngle = dot(ToLight, SamplePoint) / Height;
		float CameraAngle = dot(ToVertexFromCam, SamplePoint) / Height;
		float Scatter = StartOffset + Depth * (scale(LightAngle) - scale(CameraAngle));
		vec3 Attenuate = exp(-Scatter * (InvWaveLength * Kr4PI + Km4PI)) * ScatterColor;

		CalculartedColor += Attenuate * (Depth * ScaledLength);
		SamplePoint += SampleRay;
	}

	vec3 RayleighColor = CalculartedColor * (InvWaveLength * KrESun);
	vec3 MieColor = CalculartedColor * KmESun;
	vec3 ToCameraFromPosW = CameraPos - PosW;

	float cos = dot(ToLight, ToCameraFromPosW) / length(ToCameraFromPosW);
	float cos2 = cos * cos;
	color = vec4(getRayleighPhase(cos2) * RayleighColor + getMiePhase(cos, cos2, g, g2) * MieColor, 1.0);
	color.a = 1.0;
}