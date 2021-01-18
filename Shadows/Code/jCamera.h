#pragma once

#include "Math\Matrix.h"
#include "Math\Vector.h"
#include "Math\Plane.h"
#include "jLight.h"

class jLight;
class jAmbientLight;

struct jFrustumPlane
{
	bool IsInFrustum(const Vector& pos, float radius) const
	{
		for (const auto& plane : Planes)
		{
			const float r = pos.DotProduct(plane.n) - plane.d + radius;
			if (r < 0.0f)
				return false;
		}
		return true;
	}

	bool IsInFrustumWithDirection(const Vector& pos, const Vector& direction, float radius) const
	{
		for (const auto& plane : Planes)
		{
			const float r = pos.DotProduct(plane.n) - plane.d + radius;
			if (r < 0.0f)
			{
				if (direction.DotProduct(plane.n) <= 0.0)
					return false;
			}
		}
		return true;
	}

	std::array<jPlane, 6> Planes;
};

namespace jCameraUtil
{
	Matrix CreateViewMatrix(const Vector& pos, const Vector& target, const Vector& up);

	Matrix CreatePerspectiveMatrix(float width, float height, float fov, float farDist, float nearDist);
	Matrix CreatePerspectiveMatrixFarAtInfinity(float width, float height, float fov, float nearDist);

	Matrix CreateOrthogonalMatrix(float width, float height, float farDist, float nearDist);
	Matrix CreateOrthogonalMatrix(float left, float right, float top, float bottom, float farDist, float nearDist);
}

class jCamera
{
public:
	static std::map<int32, jCamera*> CameraMap;
	static void AddCamera(int32 id, jCamera* camera)
	{
		CameraMap.insert(std::make_pair(id, camera));
	}
	static jCamera* GetCamera(int32 id)
	{
		auto it_find = CameraMap.find(id);
		return (CameraMap.end() != it_find) ? it_find->second : nullptr;
	}
	static jCamera* CreateCamera(const Vector& pos, const Vector& target, const Vector& up, float fovRad, float nearDist, float farDist, float width, float height, bool isPerspectiveProjection)
	{
		const auto toTarget = (target - pos);
		const auto toUp = (up - pos);
		const auto toRight = toTarget.CrossProduct(toUp);
		
		jCamera* camera = new jCamera();

		camera->Pos = pos;
		camera->Up = toRight.CrossProduct(toTarget).GetNormalize();
		camera->Target = camera->Up.CrossProduct(toRight).GetNormalize();
		camera->Up += camera->Pos;
		camera->Target += camera->Pos;
		
		camera->FOVRad = fovRad;
		camera->Near = nearDist;
		camera->Far = farDist;
		camera->Width = width;
		camera->Height = height;
		camera->IsPerspectiveProjection = isPerspectiveProjection;
		return camera;
	}

	jCamera();
	virtual ~jCamera();

	virtual Matrix CreateView() const;
	virtual Matrix CreateProjection() const;

	virtual void BindCamera(const jShader* shader) const;

	void UpdateCameraFrustum();
	void UpdateCamera();

	Vector GetForwardVector() const
	{
		return (Target - Pos).GetNormalize();
	}

	Vector GetUpVector() const
	{
		return (Up - Pos).GetNormalize();
	}

	Vector GetRightVector() const
	{
		auto toForward = GetForwardVector();
		auto toUp = GetUpVector();
		return toForward.CrossProduct(toUp).GetNormalize();
	}

	void MoveShift(float dist)
	{
		auto toRight = GetRightVector() * dist;
		Pos += toRight;
		Target += toRight;
		Up += toRight;
	}

	void MoveForward(float dist)
	{
		auto toForward = GetForwardVector() * dist;
		Pos += toForward;
		Target += toForward;
		Up += toForward;
	}

	FORCEINLINE void RotateCameraAxis(const Vector& axis, float radian)
	{
		const auto transformMatrix = Matrix::MakeTranslate(Pos) * Matrix::MakeRotate(axis, radian) * Matrix::MakeTranslate(-Pos);
		Pos = transformMatrix.Transform(Pos);
		Target = transformMatrix.Transform(Target);
		Up = transformMatrix.Transform(Up);
	}

	FORCEINLINE void RotateForwardAxis(float radian)
	{
		RotateCameraAxis(GetForwardVector(), radian);
	}

	FORCEINLINE void RotateUpAxis(float radian)
	{
		RotateCameraAxis(GetUpVector(), radian);
	}

	FORCEINLINE void RotateRightAxis(float radian)
	{
		RotateCameraAxis(GetRightVector(), radian);
	}

	FORCEINLINE void RotateXAxis(float radian)
	{
		RotateCameraAxis(Vector(1.0f, 0.0f, 0.0f), radian);
	}

	FORCEINLINE void RotateYAxis(float radian)
	{
		RotateCameraAxis(Vector(0.0f, 1.0f, 0.0f), radian);
	}

	FORCEINLINE void RotateZAxis(float radian)
	{
		RotateCameraAxis(Vector(0.0f, 0.0f, 1.0f), radian);
	}

	bool IsInFrustum(const Vector& pos, float radius)
	{
		for (auto& iter : Frustum.Planes)
		{
			const float d = pos.DotProduct(iter.n) - iter.d + radius;
			if (d < 0.0f)
				return false;
		}
		return true;
	}

	bool IsInFrustumWithDirection(const Vector& pos, const Vector& dir, float radius) const
	{
		for (auto& iter : Frustum.Planes)
		{
			const float d = pos.DotProduct(iter.n) - iter.d + radius;
			if (d < 0.0f)
			{
				if (dir.DotProduct(iter.n) <= 0)
					return false;
			}
		}
		return true;
	}

	void AddLight(jLight* light);
	jLight* GetLight(int32 index) const;
	jLight* GetLight(ELightType type) const;
	void RemoveLight(int32 index);
	void RemoveLight(ELightType type);
	void RemoveLight(jLight* light);

	int32 GetNumOfLight() const;

	Vector Pos;
	Vector Target;
	Vector Up;
	
	Matrix View;
	Matrix Projection;
	bool IsPerspectiveProjection = true;
	bool IsInfinityFar = false;

	// debug object

	float FOVRad = 0.0f;		// Radian
	float Near = 0.0f;
	float Far = 0.0f;

	std::vector<jLight*> LightList;
	jAmbientLight* Ambient = nullptr;
	bool UseAmbient = false;
	jFrustumPlane Frustum;
	float Width = 0.0f;
	float Height = 0.0f;

	// todo 현재 렌더 스테이트를 저장하는 객체로 옮길 예정
	bool IsEnableCullMode = false;
	float PCF_SIZE_DIRECTIONAL = 2.0f;
	float PCF_SIZE_OMNIDIRECTIONAL = 8.0f;
};

class jOrthographicCamera : public jCamera 
{
public:
	static jCamera* CreateCamera(const Vector& pos, const Vector& target, const Vector& up, float minX, float minY, float maxX, float maxY, float farDist, float nearDist)
	{
		const auto toTarget = (target - pos);
		const auto toUp = (up - pos);
		const auto toRight = toTarget.CrossProduct(toUp);

		jOrthographicCamera* camera = new jOrthographicCamera();

		camera->Pos = pos;
		camera->Up = toRight.CrossProduct(toTarget).GetNormalize();
		camera->Target = camera->Up.CrossProduct(toRight).GetNormalize();
		camera->Up += camera->Pos;
		camera->Target += camera->Pos;

		camera->Near = nearDist;
		camera->Far = farDist;
		camera->IsPerspectiveProjection = false;
		
		camera->MinX = minX;
		camera->MinY = minY;
		camera->MaxX = maxX;
		camera->MaxY = maxY;
		return camera;
	}

	virtual Matrix CreateProjection() const;
	
	float GetMinX() const { return MinX; };
	float GetMinY() const { return MinY; };
	float GetMaxX() const { return MaxX; };
	float GetMaxY() const { return MaxY; };

	void SetMinX(float minX) { MinX = minX; }
	void SetMinY(float minY) { MinY = minY; }
	void SetMaxX(float maxX) { MaxX = maxX; }
	void SetMaxY(float maxY) { MaxY = maxY; }

private:
	float MinX = 0.0f;
	float MinY = 0.0f;

	float MaxX = 0.0f;
	float MaxY = 0.0f;
};