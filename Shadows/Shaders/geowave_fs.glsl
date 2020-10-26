#version 330 core

precision mediump float;

uniform samplerCube tex_object;

in vec4 ModColor_;
in vec4 AddColor_;
out vec4 color;

in vec4 BTN_X_; // Binormal.x, Tangent.x, Normal.x
in vec4 BTN_Y_; // Bin.y, Tan.y, Norm.y
in vec4 BTN_Z_; // Bin.z, Tan.z, Norm.z


void main()
{
    vec3 normal = normalize(vec3(BTN_X_.x, BTN_Y_.y, BTN_Z_.z));
    vec3 view = normalize(vec3(BTN_X_.w, BTN_Y_.w, BTN_Z_.w));
    vec3 vReflect = reflect(view, normal);
    color = ModColor_ * texture(tex_object, vReflect) + AddColor_;
}