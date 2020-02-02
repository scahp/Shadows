#version 400 core

#preprocessor

uniform mat4 MVP;
uniform vec3 Pos;
uniform vec3 BoxMin;
uniform vec3 BoxMax;

uniform sampler2D HiZBuffer;

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
		BoundingBox[i].xyz /= BoundingBox[i].w;

	// 화면 공간의 바운드 사각형을 구함
	vec2 BoundingRect[2];
	BoundingRect[0].x = min( min( min( BoundingBox[0].x, BoundingBox[1].x ),
								  min( BoundingBox[2].x, BoundingBox[3].x ) ),
							 min( min( BoundingBox[4].x, BoundingBox[5].x ),
								  min( BoundingBox[6].x, BoundingBox[7].x ) ) ) / 2.0 + 0.5;
	BoundingRect[0].y = min( min( min( BoundingBox[0].y, BoundingBox[1].y ),
								  min( BoundingBox[2].y, BoundingBox[3].y ) ),
							 min( min( BoundingBox[4].y, BoundingBox[5].y ),
								  min( BoundingBox[6].y, BoundingBox[7].y ) ) ) / 2.0 + 0.5;
	BoundingRect[1].x = max( max( max( BoundingBox[0].x, BoundingBox[1].x ),
								  max( BoundingBox[2].x, BoundingBox[3].x ) ),
							 max( max( BoundingBox[4].x, BoundingBox[5].x ),
								  max( BoundingBox[6].x, BoundingBox[7].x ) ) ) / 2.0 + 0.5;
	BoundingRect[1].y = max( max( max( BoundingBox[0].y, BoundingBox[1].y ),
								  max( BoundingBox[2].y, BoundingBox[3].y ) ),
							 max( max( BoundingBox[4].y, BoundingBox[5].y ),
								  max( BoundingBox[6].y, BoundingBox[7].y ) ) ) / 2.0 + 0.5;

	// BoundingBox에서 화면과 가장 가까이 있는 z 값을 가져옴
	float InstanceDepth = min( min( min( BoundingBox[0].z, BoundingBox[1].z ),
									min( BoundingBox[2].z, BoundingBox[3].z ) ),
							   min( min( BoundingBox[4].z, BoundingBox[5].z ),
									min( BoundingBox[6].z, BoundingBox[7].z ) ) );

	// 뷰포트 크기 기준으로 바운딩 사각형의 크기를 구함
	float ViewSizeX = (BoundingRect[1].x-BoundingRect[0].x) * Transform.Viewport.z;
	float ViewSizeY = (BoundingRect[1].y-BoundingRect[0].y) * Transform.Viewport.w;
	
	// Depth Buffer Texture의 LOD(mip-level)을 계산.
	float LOD = ceil( log2( max( ViewSizeX, ViewSizeY ) / 2.0 ) );
	
	// 바운드 사각형의 NDC 좌표를 위에서 얻어진 Hi-z 의 LOD에 조회함.
	// 바운드 사각형의 4 모서리에 대해서 전부 조회하고, 그중에 화면과 가장 먼값을 가져옴.
	vec4 Samples;
	Samples.x = textureLod( HiZBuffer, vec2(BoundingRect[0].x, BoundingRect[0].y), LOD ).x;
	Samples.y = textureLod( HiZBuffer, vec2(BoundingRect[0].x, BoundingRect[1].y), LOD ).x;
	Samples.z = textureLod( HiZBuffer, vec2(BoundingRect[1].x, BoundingRect[1].y), LOD ).x;
	Samples.w = textureLod( HiZBuffer, vec2(BoundingRect[1].x, BoundingRect[0].y), LOD ).x;
	float MaxDepth = max( max( Samples.x, Samples.y ), max( Samples.z, Samples.w ) );
	
	// Hi-z 에 저장된 깊이 값중 가장 큰값보다 현재 바운드 사각형의 깊이 값이 더 크다면, discard 되야함.
	return ( InstanceDepth > MaxDepth ) ? 0 : 1;
}

out int Visible;

void main()
{
	Visible = HiZOcclusionCull();
}