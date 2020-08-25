#version 330 core

precision highp float;

uniform int TextureSRGB[1];
uniform vec3 ToLight;
uniform float g;
uniform float g2;

//in vec3 Color_;				// The Rayleigh color
//in vec3 Color2_;			// The Mie color
//in vec3 Direction_;
//in vec3 PosW_;
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
	//if (isnan(Color_.x))
	//{
	//	color = vec4(0.0, 1.0, 0.0, 1.0);
	//	return;
	//}

	vec3 Color_;
	vec3 Color2_;
	vec3 Direction_;
	//{
		vec3 Normal = normalize(Normal_);
		vec3 PosW_ = Normal * OuterRadius;

		vec3 ToVertexFromCam = PosW_ - CameraPos;
		float Far = length(ToVertexFromCam);
		ToVertexFromCam /= Far;

		float Near = getNearIntersection(CameraPos, ToVertexFromCam, CameraHeight * CameraHeight, OuterRadius * OuterRadius);

		vec3 RayStart = CameraPos + ToVertexFromCam * Near;
		Far -= Near;

		// Calculate the ray's starting position, then calculate its scattering offset
		float StartAngle = dot(ToVertexFromCam, RayStart) / OuterRadius;
		float StartDepth = exp(-1.0 / AverageScaleDepth);
		float StartOffset = StartDepth * scale(StartAngle);

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
			vec3 Attenuate = exp(-Scatter * (InvWaveLength * Kr4PI + Km4PI));

			CalculartedColor += Attenuate * (Depth * ScaledLength);
			SamplePoint += SampleRay;
		}

		Color_ = CalculartedColor * (InvWaveLength * KrESun);
		Color2_ = CalculartedColor * KmESun;
		Direction_ = CameraPos - PosW_;
	//}

	float cos = dot(ToLight, Direction_) / length(Direction_);
	float cos2 = cos * cos;
	color = vec4(getRayleighPhase(cos2) * Color_ + getMiePhase(cos, cos2, g, g2) * Color2_, 1.0);
	color.a = color.b;
	//if (isinf(color.a) || isnan(color.a))
	//	discard;
	//color = vec4(vec3(getRayleighPhase(cos2)), 1.0);
}
