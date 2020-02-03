#version 400 core

#preprocessor

uniform mat4 MVP;
uniform vec3 Pos;
uniform vec3 BoxMin;
uniform vec3 BoxMax;
uniform vec4 ViewportSize;

uniform sampler2D HiZBuffer;

flat out int Visible;
/*
out vec3 Test1;
out vec3 Test2;
out vec3 Test3;
out vec4 Test4;
*/

bool MakeBoundBox(out vec4 boundingBox[8])
{
	boundingBox[0] = MVP * vec4(Pos + vec3(BoxMax.x, BoxMax.y, BoxMax.z), 1);	// +++
	boundingBox[1] = MVP * vec4(Pos + vec3(BoxMin.x, BoxMax.y, BoxMax.z), 1);	// -++
	boundingBox[2] = MVP * vec4(Pos + vec3(BoxMax.x, BoxMin.y, BoxMax.z), 1);	// +-+
	boundingBox[3] = MVP * vec4(Pos + vec3(BoxMin.x, BoxMin.y, BoxMax.z), 1);	// --+
	boundingBox[4] = MVP * vec4(Pos + vec3(BoxMax.x, BoxMax.y, BoxMin.z), 1);	// ++-
	boundingBox[5] = MVP * vec4(Pos + vec3(BoxMin.x, BoxMax.y, BoxMin.z), 1);	// -+-
	boundingBox[6] = MVP * vec4(Pos + vec3(BoxMax.x, BoxMin.y, BoxMin.z), 1);	// +--
	boundingBox[7] = MVP * vec4(Pos + vec3(BoxMin.x, BoxMin.y, BoxMin.z), 1);	// ---

	int outOfBound[6] = int[6](0, 0, 0, 0, 0, 0);

	for (int i = 0; i < 8; ++i)
	{
		if (boundingBox[i].x > boundingBox[i].w) ++outOfBound[0];
		if (boundingBox[i].x < -boundingBox[i].w) ++outOfBound[1];
		if (boundingBox[i].y > boundingBox[i].w) ++outOfBound[2];
		if (boundingBox[i].y < -boundingBox[i].w) ++outOfBound[3];
		if (boundingBox[i].z > boundingBox[i].w) ++outOfBound[4];
		if (boundingBox[i].z < -boundingBox[i].w) ++outOfBound[5];
	}

	for (int i = 0; i < 6; ++i)
	{
		if (outOfBound[i] >= 8)
			return false;
	}

	return true;
}

int HiZOcclusionCull()
{
	vec4 boundingBox[8];
	if (!MakeBoundBox(boundingBox))
		return 0;

	// Perspective Divide 로 xyz를 값을 NDC 좌표계로 둠
	for (int i=0; i<8; i++)
		boundingBox[i].xyz /= boundingBox[i].w;

	// 화면 공간의 바운드 사각형을 구함
	vec2 boundingRect[2];
	boundingRect[0].x = min( min( min(boundingBox[0].x, boundingBox[1].x ),
								  min(boundingBox[2].x, boundingBox[3].x ) ),
							 min( min(boundingBox[4].x, boundingBox[5].x ),
								  min(boundingBox[6].x, boundingBox[7].x ) ) ) / 2.0 + 0.5;
	boundingRect[0].y = min( min( min(boundingBox[0].y, boundingBox[1].y ),
								  min(boundingBox[2].y, boundingBox[3].y ) ),
							 min( min(boundingBox[4].y, boundingBox[5].y ),
								  min(boundingBox[6].y, boundingBox[7].y ) ) ) / 2.0 + 0.5;
	boundingRect[1].x = max( max( max(boundingBox[0].x, boundingBox[1].x ),
								  max(boundingBox[2].x, boundingBox[3].x ) ),
							 max( max(boundingBox[4].x, boundingBox[5].x ),
								  max(boundingBox[6].x, boundingBox[7].x ) ) ) / 2.0 + 0.5;
	boundingRect[1].y = max( max( max(boundingBox[0].y, boundingBox[1].y ),
								  max(boundingBox[2].y, boundingBox[3].y ) ),
							 max( max(boundingBox[4].y, boundingBox[5].y ),
								  max(boundingBox[6].y, boundingBox[7].y ) ) ) / 2.0 + 0.5;

	// boundingBox에서 화면과 가장 가까이 있는 z 값을 가져옴
	float InstanceDepth = min( min( min(boundingBox[0].z, boundingBox[1].z ),
									min(boundingBox[2].z, boundingBox[3].z ) ),
							   min( min(boundingBox[4].z, boundingBox[5].z ),
									min(boundingBox[6].z, boundingBox[7].z ) ) );
									
	InstanceDepth = (InstanceDepth + 1.0) / 2.0;

	// 뷰포트 크기 기준으로 바운딩 사각형의 크기를 구함
	float ViewSizeX = (boundingRect[1].x- boundingRect[0].x) * ViewportSize.z;
	float ViewSizeY = (boundingRect[1].y- boundingRect[0].y) * ViewportSize.w;
	
	// Depth Buffer Texture의 LOD(mip-level)을 계산.
	float LOD = ceil( log2( max( ViewSizeX, ViewSizeY ) / 2.0 ) );
	
	// 바운드 사각형의 NDC 좌표를 위에서 얻어진 Hi-z 의 LOD에 조회함.
	// 바운드 사각형의 4 모서리에 대해서 전부 조회하고, 그중에 화면과 가장 먼값을 가져옴.
	vec4 Samples;
	Samples.x = textureLod( HiZBuffer, vec2(boundingRect[0].x, boundingRect[0].y), LOD ).x;
	Samples.y = textureLod( HiZBuffer, vec2(boundingRect[0].x, boundingRect[1].y), LOD ).x;
	Samples.z = textureLod( HiZBuffer, vec2(boundingRect[1].x, boundingRect[1].y), LOD ).x;
	Samples.w = textureLod( HiZBuffer, vec2(boundingRect[1].x, boundingRect[0].y), LOD ).x;
	float MaxDepth = max( max( Samples.x, Samples.y ), max( Samples.z, Samples.w ) );
/*	
	Test1.x = InstanceDepth;
	Test1.y = MaxDepth;
	Test1.z = LOD;
	Test2.x = boundingRect[1].x- boundingRect[0].x;
	Test2.y = boundingRect[1].y- boundingRect[0].y;
	Test2.z = boundingRect[0].x;
	Test3.x = boundingRect[0].y;
	Test3.y = boundingRect[1].x;
	Test3.z = boundingRect[1].y;
	Test4.x = ViewportSize.z;
	Test4.y = ViewportSize.w;
	Test4.z = ViewSizeX;
	Test4.w = ViewSizeY;
*/	
	// Hi-z 에 저장된 깊이 값중 가장 큰값보다 현재 바운드 사각형의 깊이 값이 더 크다면, discard 되야함.
	return ( InstanceDepth > MaxDepth ) ? 0 : 1;
}

void main()
{
	Visible = HiZOcclusionCull();
}