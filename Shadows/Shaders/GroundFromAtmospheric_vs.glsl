#version 330 core

precision highp float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;
layout(location = 2) in vec2 TexCoord;

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
out vec3 Normal_;
out vec3 PosW_;
out vec3 Pos_;
out vec2 TexCoord_;

float scale(float cos)
{
	float x = 1.0 - cos;
	return AverageScaleDepth * exp(-0.00287 + x * (0.459 + x * (3.83 + x * (-6.80 + x * 5.25))));
}

void main()
{
	gl_Position = MVP * vec4(Pos, 1.0);
	PosW_ = (M * vec4(Pos, 1.0)).xyz;
	Pos_ = Pos;
	TexCoord_ = TexCoord;
	Normal_ = Normal;

	//{
	//	vec3 ToVertexFromCam = PosW_ - CameraPos;
	//	float Far = length(ToVertexFromCam);
	//	ToVertexFromCam /= Far;

	//	float B = 2.0 * dot(CameraPos, ToVertexFromCam);
	//	float C = (CameraHeight * CameraHeight) - (OuterRadius * OuterRadius);
	//	float Det = max(0.0, B * B - 4.0 * C);
	//	float Near = 0.5 * (-B - sqrt(Det));

	//	vec3 RayStart = CameraPos + ToVertexFromCam * Near;
	//	Far -= Near;

	//	// Calculate the ray's starting position, then calculate its scattering offset
	//	float Depth = exp((InnerRadius - OuterRadius) / AverageScaleDepth);
	//	float CameraAngle = dot(-ToVertexFromCam, PosW_) / length(PosW_);
	//	float LightAngle = dot(ToLight, PosW_) / length(PosW_);
	//	float CameraScale = scale(CameraAngle);
	//	float LightScale = scale(LightAngle);
	//	float CameraOffset = Depth * CameraScale;
	//	float Temp = (LightScale + CameraScale);

	//	const int Samples = 2;
	//	float SampleLength = Far / float(Samples);
	//	float ScaledLength = SampleLength * Scale;
	//	vec3 SampleRay = ToVertexFromCam * SampleLength;
	//	vec3 SamplePoint = RayStart + SampleRay * 0.5;

	//	vec3 CalculartedColor = vec3(0.0);
	//	vec3 Attenuate;
	//	for (int i = 0; i < Samples; ++i)
	//	{
	//		float Height = length(SamplePoint);
	//		float Depth = exp(ScaleOverAverageScaleDepth * (InnerRadius - Height));
	//		float Scatter = Depth * Temp - CameraOffset;

	//		Attenuate = exp(-Scatter * (InvWaveLength * Kr4PI + Km4PI));
	//		CalculartedColor += Attenuate * (Depth * ScaledLength);

	//		SamplePoint += SampleRay;
	//	}

	//	Color_ = CalculartedColor * (InvWaveLength * KrESun + KmESun);
	//	Color2_ = Attenuate;
	//	Direction_ = CameraPos - PosW_;
	//}
}
