#version 430 core

precision highp float;

in vec2 TexCoord_;

float fresnelReflectance(vec3 H, vec3 V, float F0)
{
    float base = 1.0 - dot(V, H);
    float exponential = pow(base, 5.0);
    return exponential + F0 * (1.0 - exponential);
}

float PHBeckmann(float ndoth, float m)
{
    float alpha = acos(ndoth);
    float ta = tan(alpha);
    float val = 1.0 / (m * m * pow(ndoth, 4.0)) * exp(-(ta * ta) / (m * m));
    return val;
}

float KS_Skin_Specular(
    vec3 N, // Bumped surface normal
    vec3 L, // Points to light
    vec3 V, // Points to eye
    float m,  // Roughness
    float rho_s // Specular brightness
)
{
    float result = 0.0;
    float ndotl = dot(N, L);
    if (ndotl > 0.0)
    {
        vec3 h = L + V; // Unnormalized half-way vector
        vec3 H = normalize(h);
        float ndoth = clamp(dot(N, H), -1.0, 1.0);
        float PH = PHBeckmann(ndoth, m);
        float F = fresnelReflectance(H, V, 0.02777778);
        float frSpec = max((PH * F) / dot(h, h), 0.0);
        result = ndotl * rho_s * frSpec; // BRDF * dot(N,L) * rho_s  
    }
    return result;
}

float ComputeRhodtTex(vec2 LV_M)
{
	float costheta = LV_M.x;
	float pi = 3.14159265358979324;
	float m = LV_M.y;
	float sum = 0.0;

	int numterms = 80;
	vec3 N = vec3(0.0, 0.0, 1.0);
	vec3 V = vec3(0.0, sqrt(1.0 - costheta * costheta), costheta);
	for (int i = 0; i < numterms; ++i)
	{
        float phip = float(i) / float(numterms - 1) * 2.0 * pi;
        float localsum = 0.0;
        float cosp = cos(phip);
        float sinp = sin(phip);

		for (int j = 0; j < numterms; ++j)
		{
            float thetap = float(j) / float(numterms - 1) * pi / 2.0;
            float sint = sin(thetap);
            float cost = cos(thetap);
            vec3 L = vec3(sinp * sint, cosp * sint, cost);
            localsum += KS_Skin_Specular(N, L, V, m, 1.0) * sint;
		}
        sum += localsum * (pi / 2.0) / float(numterms);
	}
    return sum * (2.0 * pi) / float( numterms );
}

out vec4 color;

void main()
{
    color.x = ComputeRhodtTex(TexCoord_);
}
