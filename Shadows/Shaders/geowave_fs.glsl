#version 330 core

precision highp float;

uniform samplerCube tex_object;         // Environment map
uniform sampler2D tex_object3;          // Bump map
uniform vec4 ReScale;

in vec4 ModColor_;
in vec4 AddColor_;
in float Fog_;
in vec4 TexCoord0_;

out vec4 color;

in vec4 BTN_X_; // Binormal.x, Tangent.x, Normal.x
in vec4 BTN_Y_; // Binormal.x, Tangent.x, Normal.x
in vec4 BTN_Z_; // Binormal.x, Tangent.x, Normal.x

void main()
{
    vec4 TextureNormal = texture(tex_object3, TexCoord0_.xy);

    float u = dot(BTN_X_.xyz, TextureNormal.xyz);
    float v = dot(BTN_Y_.xyz, TextureNormal.xyz);
    float w = dot(BTN_Z_.xyz, TextureNormal.xyz);

    //float u = dot(vec3(BTN_X_.x, BTN_Y_.x, BTN_Z_.x), TextureNormal.xyz);
    //float v = dot(vec3(BTN_X_.y, BTN_Y_.y, BTN_Z_.y), TextureNormal.xyz);
    //float w = dot(vec3(BTN_X_.z, BTN_Y_.z, BTN_Z_.z), TextureNormal.xyz);

    vec3 Normal = (vec3(u, v, w));

    vec3 view = (vec3(BTN_X_.w, BTN_Y_.w, BTN_Z_.w));
    vec3 vReflect;// = reflect(-view, Normal);
    vReflect = 2.0 * dot(Normal, view) / dot(Normal, Normal) * Normal - view;
    color = (ModColor_ * texture(tex_object, vReflect) + AddColor_);
    color.a = ModColor_.a;
}