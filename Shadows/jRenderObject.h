#pragma once
#include "jRHIType.h"
#include "Math\Vector.h"
#include "Math\Matrix.h"

class jCamera;
class jLight;
struct jVertexBuffer;
struct jIndexBuffer;
struct jShader;
struct jMaterialData;
struct jTexture;
struct jSamplerState;

enum class EShadingModel : int32
{
	BASE = 0,
	HAIR,
};

static constexpr int32 MAX_TEX = 20;

class jRenderObject
{
public:
	jRenderObject();
	~jRenderObject();

	void CreateRenderObject(const std::shared_ptr<jVertexStreamData>& vertexStream, const std::shared_ptr<jIndexStreamData>& indexStream);
	void UpdateVertexStream(const std::shared_ptr<jVertexStreamData>& vertexStream);
	void UpdateVertexStream();

	//void Draw(const jCamera* camera, const jShader* shader, int32 startIndex = -1, int32 count = -1);
	void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 startIndex, int32 count, int32 instanceCount = 1);

	// todo 함수를 줄일까? 아니면 이렇게 쓸까? 고민
	//void Draw(const jCamera* camera, const jShader* shader, int32 startIndex, int32 count, int32 baseVertexIndex);
	void DrawBaseVertexIndex(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 startIndex, int32 count, int32 baseVertexIndex, int32 instanceCount = 1);
	
	void SetRenderProperty(const jShader* shader);
	void SetCameraProperty(const jShader* shader, const jCamera* camera);
	void SetMaterialProperty(const jShader* shader, jMaterialData* materialData);
	void SetLightProperty(const jShader* shader, const jCamera* camera, const std::list<const jLight*>& lights, jMaterialData* materialData);
	//void SetLightProperty(const jShader* shader, const jLight* light, jMaterialData* materialData);
	void SetTextureProperty(const jShader* shader, jMaterialData* materialData);

	std::shared_ptr<jVertexStreamData> VertexStream;
	jVertexBuffer* VertexBuffer = nullptr;

	std::shared_ptr<jIndexStreamData> IndexStream;
	jIndexBuffer* IndexBuffer = nullptr;

	const jTexture* tex_object[MAX_TEX] = {};
	const jSamplerState* samplerState[MAX_TEX] = {};

	jTexture* tex_object_array = nullptr;
	jSamplerState* samplerStateTexArray = nullptr;

	Vector Pos = Vector::ZeroVector;
	Vector Rot = Vector::ZeroVector;
	Vector Scale = Vector::OneVector;


	// todo 정리 필요.
	int UseUniformColor = 0;
	Vector4 Color = Vector4::OneVector;
	int UseMaterial = 0;
	EShadingModel ShadingModel = EShadingModel::BASE;

	Matrix World;

	bool Collided = false;
	bool IsTwoSided = false;
};

