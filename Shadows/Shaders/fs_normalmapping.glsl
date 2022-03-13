#version 330 core

precision mediump float;

uniform sampler2D tex_object;		// diffuse
uniform sampler2D tex_object2;		// normalmap
uniform sampler2D tex_object7;		// Ambient Occlusion map
uniform int TextureSRGB[1];
uniform int UseTexture;

uniform vec3 LightDirection;		// from light to location
uniform float AmbientOcclusionScale;

in vec2 TexCoord_;
in vec4 Color_;
in mat3 TBN;

out vec4 color;		// final color of fragment shader.

// Fetching normal from normal map
vec3 GetNormal(vec2 uv)
{
	vec3 normal = texture(tex_object2, uv).xyz;
	normal = normal * 2.0 - 1.0;
	return normal;
}

float ApplyAmbientOcclusion(vec2 uv)
{
	// To adjust cos(a) in shader, AmbientMap has radian.
	return cos(texture2D(tex_object7, uv).x * AmbientOcclusionScale);
}

void main()
{
	vec2 uv = TexCoord_;

	// NormalMap으로 부터 normal을 얻어오고, TBN 매트릭스로 변환시켜줌
	vec3 normal = GetNormal(uv);	
	normal = normalize(TBN * normal);

	// 라이팅 연산 수행
	float LightIntensity = clamp(dot(normal, -LightDirection), 0.0, 1.0);

	// Fetching Diffuse texture
	if (UseTexture > 0)
	{
		color = texture2D(tex_object, uv);
		if (TextureSRGB[0] > 0)
			color.xyz = pow(color.xyz, vec3(2.2));
	}
	else
	{
		color = Color_;
	}

	LightIntensity *= ApplyAmbientOcclusion(uv);
	color.xyz *= vec3(LightIntensity);
}