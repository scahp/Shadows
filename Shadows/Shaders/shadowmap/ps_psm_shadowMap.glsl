#version 330 core

precision mediump float;

void main()
{
    gl_FragData[0].xyz = gl_FragCoord.zzz;
    gl_FragData[0].w = 1.0;
}