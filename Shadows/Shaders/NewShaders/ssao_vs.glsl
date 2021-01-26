#version 330 core
precision mediump float;

layout(location = 0) in float VertID;

out vec2 TexCoord_;

void main()
{
    int vert = int(VertID);

    TexCoord_ = vec2((vert << 1) & 2, vert & 2);
    gl_Position = vec4(TexCoord_ * vec2(2.0, 2.0) - vec2(1.0, 1.0), 0.5, 1.0);
}
