// EdgeDetectionSobel_fs.glsl
#version 330 core

precision mediump float;

uniform sampler2D tex_object;		// DepthBuffer WorldScale
uniform float CoarseTextureWidthInv;
uniform float CoarseTextureHeightInv;

in vec2 TexCoord_;
out vec4 color;

void main()
{
	float isEdge = 0;

	float offsetX = CoarseTextureWidthInv * 0.5;
	float offsetY = CoarseTextureHeightInv * 0.5;

		
	float c0 = texture(tex_object, TexCoord_).x;
	
	float c1 = texture(tex_object, TexCoord_ + vec2( offsetX, 0) ).x;
	float c2 = texture(tex_object, TexCoord_ + vec2( 0,-offsetY) ).x;
	float c3 = texture(tex_object, TexCoord_ + vec2(-offsetX, 0) ).x;
	float c4 = texture(tex_object, TexCoord_ + vec2( 0, offsetY) ).x;
	
	float c5 = texture(tex_object, TexCoord_ + vec2( offsetX, offsetY) ).x;
	float c6 = texture(tex_object, TexCoord_ + vec2( offsetX,-offsetY) ).x;
	float c7 = texture(tex_object, TexCoord_ + vec2(-offsetX,-offsetY) ).x;
	float c8 = texture(tex_object, TexCoord_ + vec2(-offsetX, offsetY) ).x;
	
	float c9  = texture(tex_object, TexCoord_ + vec2(-2.0 * offsetX, -2.0 * offsetY) ).x;
	float c10 = texture(tex_object, TexCoord_ + vec2(-1.0 * offsetX, -2.0 * offsetY) ).x;
	float c11 = texture(tex_object, TexCoord_ + vec2( 0.0 * offsetX, -2.0 * offsetY) ).x;
	float c12 = texture(tex_object, TexCoord_ + vec2( 1.0 * offsetX, -2.0 * offsetY) ).x;
	float c13 = texture(tex_object, TexCoord_ + vec2( 2.0 * offsetX, -2.0 * offsetY) ).x;

	float c14 = texture(tex_object, TexCoord_ + vec2( -2.0 * offsetX, 2.0 * offsetY) ).x;
	float c15 = texture(tex_object, TexCoord_ + vec2( -1.0 * offsetX, 2.0 * offsetY) ).x;
	float c16 = texture(tex_object, TexCoord_ + vec2(  0.0 * offsetX, 2.0 * offsetY) ).x;
	float c17 = texture(tex_object, TexCoord_ + vec2(  1.0 * offsetX, 2.0 * offsetY) ).x;
	float c18 = texture(tex_object, TexCoord_ + vec2(  2.0 * offsetX, 2.0 * offsetY) ).x;

	float c19 = texture(tex_object, TexCoord_ + vec2( -2.0 * offsetX, -1.0 * offsetY) ).x;
	float c20 = texture(tex_object, TexCoord_ + vec2(  2.0 * offsetX, -1.0 * offsetY) ).x;
	float c21 = texture(tex_object, TexCoord_ + vec2( -2.0 * offsetX,  0.0 * offsetY) ).x;
	float c22 = texture(tex_object, TexCoord_ + vec2(  2.0 * offsetX,  0.0 * offsetY) ).x;
	float c23 = texture(tex_object, TexCoord_ + vec2( -2.0 * offsetX,  1.0 * offsetY) ).x;
	float c24 = texture(tex_object, TexCoord_ + vec2(  2.0 * offsetX,  1.0 * offsetY) ).x;

	// Apply Sobel 5x5 edge detection filter
	float Gx = 1.0 * ( -c9 -c14 + c13 + c18 ) + 2.0 * ( -c19 -c23 - c10 - c15 + c12 + c17 + c20 + c24 ) + 3.0 * ( -c21 -c7 -c8 + c6 + c5 + c22 ) + 5.0 * ( -c3 + c1 );
	float Gy = 1.0 * ( -c14 -c18 + c9 + c13 ) + 2.0 * ( -c15 -c17 - c23 - c24 + c19 + c20 + c10 + c12 ) + 3.0 * ( -c16 -c8 -c5 + c6 + c7 + c11 ) + 5.0 * ( -c4 + c2 );
	float scale = 0.000025; // Blur scale, can be depth dependent

	color.xy = vec2( Gx * scale, Gy * scale );
}