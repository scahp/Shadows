#version 330 core

precision mediump float;

layout(location = 0) in vec3 Pos;
//layout(location = 1) in vec4 Color;

uniform mat4 MV;
uniform float Far;
uniform float Near;
uniform vec3 Diffuse;
uniform int ID;
uniform int TotalNumOfID;

out vec4 Color_;

void main()
{
    ///////////////////////////////////////////////////
    // Hemisphere projection
    // transform the geometry to camera space
    vec3 mpos = (MV * vec4(Pos, 1.0)).xyz;

    // project to a point on a unit hemisphere      
    vec3 hemi_pt = normalize(mpos.xyz);

    // Compute (f-n), but let the hardware divide z by this    
    // in the w component (so premultiply x and y)    
    float f_minus_n = Far - Near;
    gl_Position.xy = hemi_pt.xy * f_minus_n;

    // compute depth proj. independently, using OpenGL orthographic      
    gl_Position.z = (-2.0 * mpos.z - Far - Near);
    gl_Position.w = f_minus_n;
    ///////////////////////////////////////////////////

    float ColorGradient = float(ID) / float(TotalNumOfID);
    Color_ = vec4(vec3(ColorGradient), 1.0);
}