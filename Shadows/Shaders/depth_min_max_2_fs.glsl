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

    vec2 depth1 = texture2D(tex_object, TexCoord_ + vec2(Offset.x, Offset.y)).xy;
    vec2 depth2 = texture2D(tex_object, TexCoord_ + vec2(Offset.x, -Offset.y)).xy;
    vec2 depth3 = texture2D(tex_object, TexCoord_ + vec2(-Offset.x, Offset.y)).xy;
    vec2 depth4 = texture2D(tex_object, TexCoord_ + vec2(-Offset.x, -Offset.y)).xy;

    float minDepth = min(depth1.x, depth2.x);
    minDepth = min(minDepth, depth3.x);
    minDepth = min(minDepth, depth4.x);

    float maxDepth = max(depth1.y, depth2.y);
    maxDepth = max(maxDepth, depth3.y);
    maxDepth = max(maxDepth, depth4.y);

    color.xy = vec2(minDepth, maxDepth);
}