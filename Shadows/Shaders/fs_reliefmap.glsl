#version 330 core

precision mediump float;

uniform sampler2D ColorTexture;
uniform sampler2D ReliefTexture;
uniform sampler2D NormalTexture;

uniform int TextureSRGB[1];
uniform int UseTexture;
uniform vec3 WorldSpace_LightDir_ToSurface;
uniform int MappingType;
uniform int DepthBias;
uniform float DepthScale;
uniform int UseShadow;
uniform vec3 WorldSpace_CameraPos;

in vec2 TexCoord_;
in vec4 Color_;
in mat3 TBN_;
in vec3 TangentSpace_LightDir_ToSurface_;
in vec3 WorldPos_;

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
	vec3 normal = normalize(transpose(TBN_) * GetTwoChannelNormal(uv));
	float intensity = clamp(dot(normal, -normalize(WorldSpace_LightDir_ToSurface)), 0.0, 1.0);
	return intensity;
}

// https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-18-relaxed-cone-stepping-relief-mapping
// ray intersect depth map using linear and binary searches depth value stored in alpha channel (black at is object surface)
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

// https://developer.nvidia.com/gpugems/gpugems3/part-iii-rendering/chapter-18-relaxed-cone-stepping-relief-mapping
void ray_intersect_relaxedcone(inout vec3 p, vec3 v)
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

void main()
{
    vec3 TangentSpace_ViewDir_ToSurface_ = TBN_ * normalize(WorldPos_ - WorldSpace_CameraPos);

	bool IsShadow = false;

	// 1. Tracing ray from camera postion
	vec3 CurrentPosition = vec3(TexCoord_,0.0);
	vec3 TangentSpace_ViewDir_ToSurface = normalize(TangentSpace_ViewDir_ToSurface_);
	TangentSpace_ViewDir_ToSurface.z = abs(TangentSpace_ViewDir_ToSurface.z);
	
	ApplyDepthBiasScale(TangentSpace_ViewDir_ToSurface);	// Apply DepthBias and DepthScale

	// 2. Tracing ray
    if (MappingType == 0)		// relief linear
		ray_intersect_relief(CurrentPosition, TangentSpace_ViewDir_ToSurface);
    else if (MappingType == 1)	// relief relaxed cone
		ray_intersect_relaxedcone(CurrentPosition, TangentSpace_ViewDir_ToSurface);

	if (UseShadow > 0)
	{
		// 3. Tracing ray from light position to current position which is previous ray tracing
		vec3 TangentSpace_LightDir_ToSurface = normalize(TangentSpace_LightDir_ToSurface_);

		// To avoid shadow issue when the geometry surface and direction of light are alomst parallel.
		// Using inversion z of light direction instead of abs.
		// TangentSpace_LightDir_ToSurface.z = abs(TangentSpace_LightDir_ToSurface.z);
		TangentSpace_LightDir_ToSurface.z = -TangentSpace_LightDir_ToSurface.z;

		ApplyDepthBiasScale(TangentSpace_LightDir_ToSurface);	// Apply DepthBias and DepthScale

		// Prepare light tracing start position by using both current position and light direction
		vec3 LightCurrentPosition = GetLightTracingStartPointFromCurrentPoint(CurrentPosition, TangentSpace_LightDir_ToSurface);

		// 4. Tracing ray
        if (MappingType == 0)		// relief linear
			ray_intersect_relief(LightCurrentPosition, TangentSpace_LightDir_ToSurface);
        else if (MappingType == 1)	// relief relaxed cone
			ray_intersect_relaxedcone(LightCurrentPosition, TangentSpace_LightDir_ToSurface);

		// Check receiving shadow
		if (LightCurrentPosition.z < CurrentPosition.z - 0.01)
			IsShadow = true;
	}

	// CurrentPosition is in tangent space. so it can be used for texture sampling of both diffuse and normal.
	vec2 uv = CurrentPosition.xy;
	float lightIntensity = GetLightIntensityFromNormalMap(uv);		// Apply normal mapping

	if (IsShadow)
		lightIntensity *= 0.0f;		// Apply shadowing if CurrentPoisition is in shadow

	color = GetDiffuse(uv);			// Apply diffuse
	color.xyz *= lightIntensity;
}
