#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"

class jObject;

struct jTriangle
{
	int32 TriangleIndex = 0;
	int32 VertexIndex0 = 0;
	int32 VertexIndex1 = 0;
	int32 VertexIndex2 = 0;
	Vector Normal;
	Vector CenterPos;
	size_t EdgeKey0 = 0;
	size_t EdgeKey1 = 0;
	size_t EdgeKey2 = 0;

	Vector TransformedNormal;
	Vector TransformedCenterPos;
};

struct jEdge
{
	int32 VertexIndex0 = 0;
	int32 VertexIndex1 = 0;
	int32 TriangleIndex = 0;
	int32 TriangleIndex2 = 0;
	int32 IndexInTriangle = 0;

	FORCEINLINE bool ContainVertexIndex(int32 vertexIndex) const
	{
		return (VertexIndex0 == vertexIndex) || (VertexIndex1 == vertexIndex);
	}
};

class jVertexAdjacency
{
public:
	static jVertexAdjacency* GenerateVertexAdjacencyInfo(const std::vector<float>& vertices, const std::vector<uint32>& faces);
	void Update(jObject* ownerObject);

	FORCEINLINE const std::vector<uint32>& GetTriangleAdjacencyIndices() const { return TriangleAdjacencyIndces; }
	void CreateTriangleAdjacencyIndices(std::vector<uint32>& OutResult) const;

	FORCEINLINE const std::vector<Vector>& GetTriangleAdjacencyVertices() const { return Vertices; }

	std::vector<uint32> TriangleAdjacencyIndces;

private:
	friend class jShadowVolumeCPU;

	size_t MakeEdgeKey(int32 vertexIndex0, int32 vertexIndex1);
	jEdge* FindEdge(int32 vertexIndex0, int32 vertexIndex1);
	jEdge* AddEdge(int32 vertexIndex0, int32 vertexIndex1, int32 triangleIndex, int32 indexInTriangle);
	void CreateAdjacencyEdgeInfo(const std::vector<float>& vertices, int32 triangleIndex, int32 vertexIndex0, int32 vertexIndex1, int32 vertexIndex2);
	int32 AddVertex(const std::vector<float>& vertices, int32 vertexIndex);
	void UpdatedTransformedAdjacencyInfo(const Matrix& world, bool generateTransformedInfo = false);
	
	bool isInitialized = false;
	std::map<int32, jTriangle*> Triangles;
	std::map<size_t, jEdge*> Edges;
	std::vector<Vector> Vertices;
	std::vector<float> ResultVertices;
};
