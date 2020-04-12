#version 330 core

precision mediump float;

layout(location = 0) in vec2 VertPos;             // [0, 1]

uniform vec2 PixelSize;
uniform vec2 Pos;
uniform vec2 Size;
uniform int UpsideDown;

out vec2 TexCoord_;

void main()
{
    TexCoord_ = VertPos;
    if (UpsideDown > 0)
        ; TexCoord_.y = 1.0 - TexCoord_.y;

    gl_Position.x = (VertPos.x * Size.x + Pos.x) * PixelSize.x;
    gl_Position.y = (VertPos.y * Size.y + Pos.y) * PixelSize.y;

    gl_Position.xy = gl_Position.xy * 2.0 - 1.0;                           // [0, 1] -> [-1, -1]
    gl_Position.z = 0.5;
    gl_Position.w = 1.0;
}
