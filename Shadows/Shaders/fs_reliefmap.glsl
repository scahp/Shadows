#version 330 core

precision mediump float;

uniform sampler2D ColorTexture;
uniform sampler2D ReliefTexture;
uniform sampler2D NormalTexture;

uniform int TextureSRGB[1];
uniform int UseTexture;
uniform vec3 LightDir;

in vec2 TexCoord_;
in vec4 Color_;
in mat3 TBN;
in vec3 LocalViewDir;
in vec3 LocalLightDir;

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


void main()
{
	bool IsShadow = false;

	// Setup ray
	vec3 p = vec3(TexCoord_,0.0);
	vec3 viewDir = normalize(LocalViewDir);
	viewDir.z = abs(viewDir.z);

	bool g_DepthBias = true;
	float g_DepthScale = 0.15f;

	if (g_DepthBias)
	{
		float db=1.0-viewDir.z; 
		db*=db; 
		db*=db; 
		db=1.0-db*db;
		viewDir.xy*=db;
	}

	viewDir.xy *= g_DepthScale;

	// tracing ray
	ray_intersect_relief(p, viewDir);
	//ray_intersect_relaxedcone(p, viewDir);

	vec3 lDir = normalize(LocalLightDir);
	vec3 p2 = vec3(p);
	
//	float t = -p2.z / lDir.z;
//	p2 = p2 + (lDir) * t;

	lDir.z = abs(lDir.z);

	if (g_DepthBias)
	{
		float db=1.0-lDir.z; 
		db*=db; 
		db*=db; 
		db=1.0-db*db;
		lDir.xy*=db;
	}
	lDir.xy *= g_DepthScale;

	p2 = vec3(TexCoord_,0.0);
	ray_intersect_relief(p2, lDir);
	//ray_intersect_relaxedcone(p2, lDir);

//	if (p2.z < p.z - 0.005)
//		IsShadow = true;

	vec2 texcoord = TexCoord_;
	texcoord = p.xy;

	// NormalMap으로 부터 normal을 얻어오고, TBN 매트릭스로 변환시켜줌
	vec3 normal = normalize(transpose(TBN) * GetTwoChannelNormal(texcoord));
	float lightIntensity = clamp(dot(normal, -LightDir), 0.0, 1.0);

	if (IsShadow)
		lightIntensity *= 0.0f;

	if (UseTexture > 0)
	{
		color = texture2D(ColorTexture, texcoord);
		if (TextureSRGB[0] > 0)
			color.xyz = pow(color.xyz, vec3(2.2));
	}
	else
	{
		color = Color_;
	}
 
	color.xyz *= lightIntensity;
}