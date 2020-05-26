#version 330 core

precision highp float;

uniform float ModelScale;

in vec3 Pos_;
out vec4 color;

void main()
{
    vec3 derivu = dFdx(Pos_);
    vec3 derivv = dFdy(Pos_);

    // 0.001 scales the values to map into [0,1]
    // this depends on the model
    vec2 NomralizeContant = vec2(1.0);
    //vec2 NomralizeContant = vec2(0.1);
    float stretchU = NomralizeContant.x * 1.0 / length(derivu);
    float stretchV = NomralizeContant.y * 1.0 / length(derivv);
    color.xy = vec2(stretchU, stretchV); // A two-component texture 
    color.z = 1.0;
}
