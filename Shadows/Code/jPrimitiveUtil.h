#pragma once
#include "Math/Vector.h"
#include "Math/Plane.h"
#include "jObject.h"

class jCamera;
class jDirectionalLight;
class jPointLight;
class jSpotLight;
struct jTexture;
class jGraph2D;

class jQuadPrimitive : public jObject
{
public:
	void SetPlane(const jPlane& plane);

	jPlane Plane;
};

class jConePrimitive : public jObject
{
public:
	float Height = 0.0f;
	float Radius = 0.0f;
	Vector4 Color;
};

class jCylinderPrimitive : public jObject
{
public:
	float Height = 0.0f;
	float Radius = 0.0f;
	Vector4 Color;
};

class jBillboardQuadPrimitive : public jQuadPrimitive
{
public:
	jCamera* Camera = nullptr;

	virtual void Update(float deltaTime) override;
};

class jUIQuadPrimitive : public jObject
{
public:
	Vector2 Pos;
	Vector2 Size;

	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const override;
	void SetTexture(const jTexture* texture);
	void SetUniformParams(const jShader* shader) const;
	const jTexture* GetTexture() const;
};

class jFullscreenQuadPrimitive : public jObject
{
public:
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const override;
	void SetUniformBuffer(const jShader* shader) const;
	void SetTexture(int index, const jTexture* texture, const jSamplerState* samplerState);
	void SetTexture(const jTexture* texture, const jSamplerState* samplerState);
	void SetTexture2(const jTexture* texture, const jSamplerState* samplerState);
	void SetTexture3(const jTexture* texture, const jSamplerState* samplerState);
};

class jBoundBoxObject : public jObject
{
public:
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const override;
	void SetUniformBuffer(const jShader* shader);

	Vector4 Color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	jBoundBox BoundBox;
	jObject* OwnerObject = nullptr;
};

class jBoundSphereObject : public jObject
{
public:
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const override;
	void SetUniformBuffer(const jShader* shader);

	Vector4 Color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	jBoundSphere BoundSphere;
	jObject* OwnerObject = nullptr;
};

class jSegmentPrimitive : public jObject
{
public:
	Vector GetCurrentEnd() const
	{
		float t = Clamp(Time, 0.0f, 1.0f);
		Vector end = (End - Start);
		return end * t + Start;
	};

	Vector GetDirectionNormalized() const
	{
		return (End - Start).GetNormalize();
	}

	void UpdateSegment();
	void UpdateSegment(const Vector& start, const Vector& end, const Vector4& color, float time = 1.0f);

	Vector Start;
	Vector End;
	Vector4 Color = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	float Time = 0.0f;

	virtual void Update(float deltaTime) override;

};

class jArrowSegmentPrimitive : public jObject
{
public:
	virtual void Update(float deltaTime) override;
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const override;

	void SetPos(const Vector& pos);
	void SetStart(const Vector& start);
	void SetEnd(const Vector& end);
	void SetTime(float time);

	jSegmentPrimitive* SegmentObject = nullptr;
	jConePrimitive* ConeObject = nullptr;
};

class jDirectionalLightPrimitive : public jObject
{
public:
	virtual void Update(float deltaTime) override;
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const override;

	jBillboardQuadPrimitive* BillboardObject = nullptr;
	jArrowSegmentPrimitive* ArrowSegementObject = nullptr;
	Vector Pos = Vector(0.0f);
	jDirectionalLight* Light = nullptr;
};

class jPointLightPrimitive : public jObject
{
public:
	virtual void Update(float deltaTime) override;
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const override;

	jBillboardQuadPrimitive* BillboardObject = nullptr;
	jObject* SphereObject = nullptr;
	jPointLight* Light = nullptr;
};

class jSpotLightPrimitive : public jObject
{
public:
	virtual void Update(float deltaTime) override;
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const override;

	jBillboardQuadPrimitive* BillboardObject = nullptr;
	jConePrimitive* UmbraConeObject = nullptr;
	jConePrimitive* PenumbraConeObject = nullptr;
	jSpotLight* Light = nullptr;
};

class jFrustumPrimitive : public jObject
{
public:
	jFrustumPrimitive() = default;
	jFrustumPrimitive(const jCamera* targetCamera)
		: TargetCamera(targetCamera)
	{}

	virtual void Update(float deltaTime) override;
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const override;

	jSegmentPrimitive* Segments[12] = { };
	jQuadPrimitive* Plane[6] = { };
	const jCamera* TargetCamera = nullptr;
};

class jGraph2D : public jObject
{
public:
	virtual void Update(float deltaTime) override;
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount = 0) const override;

	void SethPos(const Vector2& pos);
	void SetPoints(const std::vector<Vector2>& points);
	void SetGuardLineSize(const Vector2& size) { GuardLineSize = size; }
	void UpdateBuffer();
	int32 GetMaxInstancCount() const { return static_cast<int32>(ResultMatrices.size()); }

	bool DirtyFlag = false;
	Vector2 Pos;
	Vector2 GuardLineSize = Vector2(100.0f, 100.0f);
	std::vector<Vector2> Points;
	std::vector<Vector2> ResultPoints;
	std::vector<Matrix> ResultMatrices;
	jObject* GuardLineObject = nullptr;
};

namespace jPrimitiveUtil
{
	std::vector<float> GenerateColor(const Vector4& color, int32 elementCount);
	jBoundBox GenerateBoundBox(const std::vector<float>& vertices);
	jBoundSphere GenerateBoundSphere(const std::vector<float>& vertices);
	void CreateShadowVolume(const std::vector<float>& vertices, const std::vector<uint32>& faces, jObject* ownerObject);
	void CreateBoundObjects(const std::vector<float>& vertices, jObject* ownerObject);

	jBoundBoxObject* CreateBoundBox(jBoundBox boundBox, jObject* ownerObject, const Vector4& color = Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	jBoundSphereObject* CreateBoundSphere(jBoundSphere boundSphere, jObject* ownerObject, const Vector4& color = Vector4(0.0f, 0.0f, 0.0f, 1.0f));

	jQuadPrimitive* CreateQuad(const Vector& pos, const Vector& size, const Vector& scale, const Vector4& color);
	jObject* CreateGizmo(const Vector& pos, const Vector& rot, const Vector& scale);
	jObject* CreateTriangle(const Vector& pos, const Vector& size, const Vector& scale, const Vector4& color);
	jObject* CreateCube(const Vector& pos, const Vector& size, const Vector& scale, const Vector4& color);
	jObject* CreateCapsule(const Vector& pos, float height, float radius, int32 slice, const Vector& scale, const Vector4& color);
	jConePrimitive* CreateCone(const Vector& pos, float height, float radius, int32 slice, const Vector& scale, const Vector4& color, bool isWireframe = false, bool createBoundInfo = true, bool createShadowVolumeInfo = true);
	jObject* CreateCylinder(const Vector& pos, float height, float radius, int32 slice, const Vector& scale, const Vector4& color);
	jObject* CreateSphere(const Vector& pos, float radius, int32 slice, const Vector& scale, const Vector4& color, bool isWireframe = false, bool createBoundInfo = true, bool createShadowVolumeInfo = true);
	jBillboardQuadPrimitive* CreateBillobardQuad(const Vector& pos, const Vector& size, const Vector& scale, const Vector4& color, jCamera* camera);
	jUIQuadPrimitive* CreateUIQuad(const Vector2& pos, const Vector2& size, jTexture* texture);
	jFullscreenQuadPrimitive* CreateFullscreenQuad(jTexture* texture);
	jSegmentPrimitive* CreateSegment(const Vector& start, const Vector& end, float time, const Vector4& color);
	jArrowSegmentPrimitive* CreateArrowSegment(const Vector& start, const Vector& end, float time, float coneHeight, float coneRadius, const Vector4& color);

	//////////////////////////////////////////////////////////////////////////
	jDirectionalLightPrimitive* CreateDirectionalLightDebug(const Vector& pos, const Vector& scale, float length, jCamera* targetCamera, jDirectionalLight* light, const char* textureFilename);
	jPointLightPrimitive* CreatePointLightDebug(const Vector& scale, jCamera* targetCamera, jPointLight* light, const char* textureFilename);
	jSpotLightPrimitive* CreateSpotLightDebug(const Vector& scale, jCamera* targetCamera, jSpotLight* light, const char* textureFilename);

	//////////////////////////////////////////////////////////////////////////
	jFrustumPrimitive* CreateFrustumDebug(const jCamera* targetCamera);
	jGraph2D* CreateGraph2D(const Vector2& pos, const Vector2& size, const std::vector<Vector2>& points);
}
