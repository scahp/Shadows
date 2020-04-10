#version 330 core
precision mediump float;

uniform sampler2D tex_object;
uniform sampler2D tex_object2;

uniform float Delta;
uniform int EnableSDF;
uniform int ShowSDF;
uniform int EnableOutline;

in vec2 TexCoord_;
out vec4 FragColor;

void main()
{
    if (ShowSDF > 0)
    {
        FragColor = texture(tex_object2, TexCoord_);
        return;
    }

    FragColor = texture(tex_object, TexCoord_);

    if (EnableSDF > 0)
    {
        // Get a value from the distance field texture
        float RawAlpha = 1.0 - texture(tex_object2, TexCoord_).r;
        if ((RawAlpha - (0.5 - Delta)) < 0.0)
        {
            if (EnableOutline > 0)
            {
                if (RawAlpha <= 0.2)    // set the outline size here.
                    discard;

                FragColor = vec4(0.0, 1.0, 0.0, 1.0);
            }
            else
            {
                discard;
            }
            return;
        }

        FragColor = vec4(FragColor.xyz, smoothstep(0.5 - Delta, 0.5 + Delta, RawAlpha));
    }
}
