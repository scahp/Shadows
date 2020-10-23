#version 330 core

precision mediump float;

in vec3 TexCoord_;
out vec4 color;

uniform samplerCube tex_object;

void main()
{    
    color = texture(tex_object, TexCoord_);
}