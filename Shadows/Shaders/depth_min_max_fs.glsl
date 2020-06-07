#version 330 core

precision mediump float;

uniform sampler2D tex_object;
uniform vec2 BufferSizeInv;

in vec2 TexCoord_;
out vec4 color;

void main()
{
    vec2 Offset;
    Offset.x = BufferSizeInv.x * 0.25;
    Offset.y = BufferSizeInv.y * 0.25;

    float depth1 = texture2D(tex_object, TexCoord_ + vec2(Offset.x, Offset.y)).x;
    float depth2 = texture2D(tex_object, TexCoord_ + vec2(Offset.x, -Offset.y)).x;
    float depth3 = texture2D(tex_object, TexCoord_ + vec2(-Offset.x, Offset.y)).x;
    float depth4 = texture2D(tex_object, TexCoord_ + vec2(-Offset.x, -Offset.y)).x;

    float minDepth = min(depth1, depth2);
    minDepth = min(minDepth, depth3);
    minDepth = min(minDepth, depth4);

    float maxDepth = max(depth1, depth2);
    maxDepth = max(maxDepth, depth3);
    maxDepth = max(maxDepth, depth4);

    color.xy = vec2(minDepth, maxDepth);
}