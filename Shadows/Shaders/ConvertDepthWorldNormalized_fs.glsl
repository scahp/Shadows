// ConvertDepthWorldNormalized_fs.glsl
#version 330 core
precision mediump float;

uniform sampler2D tex_object;
uniform vec3 EyePos;
uniform vec3 WorldFront;
uniform mat4 VPInv;

in vec2 TexCoord_;
out vec4 FragColor;

void main()
{
	float sceneDepth = texture(tex_object, TexCoord_).x;
	
    vec4 clipPos;
	clipPos.x = 2.0 * TexCoord_.x - 1.0;
	clipPos.y = 2.0 * TexCoord_.y - 1.0;
	clipPos.z = 2.0 * sceneDepth - 1.0;
	clipPos.w = 1.0;
						
	vec4 positionWS = VPInv * clipPos;
	positionWS.w = 1.0 / positionWS.w;
	positionWS.xyz *= positionWS.w;

	FragColor.x = dot( positionWS.xyz - EyePos, WorldFront );
}