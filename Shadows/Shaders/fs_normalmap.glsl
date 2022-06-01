#version 330 core

precision mediump float;

uniform sampler2D ColorTexture;
uniform sampler2D ReliefTexture;
uniform sampler2D NormalTexture;

uniform int TextureSRGB[1];
uniform int UseTexture;
uniform vec3 LightDir;
uniform vec3 CameraPos;

in vec2 TexCoord_;
in vec4 Color_;
in mat3 TBN;

out vec4 color;

vec3 GetNormal(vec2 uv)
{
	vec3 normal = texture(NormalTexture, uv).xyz;
	normal = normal * 2.0 - 1.0;
	normal.y = -normal.y;			// if it's using opengl normal map
	return normal;
}

vec3 GetTwoChannelNormal(vec2 uv)
{
	vec2 normal = texture(NormalTexture, uv).xy;
	normal = normal * 2.0 - 1.0;
	normal.y = -normal.y;			// if it's using opengl normal map
	return vec3(normal.x, normal.y, sqrt(1.0 - normal.x*normal.x - normal.y*normal.y));
}

void main()
{
	// NormalMap으로 부터 normal을 얻어오고, TBN 매트릭스로 변환시켜줌
	vec3 normal = normalize(TBN * GetTwoChannelNormal(TexCoord_));
	float lightIntensity = clamp(dot(normal, -LightDir), 0.0, 1.0);

	if (UseTexture > 0)
	{
		color = texture2D(ColorTexture, TexCoord_);
		if (TextureSRGB[0] > 0)
			color.xyz = pow(color.xyz, vec3(2.2));
	}
	else
	{
		color = Color_;
	}
 
	color.xyz *= lightIntensity;
}