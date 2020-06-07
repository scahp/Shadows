#version 330 core

precision mediump float;

uniform sampler2D tex_object;
uniform vec3 LightPos;
uniform vec3 LightForward;
uniform mat4 VPInv;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	float depth = texture2D(tex_object, TexCoord_).x;

    vec4 clipPos;
    clipPos.x = 2.0 * TexCoord_.x - 1.0;
    clipPos.y = -2.0 * TexCoord_.y + 1.0;
    clipPos.z = depth;
    clipPos.w = 1.0;

    vec4 posWS = VPInv * clipPos;
    posWS /= posWS.w;

    float WorldZ = dot(posWS.xyz - LightPos, LightForward);
    color = vec4(WorldZ, WorldZ, WorldZ, 1.0);
}