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
	float a = 1.0 - cos;
	return AverageScaleDepth * exp(-0.00287 + a * (0.459 + a * (3.83 + a * (-6.80 + a * 5.25))));
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
	vec3 RayStart = CameraPos;

	float Depth = exp((InnerRadius - OuterRadius) / AverageScaleDepth);
	float CameraAngle = dot(-ToVertexFromCam, PosWNorm);
	float LightAngle = dot(ToLight, PosWNorm);
	float CameraScale = scale(CameraAngle);
	float LightScale = scale(LightAngle);
	float CameraOffset = Depth * CameraScale;
	float Temp = (LightScale + CameraScale);

	const int Samples = 3;
	float SampleLength = Far / float(Samples);
	float ScaledLength = SampleLength * Scale;
	vec3 SampleRay = ToVertexFromCam * SampleLength;
	vec3 SamplePoint = RayStart + SampleRay * 0.5;

	vec3 CalculartedColor = vec3(0.0);
	vec3 Attenuate;
	for (int i = 0; i < Samples; ++i)
	{
		float Height = length(SamplePoint);
		float Depth = exp(ScaleOverAverageScaleDepth * (InnerRadius - Height));
		float Scatter = Depth * Temp - CameraOffset;

		Attenuate = exp(-Scatter * (InvWaveLength * Kr4PI + Km4PI)) * ScatterColor;
		CalculartedColor += Attenuate * (Depth * ScaledLength);
		SamplePoint += SampleRay;
	}

	vec3 RayleighColor = CalculartedColor * (InvWaveLength * KrESun + KmESun);
	vec3 MieColor = Attenuate;

	vec3 diffuseColor = texture(tex_object, TexCoord_).xyz;
	color = vec4(RayleighColor + diffuseColor.xyz * MieColor, 1.0);
}
