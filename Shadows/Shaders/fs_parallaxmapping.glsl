#version 330 core

precision mediump float;

uniform sampler2D tex_object;		// diffuse
uniform sampler2D tex_object2;		// normalmap
uniform sampler2D tex_object3;		// height map
uniform int TextureSRGB[1];
uniform int UseTexture;
uniform int FlipedYNormalMap;		// to support converting normalmap type between dx and gl.

uniform int TexturemappingType;
uniform vec3 LightDirection;		// from light to location
uniform vec2 TextureSize;
uniform float HeightScale;			// s = (max height / 1 texel with), this was using when the normalmap was generated.
uniform float NumOfSteps;			// Num of ParallaxMap iteration steps

in vec2 TexCoord_;
in vec4 Color_;
in mat3 TBN;
in vec3 TangentSpaceViewDir;

out vec4 color;		// final color of fragment shader.

// Fetching normal from normal map
vec3 GetNormal(vec2 uv)
{
	vec3 normal = texture(tex_object2, uv).xyz;
	if (FlipedYNormalMap > 0)
		normal.y = 1.0 - normal.y;
	normal = normal * 2.0 - 1.0;
	return normal;
}

// Shifting UV by using Parallax Mapping
vec2 ApplyParallaxOffset(vec2 uv, vec3 vDir, vec2 scale)
{
	vec2 pdir = vDir.xy * scale;
	for (int i = 0; i < NumOfSteps; ++i)
	{
		// This code can be replaced with fetching parallax map for parallax variable(h * nz)
		float nz = GetNormal(uv).z;
		float h = (texture(tex_object3, uv).x * 2 - 1);
		float parallax = nz * h;
		/////////////////////////////////

		uv += pdir * parallax;
	}

	return uv;
}

void main()
{
	vec2 uv = TexCoord_;
	
	// Parallax Mapping을 사용하는 경우 UV를 조정함.
	if (TexturemappingType == 2)
	{
		vec2 scale = HeightScale / (2.0 * NumOfSteps * TextureSize);
		uv = ApplyParallaxOffset(uv, TangentSpaceViewDir, scale);
		uv = clamp(uv, vec2(0.0), vec2(1.0));
	}

	// NormalMap으로 부터 normal을 얻어오고, TBN 매트릭스로 변환시켜줌
	vec3 normal = GetNormal(uv);	
	normal = normalize(TBN * normal);

	// 라이팅 연산 수행
	float LightIntensity = 1.0f;
	if (TexturemappingType > 0)
	{
		LightIntensity = clamp(dot(normal, -LightDirection), 0.0, 1.0);
	}
	else
	{
		LightIntensity = clamp(dot(vec3(0.0, 1.0, 0.0), -LightDirection), 0.0, 1.0);
	}
	
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

	color.xyz *= vec3(LightIntensity);
}