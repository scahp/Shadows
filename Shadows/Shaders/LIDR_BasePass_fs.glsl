#version 430 core

precision mediump float;

uniform sampler2D LightBuffer;


struct LightData_LIDR
{
    vec3 Pos;
    float Radius;
    vec4 Color;
};

layout (std140) uniform PointLight_LIDR
{
    //LightData_LIDR PointLight[5];
    LightData_LIDR PointLight[255];
};

in vec3 WorldPos_;
in vec4 Color_;
in vec4 ClipSpacePos_;
out vec4 color;

void main()
{
    vec4 Temp = ClipSpacePos_ / ClipSpacePos_.w;
    vec2 Coord = Temp.xy * vec2(0.5) + vec2(0.5);
    vec4 packedLightIndex = texture(LightBuffer, Coord);
    color = vec4(Color_) * vec4(0.2, 0.2, 0.2, 1.0);
    //color = vec4(0.0, 0.0, 0.0, 1.0);

    vec4 unpackConst = vec4(4.0, 16.0, 64.0, 256.0) / 256.0;
    vec4 floorValues = ceil(packedLightIndex * 254.5);

    for (int i = 0; i < 4; ++i)
    {
        packedLightIndex = floorValues * 0.25;
        //packedLightIndex = packedLightIndex * 0.25;
        //packedLightIndex = packedLightIndex * 0.25;
        //packedLightIndex = packedLightIndex * 0.25;
        //color.y = packedLightIndex.x;
        floorValues = floor(packedLightIndex);
        //color.z = floorValues.x;
        vec4 fracParts = packedLightIndex - floorValues;
        //color.w = fracParts.x;
        //fracParts *= 256;
        float tempIndex = dot(fracParts, unpackConst) * 256;
        int lightIndex = int(tempIndex);

        //if (i == 0)
        //    color.x = tempIndex;
        //else if (i == 1)
        //    color.y = tempIndex;
        //else if (i == 2)
        //    color.z = tempIndex;
        //else
        //    color.w = tempIndex;

        //float tempIndex = dot(floorValues, unpackConst);

        float dist = distance(PointLight[lightIndex].Pos, WorldPos_);
        float distSQ = dist * dist;

        //if (lightIndex > 0 && lightIndex < 5)
        if (lightIndex > 0)
            color += (PointLight[lightIndex].Color * (2.0 / distSQ));
    }
}