#pragma once
#include "jObject.h"
#include "Math\Vector.h"

struct jTexture;

struct jMeshMaterial
{
	struct LightData
	{
		Vector4 Diffuse = Vector4::OneVector;
		Vector4 Specular = Vector4::OneVector;
		Vector Emissive = Vector::OneVector;
		float SpecularPow = 0.0f;
	};

	LightData Data;
	jTexture* Texture = nullptr;
	std::string TextureName;
};

struct jMeshData
{
	std::vector<Vector> Vertices;
	std::vector<Vector> Normals;
	std::vector<Vector2> TexCoord;
	std::map<int32, jMeshMaterial*> Materials;
	std::vector<uint32> Faces;
};

struct jSubMesh
{
	int32 StartVertex = -1;
	int32 EndVertex = -1;
	int32 StartFace = -1;
	int32 EndFace = -1;
	int32 MaterialIndex = -1;
};

struct jMeshNode
{
	int32 SubMeshIndex = -1;
	jMeshNode* Parent = nullptr;

	std::vector<int32> MeshIndex;
	std::vector<jMeshNode*> childNode;
};

class jMeshObject : public jObject
{
public:
	static jMeshMaterial NullMeshMateral;

	jMeshObject();
	
	jMeshData* MeshData = nullptr;
	std::vector<jSubMesh> SubMeshes;
	jMeshNode* RootNode = nullptr;

	std::function<void(jMeshObject*)> SetMaterialOverride;

	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0 ) override;

	void SetMaterialUniform(const jShader* shader, const jMeshMaterial* material);

	void DrawNode(const jMeshNode* node, const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights);
	void DrawSubMesh(int32 meshIndex, const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights);
};

class jHairObject : public jObject
{
public:

};
