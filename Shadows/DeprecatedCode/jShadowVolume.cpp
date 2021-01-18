#include "pch.h"
#include "jShadowVolume.h"
#include "jObject.h"
#include "Math/Matrix.h"
#include "jRenderObject.h"
#include "jVertexAdjacency.h"
#include "jRHI.h"
#include "jPrimitiveUtil.h"

//////////////////////////////////////////////////////////////////////////
// jShadowVolumeCPU
void jShadowVolumeCPU::Update(const Vector& lightPosOrDirection, bool isOmniDirectional, jObject* ownerObject)
{
	QuadVertices.clear();
	EdgeVertices.clear();
	Edges.clear();

	const Matrix matWorldInv = ownerObject->RenderObject->World.GetInverse();
	Vector lightDirWorldInv(Vector::ZeroVector);
	Vector lightPosWorldInv(Vector::ZeroVector);
	if (isOmniDirectional)
		lightPosWorldInv = matWorldInv.Transform(lightPosOrDirection);
	else
		lightDirWorldInv = Matrix3(matWorldInv).Transform(lightPosOrDirection);

	const auto getLightDirection = [&lightDirWorldInv, &isOmniDirectional, &lightPosWorldInv](const Vector& pos)
	{
		if (isOmniDirectional)
			return pos - lightPosWorldInv;

		return lightDirWorldInv;

		JMESSAGE("lightDirection or lightPos should be not null");
		return Vector::ZeroVector;
	};

	std::map<int32, bool> backfaceToLightDirInTriangle;
	for (auto& iter : VertexAdjacency->Triangles)
	{
		const auto& key = iter.first;
		const auto triangle = iter.second;
		auto v0 = VertexAdjacency->Vertices[triangle->VertexIndex0];
		auto v1 = VertexAdjacency->Vertices[triangle->VertexIndex1];
		auto v2 = VertexAdjacency->Vertices[triangle->VertexIndex2];

		auto lightDir = getLightDirection(triangle->CenterPos);
		const auto isBackfaceToLight = (triangle->Normal.DotProduct(lightDir) > 0.0);
		backfaceToLightDirInTriangle[triangle->TriangleIndex] = isBackfaceToLight;

		bool needFrontCap = false;
		bool needBackCap = false;
		if (ownerObject->RenderObject->IsTwoSided)
		{
			needFrontCap = true;
			needBackCap = true;
		}
		else
		{
			if (!isBackfaceToLight)
				needFrontCap = true;
			else
				needBackCap = true;
		}

		if (needFrontCap)
		{
			UpdateEdge(triangle->EdgeKey0);
			UpdateEdge(triangle->EdgeKey1);
			UpdateEdge(triangle->EdgeKey2);

			// front cap
			if (isBackfaceToLight)
			{
				auto temp = v2;
				v2 = v1;
				v1 = temp;
			}
			QuadVertices.push_back(v0.x);   QuadVertices.push_back(v0.y);   QuadVertices.push_back(v0.z);    QuadVertices.push_back(1.0);
			QuadVertices.push_back(v1.x);   QuadVertices.push_back(v1.y);   QuadVertices.push_back(v1.z);    QuadVertices.push_back(1.0);
			QuadVertices.push_back(v2.x);   QuadVertices.push_back(v2.y);   QuadVertices.push_back(v2.z);    QuadVertices.push_back(1.0);
		}

		if (needBackCap)
		{
			// back cap
			if (ownerObject->RenderObject->IsTwoSided || !isBackfaceToLight)
			{
				auto temp = v2;
				v2 = v1;
				v1 = temp;
			}

			QuadVertices.push_back(v0.x);   QuadVertices.push_back(v0.y);   QuadVertices.push_back(v0.z);    QuadVertices.push_back(0.0);
			QuadVertices.push_back(v1.x);   QuadVertices.push_back(v1.y);   QuadVertices.push_back(v1.z);    QuadVertices.push_back(0.0);
			QuadVertices.push_back(v2.x);   QuadVertices.push_back(v2.y);   QuadVertices.push_back(v2.z);    QuadVertices.push_back(0.0);
		}
	}

	EdgeVertices.resize(Edges.size() * 6);
	int index = 0;
	for (auto& iter : Edges)
	{
		const auto edge = VertexAdjacency->Edges[iter];
		const auto v0 = VertexAdjacency->Vertices[edge->VertexIndex0];
		const auto v1 = VertexAdjacency->Vertices[edge->VertexIndex1];

		EdgeVertices[index * 6 + 0] = v0.x;
		EdgeVertices[index * 6 + 1] = v0.y;
		EdgeVertices[index * 6 + 2] = v0.z;
		EdgeVertices[index * 6 + 3] = v1.x;
		EdgeVertices[index * 6 + 4] = v1.y;
		EdgeVertices[index * 6 + 5] = v1.z;
		++index;
	}
	/////////////////////////////////////////

	const int32 startIndex = static_cast<int32>(QuadVertices.size());
	index = 0;
	QuadVertices.resize(QuadVertices.size() + Edges.size() * 24);
	// Creation of quad from Edges that was created by previous step
	for (auto& iter : Edges)
	{
		const auto edge = VertexAdjacency->Edges[iter];
		const auto triangle = VertexAdjacency->Triangles[edge->TriangleIndex];
		auto lightDir = getLightDirection(triangle->CenterPos);

		auto v0 = VertexAdjacency->Vertices[edge->VertexIndex0];
		auto v1 = VertexAdjacency->Vertices[edge->VertexIndex1];
		auto v2 = v0;
		auto v3 = v1;

		// quad should face to triangle normal
		const auto isBackfaceToLight = backfaceToLightDirInTriangle[triangle->TriangleIndex];

		if (isBackfaceToLight)
		{
			{
				auto temp = v0;
				v0 = v1;
				v1 = temp;
			}

			{
				auto temp = v2;
				v2 = v3;
				v3 = temp;
			}
		}

		int cnt = startIndex;
		QuadVertices[index * 24 + (cnt++)] = (v0.x);   QuadVertices[index * 24 + (cnt++)] = (v0.y);   QuadVertices[index * 24 + (cnt++)] = (v0.z);    QuadVertices[index * 24 + (cnt++)] = (1.0);
		QuadVertices[index * 24 + (cnt++)] = (v1.x);   QuadVertices[index * 24 + (cnt++)] = (v1.y);   QuadVertices[index * 24 + (cnt++)] = (v1.z);    QuadVertices[index * 24 + (cnt++)] = (1.0);
		QuadVertices[index * 24 + (cnt++)] = (v2.x);   QuadVertices[index * 24 + (cnt++)] = (v2.y);   QuadVertices[index * 24 + (cnt++)] = (v2.z);    QuadVertices[index * 24 + (cnt++)] = (0.0);

		QuadVertices[index * 24 + (cnt++)] = (v2.x);   QuadVertices[index * 24 + (cnt++)] = (v2.y);   QuadVertices[index * 24 + (cnt++)] = (v2.z);    QuadVertices[index * 24 + (cnt++)] = (0.0);
		QuadVertices[index * 24 + (cnt++)] = (v1.x);   QuadVertices[index * 24 + (cnt++)] = (v1.y);   QuadVertices[index * 24 + (cnt++)] = (v1.z);    QuadVertices[index * 24 + (cnt++)] = (1.0);
		QuadVertices[index * 24 + (cnt++)] = (v3.x);   QuadVertices[index * 24 + (cnt++)] = (v3.y);   QuadVertices[index * 24 + (cnt++)] = (v3.z);    QuadVertices[index * 24 + (cnt++)] = (0.0);
		++index;
	}
	/////////////////////////////////////////

	if (IsInitialized)
	{
		UpdateShadowVolumeObject();
	}
	else
	{
		CreateShadowVolumeObject();
		IsInitialized = true;
	}

	if (EdgeObject)
	{
		EdgeObject->RenderObject->Pos = ownerObject->RenderObject->Pos;
		EdgeObject->RenderObject->Rot = ownerObject->RenderObject->Rot;
		EdgeObject->RenderObject->Scale = ownerObject->RenderObject->Scale;
	}

	if (QuadObject)
	{
		QuadObject->RenderObject->Pos = ownerObject->RenderObject->Pos;
		QuadObject->RenderObject->Rot = ownerObject->RenderObject->Rot;
		QuadObject->RenderObject->Scale = ownerObject->RenderObject->Scale;
	}
}

void jShadowVolumeCPU::UpdateEdge(size_t edgeKey)
{
	if (Edges.end() != Edges.find(edgeKey))
		Edges.erase(edgeKey);
	else
		Edges.insert(edgeKey);
}

void jShadowVolumeCPU::CreateShadowVolumeObject()
{
	if (CreateEdgeObject)
	{
		const int32 elementCount = static_cast<int32>(EdgeVertices.size() / 3);

		// attribute 추가
		EdgeVertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());

		{
			auto streamParam = new jStreamParam<float>();
			streamParam->BufferType = EBufferType::STATIC;
			streamParam->ElementTypeSize = sizeof(float);
			streamParam->ElementType = EBufferElementType::FLOAT;
			streamParam->Stride = sizeof(float) * 3;
			streamParam->Name = "Pos";
			streamParam->Data.resize(EdgeVertices.size());
			memcpy(&streamParam->Data[0], &EdgeVertices[0], EdgeVertices.size() * sizeof(float));
			EdgeVertexStreamData->Params.push_back(streamParam);
		}

		{
			auto streamParam = new jStreamParam<float>();
			streamParam->BufferType = EBufferType::STATIC;
			streamParam->ElementType = EBufferElementType::FLOAT;
			streamParam->ElementTypeSize = sizeof(float);
			streamParam->Stride = sizeof(float) * 4;
			streamParam->Name = "Color";
			streamParam->Data = std::move(jPrimitiveUtil::GenerateColor(Vector4(0.0f, 1.0f, 0.0f, 1.0f), elementCount));
			EdgeVertexStreamData->Params.push_back(streamParam);
		}

		EdgeVertexStreamData->PrimitiveType = EPrimitiveType::LINES;
		EdgeVertexStreamData->ElementCount = elementCount;

		auto object = new jObject();
		auto renderObject = new jRenderObject();
		renderObject->CreateRenderObject(EdgeVertexStreamData, nullptr);
		object->RenderObject = renderObject;
		EdgeObject = object;
	}

	if (CreateQuadObject)
	{
		const int32 elementCount = static_cast<int32>(QuadVertices.size() / 4);

		// attribute 추가
		QuadVertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());

		{
			auto streamParam = new jStreamParam<float>();
			streamParam->BufferType = EBufferType::STATIC;
			streamParam->ElementTypeSize = sizeof(float);
			streamParam->ElementType = EBufferElementType::FLOAT;
			streamParam->Stride = sizeof(float) * 4;
			streamParam->Name = "Pos";
			streamParam->Data.resize(QuadVertices.size());
			memcpy(&streamParam->Data[0], &QuadVertices[0], QuadVertices.size() * sizeof(float));
			QuadVertexStreamData->Params.push_back(streamParam);
		}

		{
			auto streamParam = new jStreamParam<float>();
			streamParam->BufferType = EBufferType::STATIC;
			streamParam->ElementType = EBufferElementType::FLOAT;
			streamParam->ElementTypeSize = sizeof(float);
			streamParam->Stride = sizeof(float) * 4;
			streamParam->Name = "Color";
			streamParam->Data = std::move(jPrimitiveUtil::GenerateColor(Vector4(0.0f, 0.0f, 1.0f, 0.5f), elementCount));
			QuadVertexStreamData->Params.push_back(streamParam);
		}

		QuadVertexStreamData->PrimitiveType = EPrimitiveType::TRIANGLES;
		QuadVertexStreamData->ElementCount = elementCount;

		auto object = new jObject();
		auto renderObject = new jRenderObject();
		renderObject->CreateRenderObject(QuadVertexStreamData, nullptr);
		object->RenderObject = renderObject;
		QuadObject = object;
	}
}

void jShadowVolumeCPU::UpdateShadowVolumeObject()
{
	if (CreateEdgeObject)
	{
		const int32 elementCount = static_cast<int32>(EdgeVertices.size() / 3);

		// attribute 추가
		EdgeVertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());

		{
			auto streamParam = new jStreamParam<float>();
			streamParam->BufferType = EBufferType::STATIC;
			streamParam->ElementTypeSize = sizeof(float);
			streamParam->ElementType = EBufferElementType::FLOAT;
			streamParam->Stride = sizeof(float) * 3;
			streamParam->Name = "Pos";
			streamParam->Data.resize(EdgeVertices.size());
			memcpy(&streamParam->Data[0], &EdgeVertices[0], EdgeVertices.size() * sizeof(float));
			EdgeVertexStreamData->Params.push_back(streamParam);
		}

		{
			auto streamParam = new jStreamParam<float>();
			streamParam->BufferType = EBufferType::STATIC;
			streamParam->ElementType = EBufferElementType::FLOAT;
			streamParam->ElementTypeSize = sizeof(float);
			streamParam->Stride = sizeof(float) * 4;
			streamParam->Name = "Color";
			streamParam->Data = std::move(jPrimitiveUtil::GenerateColor(Vector4(0.0f, 1.0f, 0.0f, 1.0f), elementCount));
			EdgeVertexStreamData->Params.push_back(streamParam);
		}

		EdgeVertexStreamData->PrimitiveType = EPrimitiveType::LINES;
		EdgeVertexStreamData->ElementCount = elementCount;

		g_rhi->UpdateVertexBuffer(EdgeObject->RenderObject->VertexBuffer, EdgeVertexStreamData);
	}

	if (CreateQuadObject)
	{
		const int32 elementCount = static_cast<int32>(QuadVertices.size() / 4);

		// attribute 추가
		QuadVertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());

		{
			auto streamParam = new jStreamParam<float>();
			streamParam->BufferType = EBufferType::STATIC;
			streamParam->ElementTypeSize = sizeof(float);
			streamParam->ElementType = EBufferElementType::FLOAT;
			streamParam->Stride = sizeof(float) * 4;
			streamParam->Name = "Pos";
			streamParam->Data.resize(QuadVertices.size());
			memcpy(&streamParam->Data[0], &QuadVertices[0], QuadVertices.size() * sizeof(float));
			QuadVertexStreamData->Params.push_back(streamParam);
		}

		{
			auto streamParam = new jStreamParam<float>();
			streamParam->BufferType = EBufferType::STATIC;
			streamParam->ElementType = EBufferElementType::FLOAT;
			streamParam->ElementTypeSize = sizeof(float);
			streamParam->Stride = sizeof(float) * 4;
			streamParam->Name = "Color";
			streamParam->Data = std::move(jPrimitiveUtil::GenerateColor(Vector4(0.0f, 0.0f, 1.0f, 0.5f), elementCount));
			QuadVertexStreamData->Params.push_back(streamParam);
		}

		QuadVertexStreamData->PrimitiveType = EPrimitiveType::TRIANGLES;
		QuadVertexStreamData->ElementCount = elementCount;

		g_rhi->UpdateVertexBuffer(QuadObject->RenderObject->VertexBuffer, QuadVertexStreamData);
	}
}

void jShadowVolumeCPU::Draw(const jPipelineContext& pipelineContext, const jShader* shader) const
{
	if (EdgeObject)
		EdgeObject->Draw(pipelineContext.Camera, shader, pipelineContext.Lights);
	if (QuadObject)
		QuadObject->Draw(pipelineContext.Camera, shader, pipelineContext.Lights);
}

//////////////////////////////////////////////////////////////////////////
// jShadowVolumeGPU
void jShadowVolumeGPU::CreateShadowVolumeObject()
{
	// #todo release 처리 아직 확실하지 않음.
	if (ShadowVolumeObject)
	{
		delete ShadowVolumeObject;
		ShadowVolumeObject = nullptr;
	}

	const auto& vertices = VertexAdjacency->GetTriangleAdjacencyVertices();
	const auto& indices = VertexAdjacency->GetTriangleAdjacencyIndices();

	//////////////////////////////////////////////////////////////////////////
	auto vertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());
	auto streamParam = new jStreamParam<float>();
	streamParam->BufferType = EBufferType::STATIC;
	streamParam->ElementTypeSize = sizeof(float);
	streamParam->ElementType = EBufferElementType::FLOAT;
	streamParam->Stride = sizeof(float) * 3;
	streamParam->Name = "Pos";
	streamParam->Data.resize(vertices.size() * 3);
	memcpy(&streamParam->Data[0], &vertices[0], vertices.size() * sizeof(Vector));
	vertexStreamData->Params.push_back(streamParam);

	vertexStreamData->ElementCount = static_cast<int32>(vertices.size());
	vertexStreamData->PrimitiveType = EPrimitiveType::TRIANGLES_ADJACENCY;

	auto indexStreamData = std::shared_ptr<jIndexStreamData>(new jIndexStreamData());
	indexStreamData->ElementCount = static_cast<int32>(indices.size());
	{
		auto streamParam = new jStreamParam<uint32>();
		streamParam->BufferType = EBufferType::STATIC;
		streamParam->ElementType = EBufferElementType::FLOAT;
		streamParam->ElementTypeSize = sizeof(uint32);
		streamParam->Stride = sizeof(uint32) * 6;
		streamParam->Name = "Index";
		streamParam->Data.resize(indices.size());
		memcpy(&streamParam->Data[0], &indices[0], indices.size() * sizeof(uint32));
		indexStreamData->Param = streamParam;
	}

	ShadowVolumeObject = new jObject();
	ShadowVolumeObject->RenderObject = new jRenderObject();
	ShadowVolumeObject->RenderObject->CreateRenderObject(vertexStreamData, indexStreamData);
	//////////////////////////////////////////////////////////////////////////
}

void jShadowVolumeGPU::Update(const Vector& lightPosOrDirection, bool isOmniDirectional, jObject* ownerObject)
{
	JASSERT(ownerObject);
	JASSERT(ownerObject->RenderObject);
	JASSERT(ShadowVolumeObject);
	JASSERT(ShadowVolumeObject->RenderObject);
	ShadowVolumeObject->RenderObject->Pos = ownerObject->RenderObject->Pos;
	ShadowVolumeObject->RenderObject->Rot = ownerObject->RenderObject->Rot;
	ShadowVolumeObject->RenderObject->Scale = ownerObject->RenderObject->Scale;
	ShadowVolumeObject->RenderObject->IsTwoSided = ownerObject->RenderObject->IsTwoSided;
}

void jShadowVolumeGPU::Draw(const jPipelineContext& pipelineContext, const jShader* shader) const
{
	if (!ShadowVolumeObject)
		return;

	ShadowVolumeObject->Draw(pipelineContext.Camera, shader, pipelineContext.Lights);
}