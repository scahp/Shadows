#include "pch.h"
#include "jModelLoader.h"

#include "assimp\Importer.hpp"
#include "assimp\cimport.h"
#include "assimp\postprocess.h"
#include "assimp\scene.h"
#include "jMeshObject.h"
#include "Math\Vector.h"
#include "jRenderObject.h"
#include "jImageFileLoader.h"
#include "jRHI.h"

#if defined _DEBUG
#pragma comment(lib, "assimp-vc141-mtd.lib")
#else
#pragma comment(lib, "assimp-vc141-mt.lib")
#endif

jModelLoader* jModelLoader::_instance = nullptr;

jModelLoader::jModelLoader()
{
}


jModelLoader::~jModelLoader()
{
}

jMeshObject* jModelLoader::LoadFromFile(const char* filename)
{
	Assimp::Importer importer;
	const aiScene *scene = importer.ReadFile(filename, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		JASSERT(!importer.GetErrorString());
		return nullptr;
	}

	jMeshObject* object = new jMeshObject();

	jMeshData* meshData = new jMeshData();
	object->MeshData = meshData;

	for (uint32 i = 0; i < scene->mNumMeshes; ++i)
	{
		const aiMesh* assimpMesh = scene->mMeshes[i];

		jSubMesh subMesh;

		subMesh.StartVertex = static_cast<int32>(meshData->Vertices.size());
		meshData->Vertices.resize(meshData->Vertices.size() + assimpMesh->mNumVertices);
		subMesh.EndVertex = static_cast<int32>(meshData->Vertices.size());

		memcpy(&meshData->Vertices[subMesh.StartVertex], &assimpMesh->mVertices[0], assimpMesh->mNumVertices * sizeof(Vector));

		if (assimpMesh->HasNormals())
		{
			meshData->Normals.resize(meshData->Normals.size() + assimpMesh->mNumVertices);
			memcpy(&meshData->Normals[subMesh.StartVertex], &assimpMesh->mNormals[0], assimpMesh->mNumVertices * sizeof(Vector));
		}

		if (assimpMesh->HasTextureCoords(0))
		{
			meshData->TexCoord.resize(meshData->TexCoord.size() + assimpMesh->mNumVertices);
			for (uint32 k = 0; k < assimpMesh->mNumVertices; ++k)
			{
				auto& texCoord = meshData->TexCoord[subMesh.StartVertex + k];
				auto& assimpTexCoord = assimpMesh->mTextureCoords[0][k];
				texCoord.x = assimpTexCoord.x;
				texCoord.y = assimpTexCoord.y;
			}
		}

		subMesh.StartFace = static_cast<int32>(meshData->Faces.size());
		meshData->Faces.resize(meshData->Faces.size() + assimpMesh->mNumFaces * 3);
		subMesh.EndFace = static_cast<int32>(meshData->Faces.size());
		for (unsigned int k = 0; k < assimpMesh->mNumFaces; ++k)
		{
			aiFace& face = assimpMesh->mFaces[k];
			memcpy(&meshData->Faces[subMesh.StartFace + k * 3], &face.mIndices[0], sizeof(uint32) * 3);
		}
		subMesh.MaterialIndex = assimpMesh->mMaterialIndex;
		object->SubMeshes.emplace_back(subMesh);
	}

	for (uint32 i = 0; i < scene->mNumMaterials; ++i)
	{
		aiMaterial* material = scene->mMaterials[i];

		auto newMeshMaterial = new jMeshMaterial();
		if (material->GetTextureCount(aiTextureType_DIFFUSE))
		{
			aiString str;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
			jImageData data;
			jImageFileLoader::GetInstance().LoadTextureFromFile(data, std::string("Image/") + str.C_Str(), true);
			newMeshMaterial->Texture = g_rhi->CreateTextureFromData(&data.ImageData[0], data.Width, data.Height, data.sRGB);
			newMeshMaterial->TextureName = str.C_Str();
		}

		aiColor3D emissive;
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive))
			memcpy(&newMeshMaterial->Data.Emissive, &emissive, sizeof(emissive));

		aiColor4D diffuse;
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse))
			memcpy(&newMeshMaterial->Data.Diffuse, &diffuse, sizeof(diffuse));

		aiColor4D specular;
		if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, specular))
			memcpy(&newMeshMaterial->Data.Specular, &specular, sizeof(specular));

		float specularPow;
		if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, specularPow))
			newMeshMaterial->Data.SpecularPow = specularPow;
		
		meshData->Materials.emplace(std::make_pair(i, newMeshMaterial));
	}

	JASSERT(scene->mRootNode);
	if (scene->mRootNode)
	{
		jMeshNode* rootNode = new jMeshNode();

		rootNode->MeshIndex.resize(scene->mRootNode->mNumMeshes);
		memcpy(&rootNode->MeshIndex[0], scene->mRootNode->mMeshes, scene->mRootNode->mNumMeshes * sizeof(int32));

		auto fillUpMeshNode = [](jMeshNode* node, aiNode* assimpNode)
		{
			node->MeshIndex.resize(assimpNode->mNumMeshes);
			memcpy(&node->MeshIndex[0], assimpNode->mMeshes, assimpNode->mNumMeshes * sizeof(int32));
		};

		std::function<void(jMeshNode*, aiNode*)> fillupChildNode = [&](jMeshNode* node, aiNode* assimpNode)
		{
			for (uint32 i = 0; i < assimpNode->mNumChildren; ++i)
			{
				auto newNode = new jMeshNode();
				fillUpMeshNode(newNode, assimpNode->mChildren[i]);
				fillupChildNode(newNode, assimpNode->mChildren[i]);
				node->childNode.push_back(newNode);
			}
		};

		fillUpMeshNode(rootNode, scene->mRootNode);
		fillupChildNode(rootNode, scene->mRootNode);

		object->RootNode = rootNode;
	}

	const int32 elementCount = static_cast<int32>(meshData->Vertices.size());

	// attribute 추가
	auto vertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());

	{
		auto streamParam = new jStreamParam<float>();
		streamParam->BufferType = EBufferType::STATIC;
		streamParam->ElementTypeSize = sizeof(float);
		streamParam->ElementType = EBufferElementType::FLOAT;
		streamParam->Stride = sizeof(float) * 3;
		streamParam->Name = "Pos";
		streamParam->Data.resize(elementCount * 3);
		memcpy(&streamParam->Data[0], &meshData->Vertices[0], meshData->Vertices.size() * sizeof(Vector));
		vertexStreamData->Params.push_back(streamParam);
	}

	{
		auto streamParam = new jStreamParam<float>();
		streamParam->BufferType = EBufferType::STATIC;
		streamParam->ElementType = EBufferElementType::FLOAT;
		streamParam->ElementTypeSize = sizeof(float);
		streamParam->Stride = sizeof(float) * 3;
		streamParam->Name = "Normal";
		streamParam->Data.resize(elementCount * 3);
		memcpy(&streamParam->Data[0], &meshData->Normals[0], meshData->Normals.size() * sizeof(Vector));
		vertexStreamData->Params.push_back(streamParam);
	}

	{
		auto streamParam = new jStreamParam<float>();
		streamParam->BufferType = EBufferType::STATIC;
		streamParam->ElementType = EBufferElementType::FLOAT;
		streamParam->ElementTypeSize = sizeof(float);
		streamParam->Stride = sizeof(float) * 2;
		streamParam->Name = "TexCoord";
		streamParam->Data.resize(elementCount * 2);
		memcpy(&streamParam->Data[0], &meshData->TexCoord[0], meshData->TexCoord.size() * sizeof(Vector2));
		vertexStreamData->Params.push_back(streamParam);
	}

	vertexStreamData->PrimitiveType = EPrimitiveType::TRIANGLES;
	vertexStreamData->ElementCount = elementCount;

	auto indexStreamData = std::shared_ptr<jIndexStreamData>(new jIndexStreamData());
	indexStreamData->ElementCount = static_cast<int32>(meshData->Faces.size());
	{
		auto streamParam = new jStreamParam<uint32>();
		streamParam->BufferType = EBufferType::STATIC;
		streamParam->ElementType = EBufferElementType::FLOAT;
		streamParam->ElementTypeSize = sizeof(uint32);
		streamParam->Stride = sizeof(uint32) * 3;
		streamParam->Name = "Index";
		streamParam->Data.resize(meshData->Faces.size());
		memcpy(&streamParam->Data[0], &meshData->Faces[0], meshData->Faces.size() * sizeof(uint32));
		indexStreamData->Param = streamParam;
	}

	auto renderObject = new jRenderObject();
	renderObject->CreateRenderObject(vertexStreamData, indexStreamData);
	renderObject->UseMaterial = 1;
	object->RenderObject = renderObject;
	return object;
}
