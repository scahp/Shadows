#include "pch.h"
#include "jCamera.h"
#include "jLight.h"
#include "jShadowAppProperties.h"

std::map<int32, jCamera*> jCamera::CameraMap;

//////////////////////////////////////////////////////////////////////////
// jCamera
jCamera::jCamera()
{
}

jCamera::~jCamera()
{
}

Matrix jCamera::CreateView() const
{
	return jCameraUtil::CreateViewMatrix(Pos, Target, Up);
}

Matrix jCamera::CreateProjection() const
{
	if (IsPerspectiveProjection)
	{
		if (IsInfinityFar)
			return jCameraUtil::CreatePerspectiveMatrixFarAtInfinity(Width, Height, FOVRad, Near);
		return jCameraUtil::CreatePerspectiveMatrix(Width, Height, FOVRad, Far, Near);
	}

	return jCameraUtil::CreateOrthogonalMatrix(Width, Height, Far, Near);
}

void jCamera::BindCamera(const jShader* shader) const
{
	auto VP = Projection * View;
	g_rhi->SetUniformbuffer("P", Projection, shader);
	g_rhi->SetUniformbuffer("V", View, shader);
	g_rhi->SetUniformbuffer("VP", VP, shader);
	g_rhi->SetUniformbuffer("Eye", Pos, shader);
	g_rhi->SetUniformbuffer("PCF_Size_Directional", PCF_SIZE_DIRECTIONAL, shader);
	g_rhi->SetUniformbuffer("PCF_Size_OmniDirectional", PCF_SIZE_OMNIDIRECTIONAL, shader);
	g_rhi->SetUniformbuffer("ESM_C", 40.0f, shader);
	g_rhi->SetUniformbuffer("PointLightESM_C", 40.0f, shader);
	g_rhi->SetUniformbuffer("SpotLightESM_C", 40.0f, shader);
	g_rhi->SetUniformbuffer("ShadowMapWidth", SM_WIDTH, shader);
	g_rhi->SetUniformbuffer("ShadowMapHeight", SM_HEIGHT, shader);
	g_rhi->SetUniformbuffer("CSMDebugOn", static_cast<int>(jShadowAppSettingProperties::GetInstance().CSMDebugOn), shader);
	g_rhi->SetUniformbuffer("ShadowOn", jShadowAppSettingProperties::GetInstance().ShadowOn ? 1 : 0, shader);
	g_rhi->SetUniformbuffer("DeepShadowAlpha", jShadowAppSettingProperties::GetInstance().DeepShadowAlpha, shader);
}

void jCamera::UpdateCameraFrustum()
{
	auto toTarget = (Target - Pos).GetNormalize();
	const auto length = tanf(FOVRad) * Far;
	auto toRight = GetRightVector() * length;
	auto toUp = GetUpVector() * length;

	auto rightUp = (toTarget * Far + toRight + toUp).GetNormalize();
	auto leftUp = (toTarget * Far - toRight + toUp).GetNormalize();
	auto rightDown = (toTarget * Far + toRight - toUp).GetNormalize();
	auto leftDown = (toTarget * Far - toRight - toUp).GetNormalize();

	auto far_lt = Pos + leftUp * Far;
	auto far_rt = Pos + rightUp * Far;
	auto far_lb = Pos + leftDown * Far;
	auto far_rb = Pos + rightDown * Far;

	auto near_lt = Pos + leftUp * Near;
	auto near_rt = Pos + rightUp * Near;
	auto near_lb = Pos + leftDown * Near;
	auto near_rb = Pos + rightDown * Near;

	Frustum.Planes[0] = jPlane::CreateFrustumFromThreePoints(near_lb, far_lb, near_lt);		// left
	Frustum.Planes[1] = jPlane::CreateFrustumFromThreePoints(near_rt, far_rt, near_rb);		// right
	Frustum.Planes[2] = jPlane::CreateFrustumFromThreePoints(near_lt, far_lt, near_rt);		// top
	Frustum.Planes[3] = jPlane::CreateFrustumFromThreePoints(near_rb, far_rb, near_lb);		// bottom
	Frustum.Planes[4] = jPlane::CreateFrustumFromThreePoints(near_lb, near_lt, near_rb);	// near
	Frustum.Planes[5] = jPlane::CreateFrustumFromThreePoints(far_rb, far_rt, far_lb);		// far

	// debug object update
}

void jCamera::UpdateCamera()
{
	View = CreateView();
	Projection = CreateProjection();
}

void jCamera::AddLight(jLight* light)
{
	if (light)
	{
		LightList.push_back(light);
		if (light->Type == ELightType::AMBIENT)
			Ambient = static_cast<jAmbientLight*>(light);
	}
}

jLight* jCamera::GetLight(int32 index) const
{
	JASSERT(index >= 0 && LightList.size() > index);
	return LightList[index];
}

jLight* jCamera::GetLight(ELightType type) const
{
	for (auto& iter : LightList)
	{
		if (iter->Type == type)
			return iter;
	}
	return nullptr;
}

void jCamera::RemoveLight(int32 index)
{
	LightList.erase(LightList.begin() + index);
}

void jCamera::RemoveLight(ELightType type)
{
	for (auto it = LightList.begin(); it != LightList.end();)
	{
		if ((*it)->Type == type)
		{
			it = LightList.erase(it);
			continue;
		}
		++it;
	}
}

void jCamera::RemoveLight(jLight* light)
{
	for (auto it = LightList.begin(); it != LightList.end();)
	{
		if (*it == light)
		{
			it = LightList.erase(it);
			continue;
		}
		++it;
	}
}

int32 jCamera::GetNumOfLight() const
{
	return static_cast<int32>(LightList.size());
}

//////////////////////////////////////////////////////////////////////////
// jOrthographicCamera
Matrix jOrthographicCamera::CreateProjection() const
{
	return jCameraUtil::CreateOrthogonalMatrix(MinX, MaxX, MaxY, MinY, Far, Near);
}


Matrix jCameraUtil::CreateViewMatrix(const Vector& pos, const Vector& target, const Vector& up)
{
	const auto zAxis = (target - pos).GetNormalize();
	auto yAxis = (up - pos).GetNormalize();
	const auto xAxis = zAxis.CrossProduct(yAxis).GetNormalize();
	yAxis = xAxis.CrossProduct(zAxis).GetNormalize();

	Matrix InvRot{ IdentityType };
	InvRot.m[0][0] = xAxis.x;
	InvRot.m[0][1] = xAxis.y;
	InvRot.m[0][2] = xAxis.z;
	InvRot.m[1][0] = yAxis.x;
	InvRot.m[1][1] = yAxis.y;
	InvRot.m[1][2] = yAxis.z;
	InvRot.m[2][0] = -zAxis.x;
	InvRot.m[2][1] = -zAxis.y;
	InvRot.m[2][2] = -zAxis.z;

	// auto InvPos = Matrix::MakeTranslate(-pos.x, -pos.y, -pos.z);
	// return InvRot * InvPos;

	auto InvPos = Vector4(-pos.x, -pos.y, -pos.z, 1.0);
	InvRot.m[0][3] = InvRot.GetRow(0).DotProduct(InvPos);
	InvRot.m[1][3] = InvRot.GetRow(1).DotProduct(InvPos);
	InvRot.m[2][3] = InvRot.GetRow(2).DotProduct(InvPos);
	return InvRot;
}

Matrix jCameraUtil::CreatePerspectiveMatrix(float width, float height, float fov, float farDist, float nearDist)
{
	const float F = 1.0f / tanf(fov / 2.0f);
	const float farSubNear = (farDist - nearDist);

	Matrix projMat;
	projMat.m[0][0] = F * (height / width); projMat.m[0][1] = 0.0f;      projMat.m[0][2] = 0.0f;									projMat.m[0][3] = 0.0f;
	projMat.m[1][0] = 0.0f;					projMat.m[1][1] = F;         projMat.m[1][2] = 0.0f;									projMat.m[1][3] = 0.0f;
	projMat.m[2][0] = 0.0f;					projMat.m[2][1] = 0.0f;      projMat.m[2][2] = -(farDist + nearDist) / farSubNear;		projMat.m[2][3] = -(2.0f*nearDist*farDist) / farSubNear;
	projMat.m[3][0] = 0.0f;					projMat.m[3][1] = 0.0f;      projMat.m[3][2] = -1.0f;									projMat.m[3][3] = 0.0f;
	return projMat;
}

Matrix jCameraUtil::CreatePerspectiveMatrixFarAtInfinity(float width, float height, float fov, float nearDist)
{
	const float F = 1.0f / tanf(fov / 2.0f);

	Matrix projMat;
	projMat.m[0][0] = F * (height / width); projMat.m[0][1] = 0.0f;      projMat.m[0][2] = 0.0f;                      projMat.m[0][3] = 0.0f;
	projMat.m[1][0] = 0.0f;					projMat.m[1][1] = F;         projMat.m[1][2] = 0.0f;                      projMat.m[1][3] = 0.0f;
	projMat.m[2][0] = 0.0f;					projMat.m[2][1] = 0.0f;      projMat.m[2][2] = -1.0f;                     projMat.m[2][3] = -(2.0f*nearDist);
	projMat.m[3][0] = 0.0f;					projMat.m[3][1] = 0.0f;      projMat.m[3][2] = -1.0f;                     projMat.m[3][3] = 0.0f;
	return projMat;
}

Matrix jCameraUtil::CreateOrthogonalMatrix(float width, float height, float farDist, float nearDist)
{
	const float farSubNear = (farDist - nearDist);
	const float halfWidth = width * 0.5f;
	const float halfHeight = height * 0.5f;

	Matrix projMat;
	projMat.m[0][0] = 1.0f / halfWidth;     projMat.m[0][1] = 0.0f;                  projMat.m[0][2] = 0.0f;                      projMat.m[0][3] = 0.0f;
	projMat.m[1][0] = 0.0f;					projMat.m[1][1] = 1.0f / halfHeight;     projMat.m[1][2] = 0.0f;                      projMat.m[1][3] = 0.0f;
	projMat.m[2][0] = 0.0f;					projMat.m[2][1] = 0.0f;                  projMat.m[2][2] = -2.0f / farSubNear;        projMat.m[2][3] = -(farDist + nearDist) / farSubNear;
	projMat.m[3][0] = 0.0f;					projMat.m[3][1] = 0.0f;                  projMat.m[3][2] = 0.0f;                      projMat.m[3][3] = 1.0f;
	return projMat;
}

Matrix jCameraUtil::CreateOrthogonalMatrix(float left, float right, float top, float bottom, float farDist, float nearDist)
{
	const float fsn = (farDist - nearDist);
	const float rsl = (right - left);
	const float tsb = (top - bottom);

	Matrix projMat;
	projMat.m[0][0] = 2.0f / rsl;		 projMat.m[0][1] = 0.0f;                  projMat.m[0][2] = 0.0f;                   projMat.m[0][3] = -(right + left) / rsl;
	projMat.m[1][0] = 0.0f;              projMat.m[1][1] = 2.0f / tsb;		      projMat.m[1][2] = 0.0f;                   projMat.m[1][3] = -(top + bottom) / tsb;
	projMat.m[2][0] = 0.0f;              projMat.m[2][1] = 0.0f;                  projMat.m[2][2] = -2.0f / fsn;			projMat.m[2][3] = -(farDist + nearDist) / fsn;
	projMat.m[3][0] = 0.0f;              projMat.m[3][1] = 0.0f;                  projMat.m[3][2] = 0.0f;                   projMat.m[3][3] = 1.0f;
	return projMat;
}
