#version 330 core

precision mediump float;

in vec4 Color_;
out vec4 color;

uniform vec3 CosmeticLayer1_S;
uniform vec3 CosmeticLayer1_K;
uniform float CosmeticLayer1_X;

uniform vec3 CosmeticLayer2_S;
uniform vec3 CosmeticLayer2_K;
uniform float CosmeticLayer2_X;

/*
R0 = sinh(bSX) / (a sinh(bSX) + b cosh(bSX))
T = b / (a sinh(bSX) + b socsh (BSX))


S = scattering per unit
K = absorption per unit
X = thickness of a layer
a = 1 + (K/S)
b = root(a^2 - 1)

K = (0.22, 1.47, 0.57)
S = (0.05, 0.003, 0.03)
*/

void GetRT(out vec3 R, out vec3 T, vec3 K, vec3 S, float X)
{
    vec3 a = vec3(1.0) + (K / S);
    vec3 b = sqrt(a * a - vec3(1.0));

    vec3 Temp = a * sinh(b * S * X) + b * cosh(b * S * X);

    R = sinh(b * S * X) / Temp;
    T = b / Temp;
}

void main()
{
    vec3 K1 = vec3(0.22, 1.47, 0.57);
    vec3 S1 = vec3(0.05, 0.003, 0.03);

    vec3 R1;
    vec3 T1;
    GetRT(R1, T1, CosmeticLayer1_K, CosmeticLayer1_S, CosmeticLayer1_X);

    vec3 R2;
    vec3 T2;
    GetRT(R2, T2, CosmeticLayer2_K, CosmeticLayer2_S, CosmeticLayer2_X);

    vec3 RSum = R1 + (T1 * R1 * T1) / (vec3(1.0) - R1 * R2);
    vec3 TSum = (T1 * T2) / (vec3(1.0) - R1 * R2);

    color = vec4(Color_);
    color.xyz = RSum + TSum * TSum * color.xyz;
    color.w = 1.0;
}