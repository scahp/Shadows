#version 330 core

precision highp float;

uniform sampler2D tex_object;
uniform float TextureSize;
uniform int MaxSteps;

in vec2 TexCoord_;
in vec3 Pos_;
out vec4 color;

void main()
{
    //////////////// UV Positional Dilation ///////////////////////////
    //** Tex **// Input Texture Object storing Volume Data
    //** TexCoord_ **// Input vec2 for UVs
    //** TextureSize **// Resolution of render target
    //** MaxSteps **// Pixel Radius to search

    float texelsize = 1.0 / TextureSize;
    float mindist = 10000000.0;
    vec2 offsets[8] = vec2[8](vec2(-1,0), vec2(1,0), vec2(0,1), vec2(0,-1), vec2(-1,1), vec2(1,1), vec2(1,-1), vec2(-1,-1));

    vec3 sample = texture2D(tex_object, TexCoord_, 0).xyz;
    vec3 curminsample = sample;

    if (sample.x == 0 && sample.y == 0 && sample.z == 0)
    {
        sample = vec3(0.0);

        int i = 0;
        while (i < MaxSteps)
        {
            i++;
            int j = 0;
            while (j < 8)
            {
                vec2 curUV = TexCoord_ + offsets[j] * texelsize * i;
                vec3 offsetsample = texture2D(tex_object, curUV, 0).xyz;

                if (offsetsample.x != 0 || offsetsample.y != 0 || offsetsample.z != 0)
                {
                    float curdist = length(TexCoord_ - curUV);

                    if (curdist < mindist)
                    {
                        vec2 projectUV = curUV + offsets[j] * texelsize * i * 0.25;
                        vec3 direction = texture2D(tex_object, projectUV, 0).xyz;
                        mindist = curdist;

                        //if (direction.x != 0 || direction.y != 0 || direction.z != 0)
                        //{
                        //    vec3 delta = offsetsample - direction;
                        //    curminsample = offsetsample + delta * 4;
                        //}
                        //else
                        {
                            curminsample = offsetsample;
                        }
                    }
                }
                j++;
            }
        }
    }

    color.xyz = curminsample;
}
