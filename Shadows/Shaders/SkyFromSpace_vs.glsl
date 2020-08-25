#version 330 core

precision highp float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;

uniform mat4 MVP;
uniform mat4 M;
uniform vec3 ToLight;
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

//out vec3 Color_;
//out vec3 Color2_;
//out vec3 Direction_;
out vec3 PosW_;
out vec3 Normal_;

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
	gl_Position = MVP * vec4(Pos, 1.0);
	PosW_ = (M * vec4(Pos, 1.0)).xyz;
	Normal_ = Normal;
	return;

	//vec3 PosW = (M * vec4(Pos, 1.0)).xyz;

	//vec3 ToVertexFromCam = PosW - CameraPos;
	//float Far = length(ToVertexFromCam);
	//ToVertexFromCam /= Far;

	////float B = 2.0 * dot(CameraPos, ToVertexFromCam);
	////float C = (CameraHeight * CameraHeight) - (OuterRadius * OuterRadius);
	////float Det = max(0.0, B * B - 4.0 * C);
	////float Near = 0.5 * (-B - sqrt(Det));

	//float Near = getNearIntersection(CameraPos, ToVertexFromCam, CameraHeight * CameraHeight, OuterRadius * OuterRadius);

	//vec3 RayStart = CameraPos + ToVertexFromCam * Near;
	//Far -= Near;

	//// Calculate the ray's starting position, then calculate its scattering offset
	//float StartAngle = dot(ToVertexFromCam, RayStart) / OuterRadius;
	//float StartDepth = exp(-1.0 / AverageScaleDepth);
	//float StartOffset = StartDepth * scale(StartAngle);

	//const int Samples = 2;
	//float SampleLength = Far / float(Samples);
	//float ScaledLength = SampleLength * Scale;
	//vec3 SampleRay = ToVertexFromCam * SampleLength;
	//vec3 SamplePoint = RayStart + SampleRay * 0.5;

	//vec3 CalculartedColor = vec3(0.0);
	//for (int i = 0; i < Samples; ++i)
	//{
	//	float Height = length(SamplePoint);
	//	float Depth = exp(ScaleOverAverageScaleDepth * (InnerRadius - Height));
	//	float LightAngle = dot(ToLight, SamplePoint) / Height;
	//	float CameraAngle = dot(ToVertexFromCam, SamplePoint) / Height;
	//	float Scatter = Depth * (StartOffset + scale(LightAngle) - scale(CameraAngle));

	//	vec3 Attenuate = exp(-Scatter * (InvWaveLength * Kr4PI + Km4PI));
	//	CalculartedColor += Attenuate * (Depth * ScaledLength);

	//	SamplePoint += SampleRay;
	//}

	//Color_ = CalculartedColor * (InvWaveLength * KrESun);
	//Color2_ = CalculartedColor * KmESun;
	//Direction_ = CameraPos - PosW;	
}
