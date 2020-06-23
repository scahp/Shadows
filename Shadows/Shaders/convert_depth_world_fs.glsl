#version 330 core

precision mediump float;

uniform sampler2D tex_object;
uniform vec3 LightPos;
uniform vec3 LightForward;
uniform mat4 LightVPInv;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	float depth = texture2D(tex_object, TexCoord_).x;

    vec4 clipPos;
    clipPos.x = 2.0 * TexCoord_.x - 1.0;
    clipPos.y = 2.0 * TexCoord_.y - 1.0;
    clipPos.z = 2.0 * depth - 1.0;          // OpenGL은 NDC 공간이 Z : -1 ~ 1
    clipPos.w = 1.0;

    vec4 posWS = LightVPInv * clipPos;
    posWS /= posWS.w;

    float WorldZ = dot(posWS.xyz - LightPos, LightForward);
    color = vec4(WorldZ, WorldZ, WorldZ, 1.0);
}