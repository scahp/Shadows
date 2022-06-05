#version 330 core

precision mediump float;

uniform sampler2D ColorTexture;
uniform sampler2D ReliefTexture;
uniform sampler2D NormalTexture;

uniform int TextureSRGB[1];
uniform int UseTexture;
uniform vec3 WorldSpace_LightDir_ToSurface;
uniform int ReliefTracingType;
uniform int DepthBias;
uniform float DepthScale;
uniform int UseShadow;
uniform mat4 InvM;
uniform vec3 LocalSpace_CameraPos;

uniform vec3 WorldSpace_CameraPos;

in vec2 TexCoord_;
in vec4 Color_;
in mat3 TBN_;
//in vec3 TangentSpace_ViewDir_ToSurface_;
in vec3 TangentSpace_LightDir_ToSurface_;
in vec3 WorldPos_;
in vec3 LocalPos_;

out vec4 color;

vec3 GetNormal(vec2 uv)
{
	vec3 normal = texture(ReliefTexture, uv).xyz;
	normal = normal * 2.0 - 1.0;
	normal.y = -normal.y;			// if it's using opengl normal map
	return normal;
}

vec3 GetTwoChannelNormal(vec2 uv)
{
	vec2 normal = texture(ReliefTexture, uv).xy;
	normal = normal * 2.0 - 1.0;
	normal.y = -normal.y;			// if it's using opengl normal map
	return vec3(normal.x, normal.y, sqrt(1.0 - normal.x*normal.x - normal.y*normal.y));
}

vec4 GetDiffuse(vec2 uv)
{
	vec4 DiffuseColor;
	if (UseTexture > 0)
	{
		DiffuseColor = texture2D(ColorTexture, uv);
		if (TextureSRGB[0] > 0)
			DiffuseColor.xyz = pow(color.xyz, vec3(2.2));
	}
	else
	{
		DiffuseColor = Color_;
	}
	return DiffuseColor;
}

float GetLightIntensityFromNormalMap(vec2 uv)
{
#define TANGENT_SPACE_NORMAL_MAPPING 1

#if TANGENT_SPACE_NORMAL_MAPPING
	// Do the normal mapping by using tangent space light direction.
	return clamp(dot(GetTwoChannelNormal(uv), -TangentSpace_LightDir_ToSurface_), 0.0, 1.0);
#else
	// Do the normal mapping by using world space light direction.
	// NormalMap으로 부터 normal을 얻어오고, TBN 매트릭스로 월드공간으로 변환시켜줌
	vec3 normal = normalize(transpose(TBN_) * GetTwoChannelNormal(uv));
	return clamp(dot(normal, -normalize(WorldSpace_LightDir_ToSurface)), 0.0, 1.0);
#endif
}

// ray intersect depth map using linear and binary searches
// depth value stored in alpha channel (black at is object surface)
void ray_intersect_relief(inout vec3 p, vec3 v)
{
	const int num_steps_lin=15;
	const int num_steps_bin=6;
	
	v /= v.z*num_steps_lin;
	
	int i;
	for( i=0;i<num_steps_lin;i++ )
	{
		vec4 tex = texture(ReliefTexture, p.xy);
		if (p.z<tex.w)
			p+=v;
	}
	
	for( i=0;i<num_steps_bin;i++ )
	{
		v *= 0.5;
		vec4 tex = texture(ReliefTexture, p.xy);
		if (p.z<tex.w)
			p+=v;
		else
			p-=v;
	}
}

void ray_intersect_relaxedcone(inout vec3 p, inout vec3 v)
{
	const int cone_steps=15;
	const int binary_steps=8;
	
	vec3 p0 = p;

	v /= v.z;
	
	float dist = length(v.xy);
	
	for( int i=0;i<cone_steps;i++ )
	{
		vec4 tex = texture(ReliefTexture, p.xy);
		float height = clamp(tex.w - p.z, 0.0, 1.0);		
		float cone_ratio = tex.z;		
		p += v * (cone_ratio * height / (dist + cone_ratio));
	}

	v *= p.z*0.5;
	p = p0 + v;

	for( int i=0;i<binary_steps;i++ )
	{
		vec4 tex = texture(ReliefTexture, p.xy);
		v *= 0.5;
		if (p.z<tex.w)
			p+=v;
		else
			p-=v;
	}
}

vec3 GetLightTracingStartPointFromCurrentPoint(vec3 CurrentPosition, vec3 LightDirectionToSurface)
{
	// In the tangent space of texture, the range of z is from 0 to 1.
	// So We can find 'light tracing start point' by using both current position and light direction in tangent space.

	// by using parameteric equation of line. we can get parameter 't' of line.
	// TargetZ = StartPointZ + t * DirectionZ, and We want to find 't' at 'StartPointZ is 0'.
	// => t = TargetZ / DirectionZ
	float t = CurrentPosition.z / LightDirectionToSurface.z;

	// Get the StartPoint by using 't', current position and light direciotn.
	// Target = StartPosition + t * Direction;
	// => StartPosition = Target - t * Direction;
	vec3 StartPosition = CurrentPosition - t * LightDirectionToSurface;

	return StartPosition;
}

void ApplyDepthBiasScale(inout vec3 Direction)
{
	if (DepthBias > 0)
	{
		float db = 1.0 - Direction.z;
		db *= db;
		db *= db;
		db = 1.0 - db * db;
		Direction.xy *= db;
	}

	Direction.xy *= DepthScale;
}

#define USE_RELAXED_CONE_TRACING 1

vec3 Uncharted2Tonemap(vec3 x)
{
	float A = 0.15;		// Shoulder Strength
	float B = 0.50;		// Linear Strength
	float C = 0.10;		// Linear Angle
	float D = 0.20;		// Toe Strength
	float E = 0.02;		// Toe Numerator
	float F = 0.30;		// Toe Denominator
	float W = 11.2;		// Linear White Point Value

	return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

vec2 GetRelief(vec2 uv)
{
	vec2 dualdepth = texture(ReliefTexture, uv.xy).xy;

	// x는 카메라에서 가까울 수록 큼
	dualdepth.x = 1.0 - dualdepth.x;

	// y는 카메라에서 멀수록 큼
	dualdepth.y = dualdepth.y;

	return dualdepth;
}

bool ShouldContinue(vec2 dualdepth)
{
	if (dot(dualdepth.x, dualdepth.y) <= 0)
		return true;
	return false;
}

bool IsInTexUV(vec2 uv)
{
	return (uv.x > 0 && uv.x < 1 && uv.y > 0 && uv.y < 1);
}

bool CheckUV(vec3 p, vec2 tex)
{
	return (
				p.z>tex.x 
				&& p.z<tex.y
				);
}

void main()
{
	#define WORLD_SPACE 0
#if WORLD_SPACE
    vec4 localViewDir = InvM * vec4(normalize(WorldPos_ - WorldSpace_CameraPos), 0.0);
    vec3 TangentSpace_ViewDir_ToSurface_ = TBN_ * localViewDir.xyz;
#else
    vec3 TangentSpace_ViewDir_ToSurface_ = TBN_ * (LocalPos_ - LocalSpace_CameraPos);
#endif

	bool IsShadow = false;
	
    //int row = 0;
    //color.xyz = vec3(TBN_[row][0], TBN_[row][1], TBN_[row][2]);
    //return;
	
	color.xyz = TangentSpace_ViewDir_ToSurface_;
	//return;

	// 1. Tracing ray from camera postion
	// Setup ray
	vec3 CurrentPosition = vec3(TexCoord_,0.0);
	vec3 TangentSpace_ViewDir_ToSurface = normalize(TangentSpace_ViewDir_ToSurface_);
	TangentSpace_ViewDir_ToSurface.z = abs(TangentSpace_ViewDir_ToSurface.z);

	ApplyDepthBiasScale(TangentSpace_ViewDir_ToSurface);	// Apply DepthBias and DepthScale

	vec3 p = CurrentPosition;
	vec3 v = TangentSpace_ViewDir_ToSurface;
	{
		//const int num_steps_lin=15;
		const int num_steps_lin=128;
		const int num_steps_bin=6;
	
		v /= v.z*num_steps_lin;

		bool found = false;

		int i;
		for( i=0;i<num_steps_lin;i++ )
		{
			vec2 tex = GetRelief(p.xy);
			if (CheckUV(p, tex) && IsInTexUV(p.xy))
			{
				found = true;
				break;
			}
			else
			{
				p+=v;
			}
		}

//		color.xyz = p.xyz;
//			return;

		if (!found)
			return;

//		for( i=0;i<num_steps_bin;i++ )
//		{
//			v *= 0.5;
//			vec2 tex = GetRelief(p.xy);
//			if (IsInTexUV(p.xy))
//				p+=v;
//			else
//				p-=v;
//		}
	}


//	// Tracing ray
//	if (ReliefTracingType == 0)
//		ray_intersect_relief(CurrentPosition, TangentSpace_ViewDir_ToSurface);
//	else
//		ray_intersect_relaxedcone(CurrentPosition, TangentSpace_ViewDir_ToSurface);
//
//	if (UseShadow > 0)
//	{
//		// 2. Tracing ray from light position to current position which is previous ray tracing
//		vec3 TangentSpace_LightDir_ToSurface = normalize(TangentSpace_LightDir_ToSurface_);
//
//		// To avoid shadow issue when the geometry surface and direction of light are alomst parallel.
//		// Using inversion z of light direction instead of abs.
//		// TangentSpace_LightDir_ToSurface.z = abs(TangentSpace_LightDir_ToSurface.z);
//		TangentSpace_LightDir_ToSurface.z = -TangentSpace_LightDir_ToSurface.z;
//
//		ApplyDepthBiasScale(TangentSpace_LightDir_ToSurface);	// Apply DepthBias and DepthScale
//
//		// Prepare light tracing start position by using both current position and light direction
//		vec3 LightCurrentPosition = GetLightTracingStartPointFromCurrentPoint(CurrentPosition, TangentSpace_LightDir_ToSurface);
//
//		// Tracing ray
//		if (ReliefTracingType == 0)
//			ray_intersect_relief(LightCurrentPosition, TangentSpace_LightDir_ToSurface);
//		else
//			ray_intersect_relaxedcone(LightCurrentPosition, TangentSpace_LightDir_ToSurface);
//
//		// Check the shadowing
//		if (LightCurrentPosition.z < CurrentPosition.z - 0.01)
//			IsShadow = true;
//	}
//
//	// CurrentPosition is in tangent space. so it can be used for texture sampling of both diffuse and normal.
//	vec2 uv = CurrentPosition.xy;
//	float lightIntensity = GetLightIntensityFromNormalMap(uv);		// Apply normal mapping
//
//	if (IsShadow)
//		lightIntensity *= 0.0f;		// Apply shadowing if CurrentPoisition is in shadow

	color = GetDiffuse(p.xy);			// Apply diffuse
	//color.xyz *= lightIntensity;
	color.xyz = Uncharted2Tonemap(color.xyz);
	//color.xyz = pow(color.xyz, vec3(1.0 / 2.2));
}