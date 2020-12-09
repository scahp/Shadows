#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
layout(location = 1) in vec3 Normal;

uniform mat4 MVP;
uniform mat4 VP;
uniform mat4 M;

out vec3 Pos_;
out vec3 Normal_;
out float SH_;

#define PI 3.14
#define SHBasisR 1.0

// Spherical Harmonics Real Form
// https://en.wikipedia.org/wiki/Table_of_spherical_harmonics#Real_spherical_harmonics

// l = 0, m = 0
float SHBasis00(vec3 InDir)
{
    return 0.5 * sqrt(1.0 / PI);
}

// l = 1, m = -1
float SHBasis1_1(vec3 InDir)
{
    return 0.5 * sqrt(3.0 / PI) * InDir.y / SHBasisR;
}

// l = 1, m = 0
float SHBasis10(vec3 InDir)
{
    return 0.5 * sqrt(3.0 / PI) * InDir.z / SHBasisR;
}

// l = 1, m = 1
float SHBasis11(vec3 InDir)
{
    return 0.5 * sqrt(3.0 / PI) * InDir.x / SHBasisR;
}

// l = 2, m = -2
float SHBasis2_2(vec3 InDir)
{
    return 0.5 * sqrt(15.0 / PI) * InDir.x * InDir.y / (SHBasisR * SHBasisR);
}

// l = 2, m = -1
float SHBasis2_1(vec3 InDir)
{
    return 0.5 * sqrt(15.0 / PI) * InDir.y * InDir.z / (SHBasisR * SHBasisR);
}

// l = 2, m = 0
float SHBasis20(vec3 InDir)
{
    return 0.25 * sqrt(5.0 / PI) * (-InDir.x * InDir.x - InDir.y * InDir.y + 2.0 * InDir.z * InDir.z) / (SHBasisR * SHBasisR);
}

// l = 2, m = 1
float SHBasis21(vec3 InDir)
{
    return 0.5 * sqrt(15.0 / PI) * InDir.z * InDir.x / (SHBasisR * SHBasisR);
}

// l = 2, m = 2
float SHBasis22(vec3 InDir)
{
    return 0.25 * sqrt(15.0 / PI) * (InDir.x * InDir.x - InDir.y * InDir.y) / (SHBasisR * SHBasisR);
}

uniform int l;
uniform int m;

void main()
{
    int index = l * (l + 1) + m;

    if (index == 0)
        SH_ = SHBasis00(Pos);
    else if (index == 1)
        SH_ = SHBasis1_1(Pos);
    else if (index == 2)
        SH_ = SHBasis10(Pos);
    else if (index == 3)
        SH_ = SHBasis11(Pos);
    else if (index == 4)
        SH_ = SHBasis2_2(Pos);
    else if (index == 5)
        SH_ = SHBasis2_1(Pos);
    else if (index == 6)
        SH_ = SHBasis20(Pos);
    else if (index == 7)
        SH_ = SHBasis21(Pos);
    else if (index == 8)
        SH_ = SHBasis22(Pos);

    vec4 tempPos = M * vec4(Pos * SH_, 1.0);
    Pos_ = tempPos.xyz;
    Normal_ = Normal;
    gl_Position = MVP * vec4(Pos * abs(SH_), 1.0);
}