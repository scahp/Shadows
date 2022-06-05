#include <pch.h>
#include "jVertexAdjacency.h"
#include "jObject.h"
#include "jRenderObject.h"

jVertexAdjacency* jVertexAdjacency::GenerateVertexAdjacencyInfo(const std::vector<float>& vertices, const std::vector<uint32>& faces)
{
	auto vertexAdjacency = new jVertexAdjacency();

	if (faces.size())
	{
		const auto numOfTriangle = (faces.size() / 3);
		for (int32 i = 0; i < numOfTriangle; ++i)
		{
			const int32 index = i * 3;
			const uint32 vertexIndex0 = faces[index + 0];
			const uint32 vertexIndex1 = faces[index + 1];
			const uint32 vertexIndex2 = faces[index + 2];

			vertexAdjacency->CreateAdjacencyEdgeInfo(vertices, i, vertexIndex0, vertexIndex1, vertexIndex2);
		}
	}
	else
	{
		const auto numOfTriangle = vertices.size() / 9;     // x, y, z component is stored seperately
		for (int32 i = 0; i < numOfTriangle; ++i)
		{
			const int32 index = i * 3;
			const int32 vertexIndex0 = index + 0;
			const int32 vertexIndex1 = index + 1;
			const int32 vertexIndex2 = index + 2;

			vertexAdjacency->CreateAdjacencyEdgeInfo(vertices, i, vertexIndex0, vertexIndex1, vertexIndex2);
		}
	}

	vertexAdjacency->ResultVertices.clear();
	for (auto& iter : vertexAdjacency->Edges)
	{
		const auto& v0 = vertexAdjacency->Vertices[iter.second->VertexIndex0];
		const auto& v1 = vertexAdjacency->Vertices[iter.second->VertexIndex1];

		vertexAdjacency->ResultVertices.push_back(v0.x);
		vertexAdjacency->ResultVertices.push_back(v0.y);
		vertexAdjacency->ResultVertices.push_back(v0.z);
		vertexAdjacency->ResultVertices.push_back(v1.x);
		vertexAdjacency->ResultVertices.push_back(v1.y);
		vertexAdjacency->ResultVertices.push_back(v1.z);
	}

	vertexAdjacency->TriangleAdjacencyIndces.clear();
	vertexAdjacency->CreateTriangleAdjacencyIndices(vertexAdjacency->TriangleAdjacencyIndces);

	return vertexAdjacency;
}



size_t jVertexAdjacency::MakeEdgeKey(int32 vertexIndex0, int32 vertexIndex1)
{
	size_t result = 0;
	if (vertexIndex0 > vertexIndex1)
	{
		hash_combine(result, vertexIndex1);
		hash_combine(result, vertexIndex0);
	}
	else
	{
		hash_combine(result, vertexIndex0);
		hash_combine(result, vertexIndex1);
	}

	return result;
}

jEdge* jVertexAdjacency::FindEdge(int32 vertexIndex0, int32 vertexIndex1)
{
	const auto edgeKey = MakeEdgeKey(vertexIndex0, vertexIndex1);
	auto it_find = Edges.find(edgeKey);
	return (Edges.end() == it_find) ? nullptr : it_find->second;
}

jEdge* jVertexAdjacency::AddEdge(int32 vertexIndex0, int32 vertexIndex1, int32 triangleIndex, int32 indexInTriangle)
{
	jEdge* result = nullptr;
	const auto edgeKey = MakeEdgeKey(vertexIndex0, vertexIndex1);
	auto it_find = Edges.find(edgeKey);
	if (Edges.end() == it_find)
	{
		auto it_ret = Edges.insert(std::make_pair(edgeKey, new jEdge({ vertexIndex0, vertexIndex1, triangleIndex, -1, indexInTriangle })));
		if (it_ret.second)
			result = it_ret.first->second;
	}
	else
	{
		result = it_find->second;
		JASSERT(result->TriangleIndex2 == -1);
		result->TriangleIndex2 = triangleIndex;
	}

	return result;
}

void jVertexAdjacency::CreateAdjacencyEdgeInfo(const std::vector<float>& vertices, int32 triangleIndex, int32 vertexIndex0, int32 vertexIndex1, int32 vertexIndex2)
{
	vertexIndex0 = AddVertex(vertices, vertexIndex0);
	vertexIndex1 = AddVertex(vertices, vertexIndex1);
	vertexIndex2 = AddVertex(vertices, vertexIndex2);

	if (vertexIndex0 == vertexIndex1)
	{
		//alert('[triangleIndex:' + triangleIndex + ']' + 'vertexIndex0 == vertexIndex1('+vertexIndex0+')');
		return;
	}
	if (vertexIndex1 == vertexIndex2)
	{
		//alert('[triangleIndex:' + triangleIndex + ']'+ 'vertexIndex1 == vertexIndex2('+vertexIndex1+')');
		return;
	}
	if (vertexIndex0 == vertexIndex2)
	{
		//alert('[triangleIndex:' + triangleIndex + ']' + 'vertexIndex0 == vertexIndex2('+v2Index+')');
		return;
	}

	AddEdge(vertexIndex1, vertexIndex0, triangleIndex, 0);
	AddEdge(vertexIndex2, vertexIndex1, triangleIndex, 1);
	AddEdge(vertexIndex0, vertexIndex2, triangleIndex, 2);

	size_t edgeKey0 = MakeEdgeKey(vertexIndex1, vertexIndex0);
	size_t edgeKey1 = MakeEdgeKey(vertexIndex2, vertexIndex1);
	size_t edgeKey2 = MakeEdgeKey(vertexIndex0, vertexIndex2);

	Vector normal = Vector::CrossProduct(Vertices[vertexIndex1] - Vertices[vertexIndex0], Vertices[vertexIndex2] - Vertices[vertexIndex0]);
	normal = normal.GetNormalize();
	const auto centerPos = (Vertices[vertexIndex0] + Vertices[vertexIndex1] + Vertices[vertexIndex2]) / 3.0f;

	Triangles[triangleIndex] = new jTriangle({ triangleIndex, vertexIndex0, vertexIndex1, vertexIndex2, normal, centerPos, edgeKey0, edgeKey1, edgeKey2 });
}

int32 jVertexAdjacency::AddVertex(const std::vector<float>& vertices, int32 vertexIndex)
{
	Vector vertex(vertices[vertexIndex * 3 + 0], vertices[vertexIndex * 3 + 1], vertices[vertexIndex * 3 + 2]);

	int32 find = -1;
	for (int32 i = 0; i < Vertices.size(); i++)
	{
		if (Vertices[i].IsNearlyEqual(vertex))
		{
			find = i;
			break;
		}
	}

	if (find == -1)
	{
		find = static_cast<int32>(Vertices.size());
		Vertices.push_back(vertex);
	}

	return find;
}

void jVertexAdjacency::UpdatedTransformedAdjacencyInfo(const Matrix& world, bool generateTransformedInfo /*= false*/)
{
	if (generateTransformedInfo)
	{
		std::vector<float> normalVers;
		normalVers.reserve(Triangles.size() * 6);
		for (auto& iter : Triangles)
		{
			auto& triangle = *iter.second;
			const auto transformedNormal = Matrix3(world).Transform(triangle.Normal).GetNormalize();
			const auto transformedCenterPos = world.TransformPoint(triangle.CenterPos);

			triangle.TransformedNormal = transformedNormal;
			triangle.TransformedCenterPos = transformedCenterPos;

			const Vector normalEnd = transformedCenterPos + transformedNormal * 2.0f;

			normalVers.push_back(transformedCenterPos.x);
			normalVers.push_back(transformedCenterPos.y);
			normalVers.push_back(transformedCenterPos.z);
			normalVers.push_back(normalEnd.x);
			normalVers.push_back(normalEnd.y);
			normalVers.push_back(normalEnd.z);
		}

		std::vector<Vector> transformedVerts;
		transformedVerts.reserve(Vertices.size());
		for (int32 i = 0; i < Vertices.size(); ++i)
			transformedVerts.push_back(world.TransformPoint(Vertices[i]));
	}
}

void jVertexAdjacency::Update(jObject* ownerObject)
{
	UpdatedTransformedAdjacencyInfo(ownerObject->RenderObject->World);
}

void jVertexAdjacency::CreateTriangleAdjacencyIndices(std::vector<uint32>& OutResult) const
{
	const auto getVertexIndexOppositeToEdgeFunc = [this](size_t edgeKey, int32 currentTriangleIndex)
	{
		int32 oppositeTriangleIndex = -1;
		auto it_find = Edges.find(edgeKey);
		JASSERT(Edges.end() != it_find);
		auto edge = it_find->second;
		JASSERT(edge);

		if (currentTriangleIndex == edge->TriangleIndex)
			oppositeTriangleIndex = edge->TriangleIndex2;
		else
			oppositeTriangleIndex = edge->TriangleIndex;

		auto it_triangle = Triangles.find(oppositeTriangleIndex);
		//JASSERT(Triangles.end() != it_triangle);
		int32 vertexIndexResult = -1;
		if (Triangles.end() == it_triangle)
		{
			JASSERT(Triangles.size() > currentTriangleIndex);
			auto currentTriangle = Triangles.find(currentTriangleIndex)->second;
			if (!edge->ContainVertexIndex(currentTriangle->VertexIndex0))
				vertexIndexResult = currentTriangle->VertexIndex0;
			else if (!edge->ContainVertexIndex(currentTriangle->VertexIndex1))
				vertexIndexResult = currentTriangle->VertexIndex1;
			else if (!edge->ContainVertexIndex(currentTriangle->VertexIndex2))
				vertexIndexResult = currentTriangle->VertexIndex2;
		}
		else
		{
			auto oppositeTriangle = it_triangle->second;
			JASSERT(oppositeTriangle);

			if (!edge->ContainVertexIndex(oppositeTriangle->VertexIndex0))
				vertexIndexResult = oppositeTriangle->VertexIndex0;
			else if (!edge->ContainVertexIndex(oppositeTriangle->VertexIndex1))
				vertexIndexResult = oppositeTriangle->VertexIndex1;
			else if (!edge->ContainVertexIndex(oppositeTriangle->VertexIndex2))
				vertexIndexResult = oppositeTriangle->VertexIndex2;
		}

		JASSERT(vertexIndexResult != -1);

		return vertexIndexResult;
	};

	for (auto& iter : Triangles)
	{
		const auto& triangle = iter.second;
		JASSERT(triangle);
		OutResult.push_back(triangle->VertexIndex0);
		OutResult.push_back(getVertexIndexOppositeToEdgeFunc(triangle->EdgeKey0, triangle->TriangleIndex));
		OutResult.push_back(triangle->VertexIndex1);
		OutResult.push_back(getVertexIndexOppositeToEdgeFunc(triangle->EdgeKey1, triangle->TriangleIndex));
		OutResult.push_back(triangle->VertexIndex2);
		OutResult.push_back(getVertexIndexOppositeToEdgeFunc(triangle->EdgeKey2, triangle->TriangleIndex));
	}
}
