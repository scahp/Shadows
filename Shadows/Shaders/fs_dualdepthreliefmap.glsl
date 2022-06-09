#version 330 core

precision mediump float;

uniform sampler2D ColorTexture;
uniform sampler2D ReliefTexture;		// x : front relief depth, y : back relief depth

uniform int TextureSRGB[1];
uniform int UseTexture;
uniform int DepthBias;
uniform float DepthScale;
uniform vec3 WorldSpace_CameraPos;

in vec2 TexCoord_;
in vec4 Color_;
in mat3 TBN_;
in vec3 WorldPos_;

out vec4 color;

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

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
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

	// This option is specialized for ue5 city sample's asset.
	dualdepth.x = 1.0 - dualdepth.x;	// x는 카메라에서 가까울 수록 큼
	dualdepth.y = dualdepth.y;			// y는 카메라에서 멀수록 큼
	return dualdepth;
}

bool ShouldContinue(vec2 dualdepth)
{
	return (dot(dualdepth.x, dualdepth.y) <= 0);
}

bool IsInTexUV(vec2 uv)
{
	return (uv.x > 0 && uv.x < 1 && uv.y > 0 && uv.y < 1);
}

bool CheckUV(vec3 p, vec2 tex)
{
	return (p.z>tex.x && p.z<tex.y);
}

// ray tracing dual depth relief until intersecting
bool ray_intersect_dual_depth_relief(inout vec3 p, vec3 direction)
{
	//const int num_steps_lin=15;
    const int num_steps_lin = 128;
    const int num_steps_bin = 6;
	
    direction /= direction.z * num_steps_lin;

    bool found = false;
    for (int i = 0; i < num_steps_lin; i++)
    {
        vec2 tex = GetRelief(p.xy);
        if (CheckUV(p, tex) && IsInTexUV(p.xy))
        {
            found = true;
            break;
        }
        else
        {
            p += direction;
        }
    }
	
    return found;
}

void main()
{
    vec3 TangentSpace_ViewDir_ToSurface_ = TBN_ * normalize(WorldPos_ - WorldSpace_CameraPos);
	
	// 1. Tracing ray from camera postion
    vec3 CurrentPosition = vec3(TexCoord_, 0.0);
	vec3 TangentSpace_ViewDir_ToSurface = normalize(TangentSpace_ViewDir_ToSurface_);
	TangentSpace_ViewDir_ToSurface.z = abs(TangentSpace_ViewDir_ToSurface.z);

	ApplyDepthBiasScale(TangentSpace_ViewDir_ToSurface);	// Apply DepthBias and DepthScale

	// 2. Tracing ray
    if (ray_intersect_dual_depth_relief(CurrentPosition, TangentSpace_ViewDir_ToSurface))
    {
        color = GetDiffuse(CurrentPosition.xy); // Apply diffuse
        color.xyz = Uncharted2Tonemap(color.xyz);
    }
}