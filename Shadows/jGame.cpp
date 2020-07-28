#include "pch.h"
#include "jGame.h"
#include "jRHI_OpenGL.h"
#include "Math\Vector.h"
#include "jCamera.h"
#include "jObject.h"
#include "jLight.h"
#include "jPrimitiveUtil.h"
#include "jRHI.h"
#include "jRenderObject.h"
#include "jShadowVolume.h"
#include "jShadowAppProperties.h"
#include "jImageFileLoader.h"
#include "jHairModelLoader.h"
#include "jMeshObject.h"
#include "jModelLoader.h"
#include "jFile.h"
#include "jRenderTargetPool.h"
#include "glad\glad.h"
#include "jDeferredRenderer.h"
#include "jForwardRenderer.h"
#include "jPipeline.h"
#include "jVertexAdjacency.h"
#include <vector>

jRHI* g_rhi = nullptr;

struct BoundingBox
{
	Vector Min;
	Vector Max;

	Vector GetPoint(int32 index) const
	{
		return Vector((index & 1) ? Min.x : Max.x, (index & 2) ? Min.y : Max.y, (index & 4) ? Min.z : Max.z);
	}
};

struct BoundingSphere
{
	Vector Center;
	float Radius;

	BoundingSphere()
		: Center(Vector::ZeroVector), Radius(0.0f)
	{}

	BoundingSphere(const std::vector<Vector>& points)
	{
		Radius = 0.0f;

		JASSERT(points.size() > 0);
		std::vector<Vector>::const_iterator ptIt = points.begin();

		Radius = 0.f;
		Center = *ptIt++;

		while (ptIt != points.end())
		{
			const Vector& tmp = *ptIt++;
			Vector cVec = tmp - Center;
			float d = cVec.DotProduct(cVec);
			if (d > Radius * Radius)
			{
				d = sqrtf(d);
				float r = 0.5f * (d + Radius);
				float scale = (r - Radius) / d;
				Center = Center + scale * cVec;
				Radius = r;
			}
		}
	}
};

jGame::jGame()
{
	g_rhi = new jRHI_OpenGL();
}

jGame::~jGame()
{
}

void jGame::ProcessInput()
{
	static float speed = 4.6f;

	jShadowAppSettingProperties& Settings = jShadowAppSettingProperties::GetInstance();

	auto CurrentCamera = Settings.PossessMockCamera ? MockCamera : MainCamera;
	JASSERT(CurrentCamera);
	if (!CurrentCamera)
		return;

	// Process Key Event
	if (g_KeyState['a'] || g_KeyState['A']) CurrentCamera->MoveShift(-speed);
	if (g_KeyState['d'] || g_KeyState['D']) CurrentCamera->MoveShift(speed);
	//if (g_KeyState['1']) MainCamera->RotateForwardAxis(-0.1f);
	//if (g_KeyState['2']) MainCamera->RotateForwardAxis(0.1f);
	//if (g_KeyState['3']) MainCamera->RotateUpAxis(-0.1f);
	//if (g_KeyState['4']) MainCamera->RotateUpAxis(0.1f);
	//if (g_KeyState['5']) MainCamera->RotateRightAxis(-0.1f);
	//if (g_KeyState['6']) MainCamera->RotateRightAxis(0.1f);
	if (g_KeyState['w'] || g_KeyState['W']) CurrentCamera->MoveForward(speed);
	if (g_KeyState['s'] || g_KeyState['S']) CurrentCamera->MoveForward(-speed);
	if (g_KeyState['+']) speed = Max(speed + 0.1f, 0.0f);
	if (g_KeyState['-']) speed = Max(speed - 0.1f, 0.0f);

	if (Settings.PossessMockCamera)
	{
		Settings.MockCameraPos = MockCamera->Pos;
		Settings.MockCameraTarget = MockCamera->Target;
	}
}

void jGame::Setup()
{
	jShadowAppSettingProperties& Settings = jShadowAppSettingProperties::GetInstance();

	//////////////////////////////////////////////////////////////////////////
	const Vector mainCameraPos(-156.032f, 160.992f, -502.611f);
	const Vector mainCameraUp(0.0f, 1.0f, 0.0f);
	MainCamera = jCamera::CreateCamera(mainCameraPos, mainCameraPos + Vector::FowardVector, mainCameraPos + mainCameraUp, DegreeToRadian(45.0f), 10.0f, 2000.0f, SCR_WIDTH, SCR_HEIGHT, true);
	jCamera::AddCamera(0, MainCamera);

	// Light creation step
	NormalDirectionalLight = jLight::CreateDirectionalLight(Settings.DirecionalLightDirection
		, Vector4(0.6f)
		, Vector(1.0f), Vector(1.0f), 64);
	CascadeDirectionalLight = jLight::CreateCascadeDirectionalLight(Settings.DirecionalLightDirection
		, Vector4(0.6f)
		, Vector(1.0f), Vector(1.0f), 64);

	DirectionalLight = NormalDirectionalLight;

	//AmbientLight = jLight::CreateAmbientLight(Vector(0.7f, 0.8f, 0.8f), Vector(0.1f));
	AmbientLight = jLight::CreateAmbientLight(Vector(0.2f, 0.5f, 1.0f), Vector(0.05f));		// sky light color

	//PointLight = jLight::CreatePointLight(jShadowAppSettingProperties::GetInstance().PointLightPosition, Vector4(2.0f, 0.7f, 0.7f, 1.0f), 500.0f, Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), 64.0f);
	//SpotLight = jLight::CreateSpotLight(jShadowAppSettingProperties::GetInstance().SpotLightPosition, jShadowAppSettingProperties::GetInstance().SpotLightDirection, Vector4(0.0f, 1.0f, 0.0f, 1.0f), 500.0f, 0.7f, 1.0f, Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), 64.0f);

	DirectionalLightInfo = jPrimitiveUtil::CreateDirectionalLightDebug(Vector(250, 260, 0) * 0.5f, Vector::OneVector * 10.0f, 10.0f, MainCamera, DirectionalLight, "Image/sun.png");
	jObject::AddDebugObject(DirectionalLightInfo);

	DirectionalLightShadowMapUIDebug = jPrimitiveUtil::CreateUIQuad({ SCR_HEIGHT - 300, 0.0f }, { 300, 300 }, DirectionalLight->GetShadowMap());
	jObject::AddUIDebugObject(DirectionalLightShadowMapUIDebug);

	//PointLightInfo = jPrimitiveUtil::CreatePointLightDebug(Vector(10.0f), MainCamera, PointLight, "Image/bulb.png");
	////jObject::AddDebugObject(PointLightInfo);

	//SpotLightInfo = jPrimitiveUtil::CreateSpotLightDebug(Vector(10.0f), MainCamera, SpotLight, "Image/spot.png");
	////jObject::AddDebugObject(SpotLightInfo);

	MainCamera->AddLight(DirectionalLight);
	//MainCamera->AddLight(PointLight);
	//MainCamera->AddLight(SpotLight);
	MainCamera->AddLight(AmbientLight);

	SpawnObjects(ESpawnedType::TestPrimitive);

	ShadowPipelineSetMap.insert(std::make_pair(EShadowMapType::SSM, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_SSM)));
	ShadowPipelineSetMap.insert(std::make_pair(EShadowMapType::PCF, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_SSM_PCF)));
	ShadowPipelineSetMap.insert(std::make_pair(EShadowMapType::PCSS, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_SSM_PCSS)));
	ShadowPipelineSetMap.insert(std::make_pair(EShadowMapType::VSM, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_VSM)));
	ShadowPipelineSetMap.insert(std::make_pair(EShadowMapType::ESM, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_ESM)));
	ShadowPipelineSetMap.insert(std::make_pair(EShadowMapType::EVSM, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_EVSM)));
	ShadowPipelineSetMap.insert(std::make_pair(EShadowMapType::CSM_SSM, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_CSM_SSM)));

	ShadowPoissonSamplePipelineSetMap.insert(std::make_pair(EShadowMapType::SSM, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_SSM)));
	ShadowPoissonSamplePipelineSetMap.insert(std::make_pair(EShadowMapType::PCF, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_SSM_PCF_Poisson)));
	ShadowPoissonSamplePipelineSetMap.insert(std::make_pair(EShadowMapType::PCSS, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_SSM_PCSS_Poisson)));
	ShadowPoissonSamplePipelineSetMap.insert(std::make_pair(EShadowMapType::VSM, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_VSM)));
	ShadowPoissonSamplePipelineSetMap.insert(std::make_pair(EShadowMapType::ESM, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_ESM)));
	ShadowPoissonSamplePipelineSetMap.insert(std::make_pair(EShadowMapType::EVSM, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_EVSM)));
	ShadowPoissonSamplePipelineSetMap.insert(std::make_pair(EShadowMapType::CSM_SSM, CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_CSM_SSM)));

	CurrentShadowMapType = Settings.ShadowMapType;

	//ForwardRenderer = new jForwardRenderer(ShadowPipelineSetMap[CurrentShadowMapType]);
	ShadowVolumePipelineSet = CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_ShadowVolume);

	// todo 정리 필요
	const auto currentShadowPipelineSet = (Settings.ShadowType == EShadowType::ShadowMap)
		? ShadowPipelineSetMap[CurrentShadowMapType] : ShadowVolumePipelineSet;
	ForwardRenderer = new jForwardRenderer(currentShadowPipelineSet);
	ForwardRenderer->Setup();

	DeferredRenderer = new jDeferredRenderer({ ETextureType::TEXTURE_2D, ETextureFormat::RGBA32F, ETextureFormat::RGBA, EFormatType::FLOAT, EDepthBufferType::DEPTH16, SCR_WIDTH, SCR_HEIGHT, 4 });
	DeferredRenderer->Setup();

	//for (int32 i = 0; i < NUM_CASCADES; ++i)
	//{
	//	jObject::AddUIDebugObject(jPrimitiveUtil::CreateUIQuad({ i * 150.0f, 0.0f }, { 150.0f, 150.0f }, DirectionalLight->ShadowMapData->CascadeShadowMapRenderTarget[i]->GetTexture()));
	//}
		
	MockCamera = jCamera::CreateCamera({ 0.0f, 100.0f, -150.0f }, { 0.0f, 100.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }
		, Settings.MockCameraFov, Settings.MockCameraNear, Settings.MockCameraFar, 300.0f, 300.0f, MainCamera->IsPerspectiveProjection);
}

void jGame::SpawnObjects(ESpawnedType spawnType)
{
	if (spawnType != SpawnedType)
	{
		SpawnedType = spawnType;
		switch (SpawnedType)
		{
		case ESpawnedType::Hair:
			SpawnHairObjects();
			break;
		case ESpawnedType::TestPrimitive:
			SpawnTestPrimitives();
			break;
		case ESpawnedType::CubePrimitive:
			SapwnCubePrimitives();
			break;
		}
	}
}

void jGame::RemoveSpawnedObjects()
{
	for (auto& iter : SpawnedObjects)
	{
		JASSERT(iter);
		jObject::RemoveObject(iter);
		delete iter;
	}
	SpawnedObjects.clear();
}

void jGame::Update(float deltaTime)
{
	SCOPE_DEBUG_EVENT(g_rhi, "Game::Update");

	UpdateAppSetting();

	MainCamera->UpdateCamera();

	const int32 numOfLights = MainCamera->GetNumOfLight();
	for (int32 i = 0; i < numOfLights; ++i)
	{
		auto light = MainCamera->GetLight(i);
		JASSERT(light);
		light->Update(deltaTime, MainCamera);
	}
	//////////////////////////////////////////////////////////////////////////

	auto CreateViewMatrix = [](const Vector& pos, const Vector& target, const Vector& up)
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
		InvRot.m[2][0] = zAxis.x;
		InvRot.m[2][1] = zAxis.y;
		InvRot.m[2][2] = zAxis.z;

		auto InvPos = Vector4(-pos.x, -pos.y, -pos.z, 1.0);
		InvRot.m[0][3] = InvRot.GetRow(0).DotProduct(InvPos);
		InvRot.m[1][3] = InvRot.GetRow(1).DotProduct(InvPos);
		InvRot.m[2][3] = InvRot.GetRow(2).DotProduct(InvPos);
		return InvRot;
	};

	auto CreatePerspectiveLH = [](float aspect, float fov, float zn, float zf)
	{
		float h = 1.0f / tanf(fov / 2.0f);
		float w = h / aspect;

		Matrix projMat;

		projMat.m[0][0] = 2 * zn / w, projMat.m[0][1] = 0, projMat.m[0][2] = 0, projMat.m[0][3] = 0,
			projMat.m[1][0] = 0, projMat.m[1][1] = 2 * zn / h, projMat.m[1][2] = 0, projMat.m[1][3] = 0,
			projMat.m[2][0] = 0, projMat.m[2][1] = 0, projMat.m[2][2] = zf / (zf - zn), projMat.m[2][3] = zn * zf / (zn - zf),
			projMat.m[3][0] = 0, projMat.m[3][1] = 0, projMat.m[3][2] = 1, projMat.m[3][3] = 0;

		return projMat;
	};

	jShadowAppSettingProperties& Settings = jShadowAppSettingProperties::GetInstance();

	static jCamera* NDCCamera = jCamera::CreateCamera({ 0.0f, 0.0f, 2.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }
		, DegreeToRadian(45.0f), 1.0f, 100.0f, 300.0f, 300.0f, true);

	{
		MockCamera->Pos = Settings.MockCameraPos;
		MockCamera->Target = Settings.MockCameraTarget;
		MockCamera->FOVRad = Settings.MockCameraFov;
		MockCamera->Near = Settings.MockCameraNear;
		MockCamera->Far = Settings.MockCameraFar;

		Vector MockCameraForward = (MockCamera->Target - MockCamera->Pos).GetNormalize();
		Vector Axis = Vector::UpVector;
		if (fabsf(MockCameraForward.DotProduct(Vector::UpVector)) >= 0.99f)
			Axis = Vector::RightVector;
		Vector MockCameraRight = MockCameraForward.CrossProduct(Axis).GetNormalize();
		MockCamera->Up = MockCameraRight.CrossProduct(MockCameraForward).GetNormalize() + MockCamera->Pos;
		MockCamera->UpdateCamera();
	}

	{
		NDCCamera->Pos = Settings.NDCCameraPos;
		NDCCamera->Target = Settings.NDCCameraTarget;

		Vector NDCCameraForward = (NDCCamera->Target - NDCCamera->Pos).GetNormalize();
		Vector Axis = Vector::UpVector;
		if (fabsf(NDCCameraForward.DotProduct(Vector::UpVector)) >= 0.99f)
			Axis = Vector::RightVector;
		Vector NDCCameraRight = NDCCameraForward.CrossProduct(Axis).GetNormalize();
		NDCCamera->Up = NDCCameraRight.CrossProduct(NDCCameraForward).GetNormalize() + NDCCamera->Pos;
		NDCCamera->UpdateCamera();
	}

	Matrix MockView = MockCamera->View;
	Matrix MockProj = MockCamera->Projection;

	std::list<const jLight*> lights;
	lights.insert(lights.end(), MainCamera->LightList.begin(), MainCamera->LightList.end());

	for (auto iter : jObject::GetStaticObject())
		iter->Update(deltaTime);

	jObject::FlushDirtyState();

	//////////////////////////////////////////////////////////////////////////

	// 0. 라이트 방향은 라이트쪽을 바라보는 방향임.
	Vector LightDir = -DirectionalLight->Data.Direction.GetNormalize();

	IStreamParam* StreamParam = Cube->RenderObject->VertexStream->Params[0];
	jStreamParam<float>* FloatStreamParam = static_cast<jStreamParam<float>*>(StreamParam);
	auto ElementCount = FloatStreamParam->Data.size() / 3;
	std::vector<Vector> Vertices;
	Vertices.resize(ElementCount);
	memcpy(&Vertices[0], FloatStreamParam->GetBufferData(), sizeof(Vector) * ElementCount);

	auto posMatrix = Matrix::MakeTranslate(Cube->RenderObject->Pos);
	auto rotMatrix = Matrix::MakeRotate(Cube->RenderObject->Rot);
	auto scaleMatrix = Matrix::MakeScale(Cube->RenderObject->Scale);

	Matrix World = posMatrix * rotMatrix * scaleMatrix;

	float FloatMinZ(FLT_MAX);
	float FloatMaxZ(-FLT_MAX);
	Matrix ProjectionView = MockView * World;
	for (int32 i = 0; i < Vertices.size(); ++i)
	{
		Vector TransformedVector = ProjectionView.Transform(Vertices[i]);

		FloatMaxZ = Max(-TransformedVector.z, FloatMaxZ);
		FloatMinZ = Min(-TransformedVector.z, FloatMinZ);
	}

	// 1. VirtualCamera의 View, Proj, ViewProj 만듬
	Matrix VCView;
	Matrix VCProj;

	if (Settings.PossessMockCamera || Settings.PossessMockCameraOnlyShadow)
	{
		VCView = MockView;
		VCProj = MockProj;
	}
	else
	{
		VCView = MainCamera->View;
		VCProj = jCameraUtil::CreatePerspectiveMatrix(SCR_WIDTH, SCR_HEIGHT, MainCamera->FOVRad, MainCamera->Far, MainCamera->Near + 200.0f);
	}

	Matrix ProjPP;
	Matrix ViewPP;
	Vector LightPosPP;
	Vector UpVector;
	Vector CubeCenterPP = Vector::ZeroVector;
	float CubeRadiusPP = Vector::OneVector.Length();	// 6. PP 공간의 큐브의 반지름을 구함. (OpenGL은 NDC 공간이 X, Y, Z 모두 길이 2임)
	float FovPP = 0.0f;
	float NearPP = 0.0f;
	float FarPP = 0.0f;
	static std::shared_ptr<jCamera> PPCamera = nullptr;

	auto MakePPInfo = [&](const Matrix& InView, const Matrix& InProj, bool InUpdatePPCamera)
	{
		// 2. Camera 공간의 LightDir 구함
		Vector EyeLightDir = InView.Transform(Vector4(LightDir, 0.0f));

		// 3. Camera 공간의 LightDir을 PP 공간으로 이동시킴
		Vector4 LightPP = InProj.Transform(Vector4(EyeLightDir, 0.0f));

		// 4. 라이트가 Eye의 뒤쪽에 있는지 판단한다.
		bool LightIsBehindOfEye = (LightPP.w < 0.0f);

		static float W_EPSILON = 0.01f;
		bool IsOrthoMatrix = (fabsf(LightPP.w) <= W_EPSILON);

		float WidthPP = 1.0f;
		float HeightPP = 1.0f;
		if (IsOrthoMatrix)
		{
			Vector LightDirPP(LightPP.x, LightPP.y, LightPP.z);

			LightPosPP = CubeCenterPP + 2.0f * CubeRadiusPP * LightDirPP;
			float DistToCenter = LightPosPP.Length();

			NearPP = DistToCenter - CubeRadiusPP;
			FarPP = DistToCenter + CubeRadiusPP;

			ViewPP = jCameraUtil::CreateViewMatrix(LightPosPP, CubeCenterPP, (LightPosPP + UpVector));
			ProjPP = jCameraUtil::CreateOrthogonalMatrix(CubeRadiusPP, CubeRadiusPP, FarPP, NearPP);

			if (InUpdatePPCamera)
			{
				PPCamera = std::shared_ptr<jCamera>(jCamera::CreateCamera(LightPosPP, CubeCenterPP, (LightPosPP + UpVector)
					, FovPP, NearPP, FarPP, CubeRadiusPP * 2, CubeRadiusPP * 2, !IsOrthoMatrix));
			}
		}
		else
		{
			// 5. PP 공간의 LightDir로 변경한 후, LightDir을 LightPos으로 변경함.
			float wRecip = 1.0f / LightPP.w;
			LightPosPP.x = LightPP.x * wRecip;
			LightPosPP.y = LightPP.y * wRecip;
			LightPosPP.z = LightPP.z * wRecip;

			//////////////////////////////////////////////////////////////////////////
			// 7. LightPP위치에서 CubeCenter를 바라보는 벡터와 그 벡터의 거리를 구함.
			Vector LookAtCubePP = (CubeCenterPP - LightPosPP);
			float DistLookAtCubePP = LookAtCubePP.Length();
			LookAtCubePP /= DistLookAtCubePP;

			if (LightIsBehindOfEye)
			{
				BoundingBox BBox;
				BBox.Min = -Vector::OneVector;
				BBox.Max = Vector::OneVector;

				std::vector<Vector> Points;
				for (int i = 0; i < 8; ++i)
					Points.push_back(BBox.GetPoint(i));

				BoundingSphere BSphere(Points);
				Vector ToBSphereDirection = BSphere.Center - LightPosPP;
				float DistToBSphereDirection = ToBSphereDirection.Length();
				ToBSphereDirection = ToBSphereDirection.GetNormalize();

				Vector YAxis = Vector::UpVector;
				if (fabsf(YAxis.DotProduct(ToBSphereDirection)) > 0.99f)
					YAxis = Vector::RightVector;

				Matrix LookAt = jCameraUtil::CreateViewMatrix(LightPosPP, (LightPosPP + ToBSphereDirection), LightPosPP + YAxis);
				NearPP = 1e32f;
				FarPP = 0.0f;

				float maxx = 0.f, maxy = 0.f;
				for (int32 i = 0; i < Points.size(); i++)
				{
					Vector tmp = LookAt.Transform(Points[i]);
					float InvZ = -tmp.z;

					maxx = Max(maxx, fabsf(tmp.x / InvZ));
					maxy = Max(maxy, fabsf(tmp.y / InvZ));
					NearPP = Min(NearPP, InvZ);
					FarPP = Max(FarPP, InvZ);
				}

				float fovx = atanf(maxx);
				float fovy = atanf(maxy);

				NearPP = Max(0.1f, NearPP);
				FarPP = NearPP;
				NearPP = -NearPP;

				FovPP = 2.0f * atanf(BSphere.Radius / DistToBSphereDirection);
				ProjPP = jCameraUtil::CreatePerspectiveMatrix(2.f * tanf(fovx) * NearPP
					, 2.f * tanf(fovy) * NearPP
					, FovPP, FarPP, NearPP);
			}
			else
			{
				// 10. PP의 Cube에 딱 맞는 Proj Mat을 생성함.
				FovPP = 2.0f * atanf(CubeRadiusPP / DistLookAtCubePP);
				float AspectPP = 1.0f;

				static float testBias = 0.0f;
				NearPP = std::max(testBias + 0.1f, DistLookAtCubePP - CubeRadiusPP);
				FarPP = DistLookAtCubePP + 2.0f * CubeRadiusPP;
				//////////////////////////////////////////////////////////////////////////

				ProjPP = jCameraUtil::CreatePerspectiveMatrix(1.0f, 1.0f, FovPP, FarPP, NearPP);
			}

			// 8. Up 벡터를 선택함, LookAtCubePP와 Vector::UpVector가 거의 일치하면 Vector::RightVector를 UpVector로 설정
			UpVector = Vector::UpVector;
			if (fabsf(Vector::UpVector.DotProduct(LookAtCubePP)) > 0.99f)
				UpVector = Vector::RightVector;

			// 9. PP에서의 라이트 위치, PP의 중심, 위에서 구한 Up벡터를 사용해서 PP에서의 ViewMatrix 구함.
			ViewPP = jCameraUtil::CreateViewMatrix(LightPosPP, CubeCenterPP, (LightPosPP + UpVector));

			if (InUpdatePPCamera)
			{
				PPCamera = std::shared_ptr<jCamera>(jCamera::CreateCamera(LightPosPP, CubeCenterPP, (LightPosPP + UpVector)
					, FovPP, NearPP, FarPP, WidthPP, HeightPP, !IsOrthoMatrix));
			}
		}
		if (InUpdatePPCamera)
			PPCamera->UpdateCamera();
	};

	MakePPInfo(MockView, MockProj, true);

	if (Settings.PossessMockCamera || Settings.PossessMockCameraOnlyShadow)
	{
		VCView = MockView;
		VCProj = MockProj;
	}
	else
	{
		VCView = MainCamera->View;
		VCProj = jCameraUtil::CreatePerspectiveMatrix(SCR_WIDTH, SCR_HEIGHT, MainCamera->FOVRad, MainCamera->Far, MainCamera->Near + 200.0f);
		MakePPInfo(VCView, VCProj, false);
	}

	// 11. PSM 용 Matrix 생성
	// ProjPP * ViewPP * VirtualCameraProj(별도로 만든 Proj) * VirtualCameraView(현재 카메라의 View)
	Matrix PSM_Mat = ProjPP * ViewPP * VCProj * VCView;

	std::list<jObject*> ObjectPPs;
auto& appSetting = jShadowAppSettingProperties::GetInstance();
	Cube->RenderObject->Pos = appSetting.CubePos;
	Cube->RenderObject->Scale = appSetting.CubeScale;

	// CubePP 의 위치로 Transform 시킨다.
	{
		CubePP->RenderObject->Pos = Settings.OffsetPP;
		CubePP->RenderObject->Rot = Vector::ZeroVector;
		CubePP->RenderObject->Scale = Settings.ScalePP;

		auto posMatrix = Matrix::MakeTranslate(Cube->RenderObject->Pos);
		auto rotMatrix = Matrix::MakeRotate(Cube->RenderObject->Rot);
		auto scaleMatrix = Matrix::MakeScale(Cube->RenderObject->Scale);
		Matrix CubeWorld = posMatrix * rotMatrix * scaleMatrix;

		auto ProjView = MockCamera->Projection * MockCamera->View * CubeWorld;

		jStreamParam<float>* thisObjectParam = dynamic_cast<jStreamParam<float>*>(CubePP->RenderObject->VertexStream->Params[0]);
		jStreamParam<float>* cubeObjectParam = dynamic_cast<jStreamParam<float>*>(Cube->RenderObject->VertexStream->Params[0]);

		int32 ElementCount = (int32)cubeObjectParam->GetBufferSize() / cubeObjectParam->Stride;
		Vector* pCubeData = (Vector*)cubeObjectParam->GetBufferData();
		Vector* pThisObjectData = (Vector*)thisObjectParam->GetBufferData();
		for (int32 i = 0; i < ElementCount; ++i)
		{
			const Vector& CubePos = *(pCubeData + i);
			Vector& ThisObjectPos = *(pThisObjectData + i);

			ThisObjectPos = ProjView.Transform(CubePos);

			ThisObjectPos.x = -ThisObjectPos.x;				// NDC 좌표계 -> OpenGL 좌표계로 변환, X축이 서로 반전되어있으므로 그 부분을 추가 해줌.
		}
		CubePP->RenderObject->UpdateVertexStream();
		ObjectPPs.push_back(CubePP);
	}

	// SpherePP의 위치로 Transform 시킨다.
	{
		SpherePP->RenderObject->Pos = Settings.OffsetPP;
		SpherePP->RenderObject->Rot = Vector::ZeroVector;
		SpherePP->RenderObject->Scale = Settings.ScalePP;

		auto posMatrix = Matrix::MakeTranslate(Sphere->RenderObject->Pos);
		auto rotMatrix = Matrix::MakeRotate(Sphere->RenderObject->Rot);
		auto scaleMatrix = Matrix::MakeScale(Sphere->RenderObject->Scale);
		Matrix SphereWorld = posMatrix * rotMatrix * scaleMatrix;

		auto ProjView = MockCamera->Projection * MockCamera->View * SphereWorld;

		jStreamParam<float>* thisObjectParam = dynamic_cast<jStreamParam<float>*>(SpherePP->RenderObject->VertexStream->Params[0]);
		jStreamParam<float>* sphereObjectParam = dynamic_cast<jStreamParam<float>*>(Sphere->RenderObject->VertexStream->Params[0]);

		int32 ElementCount = (int32)sphereObjectParam->GetBufferSize() / sphereObjectParam->Stride;
		Vector* pSphereData = (Vector*)sphereObjectParam->GetBufferData();
		Vector* pThisObjectData = (Vector*)thisObjectParam->GetBufferData();
		for (int32 i = 0; i < ElementCount; ++i)
		{
			const Vector& SpherePos = *(pSphereData + i);
			Vector& ThisObjectPos = *(pThisObjectData + i);

			ThisObjectPos = ProjView.Transform(SpherePos);

			ThisObjectPos.x = -ThisObjectPos.x;				// NDC 좌표계 -> OpenGL 좌표계로 변환, X축이 서로 반전되어있으므로 그 부분을 추가 해줌.
		}
		SpherePP->RenderObject->UpdateVertexStream();
		ObjectPPs.push_back(SpherePP);
	}

	// Draw PSM ShadowMap
	if (DirectionalLight->GetShadowMapRenderTarget()->Begin())
	{
		auto ClearColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableClear = true;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = false;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;

		auto Shader = jShader::GetShader("ShadowGen_PSM");
		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}
		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);
		g_rhi->SetShader(Shader);

		SET_UNIFORM_BUFFER_STATIC(Matrix, "PSM", PSM_Mat, Shader);

		const jCamera* ShadowMapCamera = DirectionalLight->ShadowMapData->ShadowMapCamera;
		ShadowMapCamera->BindCamera(Shader);

		for (auto iter : jObject::GetStaticObject())
			iter->Draw(ShadowMapCamera, Shader, lights);

		DirectionalLight->GetShadowMapRenderTarget()->End();
	}

	// Draw MainScene
	{
		auto ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableClear = true;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = false;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;

		auto Shader = jShader::GetShader("PSM");
		//auto Shader = jShader::GetShader("Simple");
		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}
		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);
		g_rhi->SetShader(Shader);

		auto CurrentCamera = Settings.PossessMockCamera ? MockCamera : MainCamera;
		CurrentCamera->BindCamera(Shader);
		jLight::BindLights(lights, Shader);

		SET_UNIFORM_BUFFER_STATIC(Matrix, "PSM", PSM_Mat, Shader);

		for (auto iter : jObject::GetStaticObject())
			iter->Draw(CurrentCamera, Shader, lights);
	}

	//////////////////////////////////////////////////////////////////////////
	// Draw FrustumInfo
	static auto MockCameraFrustumDebug = jPrimitiveUtil::CreateFrustumDebug(MockCamera);
	MockCameraFrustumDebug->TargetCamera = MockCamera;
	MockCameraFrustumDebug->PostPerspective = false;
	MockCameraFrustumDebug->DrawPlane = false;
	MockCameraFrustumDebug->Update(deltaTime);

	static auto MockCameraPPFrustumDebug = jPrimitiveUtil::CreateFrustumDebug(MockCamera);
	MockCameraPPFrustumDebug->TargetCamera = MockCamera;
	MockCameraPPFrustumDebug->PostPerspective = true;
	MockCameraPPFrustumDebug->DrawPlane = false;
	MockCameraPPFrustumDebug->Update(deltaTime);
	MockCameraPPFrustumDebug->Offset = Settings.OffsetPP;
	MockCameraPPFrustumDebug->Scale = Settings.ScalePP;
	ObjectPPs.push_back(MockCameraPPFrustumDebug);

	static auto PPCameraFrustumDebug = jPrimitiveUtil::CreateFrustumDebug(PPCamera.get());
	PPCameraFrustumDebug->TargetCamera = PPCamera.get();
	PPCameraFrustumDebug->PostPerspective = false;
	PPCameraFrustumDebug->DrawPlane = false;
	PPCameraFrustumDebug->Update(deltaTime);
	PPCameraFrustumDebug->Offset = Settings.OffsetPP;
	PPCameraFrustumDebug->Scale = Settings.ScalePP;

	std::vector<jObject*> FrustumList;
	FrustumList.push_back(MockCameraFrustumDebug);
	FrustumList.push_back(MockCameraPPFrustumDebug);
	FrustumList.push_back(PPCameraFrustumDebug);

	if (!Settings.PossessMockCamera)
	{
		auto EnableClear = true;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = false;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;

		auto Shader = jShader::GetShader("Simple");
		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);
		g_rhi->SetShader(Shader);

		MainCamera->BindCamera(Shader);
		jLight::BindLights(lights, Shader);

		for (uint32 i = 0; i < FrustumList.size(); ++i)
			FrustumList[i]->Draw(MainCamera, Shader, lights);

		CubePP->Draw(MainCamera, Shader, lights);
		SpherePP->Draw(MainCamera, Shader, lights);
	}


	static auto TestRTInfo = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::RGBA
	, ETextureFormat::RGBA, EFormatType::BYTE, EDepthBufferType::DEPTH24_STENCIL8, 300, 300, 1 }));
	if (TestRTInfo->Begin())
	{
		auto ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableClear = true;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = false;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;

		auto Shader = jShader::GetShader("Simple");
		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}
		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);
		g_rhi->SetShader(Shader);

		MockCamera->BindCamera(Shader);
		jLight::BindLights(lights, Shader);

		for (auto iter : jObject::GetStaticObject())
			iter->Draw(MockCamera, Shader, lights);

		TestRTInfo->End();
	}

	const Vector2 PreviewSize(150, 150);
	static auto PreviewUI = jPrimitiveUtil::CreateUIQuad(Vector2(SCR_WIDTH - PreviewSize.x, SCR_HEIGHT - PreviewSize.y), PreviewSize, nullptr);
#define PREVIEW_TEXTURE(TEXTURE)\
	{\
		auto EnableClear = false;\
		auto EnableDepthTest = false;\
		auto DepthStencilFunc = EComparisonFunc::LESS;\
		auto EnableBlend = false;\
		auto BlendSrc = EBlendSrc::ONE;\
		auto BlendDest = EBlendDest::ZERO;\
		auto Shader = jShader::GetShader("UIShader");\
		g_rhi->EnableDepthTest(false);\
		g_rhi->EnableBlend(EnableBlend);\
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);\
		g_rhi->SetShader(Shader);\
		MainCamera->BindCamera(Shader);\
		PreviewUI->RenderObject->tex_object = TEXTURE;\
		PreviewUI->Draw(MainCamera, Shader, {});\
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	if (DirectionalLight->GetShadowMapRenderTarget()->GetTexture())
	{
		PreviewUI->Pos = Vector2(SCR_WIDTH - PreviewSize.x, SCR_HEIGHT - PreviewSize.y);
		PREVIEW_TEXTURE(DirectionalLight->GetShadowMapRenderTarget()->GetTexture());
	}

	PreviewUI->Pos = Vector2(SCR_WIDTH - PreviewSize.x * 2, SCR_HEIGHT - PreviewSize.y);
	PREVIEW_TEXTURE(TestRTInfo->GetTexture());
}

void jGame::UpdateAppSetting()
{
	auto& appSetting = jShadowAppSettingProperties::GetInstance();

	appSetting.SpotLightDirection = Matrix::MakeRotateY(0.01f).Transform(appSetting.SpotLightDirection);

	bool changedDirectionalLight = false;
	if (appSetting.ShadowMapType == EShadowMapType::CSM_SSM)
	{
		if (DirectionalLight != CascadeDirectionalLight)
		{
			MainCamera->RemoveLight(DirectionalLight);
			DirectionalLight = CascadeDirectionalLight;
			MainCamera->AddLight(DirectionalLight);
			changedDirectionalLight = true;
		}
	}
	else
	{
		if (DirectionalLight != NormalDirectionalLight)
		{
			MainCamera->RemoveLight(DirectionalLight);
			DirectionalLight = NormalDirectionalLight;
			MainCamera->AddLight(DirectionalLight);
			changedDirectionalLight = true;
		}
	}

	if (changedDirectionalLight)
	{
		const auto shaderMapRenderTarget = DirectionalLight->ShadowMapData->ShadowMapRenderTarget;
		const float aspect = static_cast<float>(shaderMapRenderTarget->Info.Height) / shaderMapRenderTarget->Info.Width;
		DirectionalLightShadowMapUIDebug->SetTexture(DirectionalLight->ShadowMapData->ShadowMapRenderTarget->GetTexture());
		DirectionalLightShadowMapUIDebug->Size.y = DirectionalLightShadowMapUIDebug->Size.x * aspect;
	}

	const bool isChangedShadowType = CurrentShadowType != appSetting.ShadowType;
	const bool isChangedShadowMapType = (CurrentShadowMapType != appSetting.ShadowMapType);
	if (appSetting.ShadowType == EShadowType::ShadowMap)
	{
		if (appSetting.ShadowMapType == EShadowMapType::DeepShadowMap_DirectionalLight)
		{
			//if (DirectionalLight && DirectionalLight->ShadowMapData && DirectionalLight->ShadowMapData->ShadowMapCamera)
			//	DirectionalLight->ShadowMapData->ShadowMapCamera->IsPerspectiveProjection = true;
			Renderer = DeferredRenderer;
			appSetting.ShowPointLightInfo = false;
			appSetting.ShowSpotLightInfo = false;

			SpawnObjects(ESpawnedType::Hair);
		}
		else
		{
			//if (DirectionalLight && DirectionalLight->ShadowMapData && DirectionalLight->ShadowMapData->ShadowMapCamera)
			//	DirectionalLight->ShadowMapData->ShadowMapCamera->IsPerspectiveProjection = false;
			Renderer = ForwardRenderer;

			const bool isChangedPoisson = (UsePoissonSample != appSetting.UsePoissonSample);
			if (isChangedShadowType || isChangedPoisson || isChangedShadowMapType)
			{
				CurrentShadowType = appSetting.ShadowType;
				UsePoissonSample = appSetting.UsePoissonSample;
				CurrentShadowMapType = appSetting.ShadowMapType;
				auto newPipelineSet = UsePoissonSample ? ShadowPoissonSamplePipelineSetMap[CurrentShadowMapType] : ShadowPipelineSetMap[CurrentShadowMapType];
				Renderer->SetChangePipelineSet(newPipelineSet);
			}

			if (appSetting.ShadowMapType == EShadowMapType::CSM_SSM)
				SpawnObjects(ESpawnedType::CubePrimitive);
			else
				SpawnObjects(ESpawnedType::TestPrimitive);
		}
		MainCamera->IsInfinityFar = false;
	}
	else if (appSetting.ShadowType == EShadowType::ShadowVolume)
	{
		Renderer = ForwardRenderer;
		if (DirectionalLight && DirectionalLight->ShadowMapData && DirectionalLight->ShadowMapData->ShadowMapCamera)
			DirectionalLight->ShadowMapData->ShadowMapCamera->IsPerspectiveProjection = false;

		const bool isChangedShadowType = CurrentShadowType != appSetting.ShadowType;
		if (isChangedShadowType)
		{
			CurrentShadowType = appSetting.ShadowType;
			Renderer->SetChangePipelineSet(ShadowVolumePipelineSet);
		}

		SpawnObjects(ESpawnedType::TestPrimitive);
		MainCamera->IsInfinityFar = true;
	}

	if (isChangedShadowType)
		appSetting.SwitchShadowType(jAppSettings::GetInstance().Get("MainPannel"));
	if (isChangedShadowMapType)
		appSetting.SwitchShadowMapType(jAppSettings::GetInstance().Get("MainPannel"));

	static auto s_showDirectionalLightInfo = appSetting.ShowDirectionalLightInfo;
	static auto s_showPointLightInfo = appSetting.ShowPointLightInfo;
	static auto s_showSpotLightInfo = appSetting.ShowSpotLightInfo;

	static auto s_showDirectionalLightOn = appSetting.DirectionalLightOn;
	static auto s_showPointLightOn = appSetting.PointLightOn;
	static auto s_showSpotLightOn = appSetting.SpotLightOn;

	const auto compareFunc = [](bool& LHS, const bool& RHS, auto func)
	{
		if (LHS != RHS)
		{
			LHS = RHS;
			func(LHS);
		}
	};

	compareFunc(s_showDirectionalLightInfo, appSetting.ShowDirectionalLightInfo, [this](const auto& param) {
		if (param)
			jObject::AddDebugObject(DirectionalLightInfo);
		else
			jObject::RemoveDebugObject(DirectionalLightInfo);
		});

	compareFunc(s_showDirectionalLightOn, appSetting.DirectionalLightOn, [this](const auto& param) {
		if (param)
			MainCamera->AddLight(DirectionalLight);
		else
			MainCamera->RemoveLight(DirectionalLight);
		});

	compareFunc(s_showPointLightInfo, appSetting.ShowPointLightInfo, [this](const auto& param) {
		if (param)
			jObject::AddDebugObject(PointLightInfo);
		else
			jObject::RemoveDebugObject(PointLightInfo);
		});

	compareFunc(s_showPointLightOn, appSetting.PointLightOn, [this](const auto& param) {
		if (param)
			MainCamera->AddLight(PointLight);
		else
			MainCamera->RemoveLight(PointLight);
		});

	compareFunc(s_showSpotLightInfo, appSetting.ShowSpotLightInfo, [this](const auto& param) {
		if (param)
			jObject::AddDebugObject(SpotLightInfo);
		else
			jObject::RemoveDebugObject(SpotLightInfo);
		});

	compareFunc(s_showSpotLightOn, appSetting.SpotLightOn, [this](const auto& param) {
		if (param)
			MainCamera->AddLight(SpotLight);
		else
			MainCamera->RemoveLight(SpotLight);
		});

	// todo debug test, should remove this
	if (DirectionalLight)
		DirectionalLight->Data.Direction = appSetting.DirecionalLightDirection;
	if (PointLight)
		PointLight->Data.Position = appSetting.PointLightPosition;
	if (SpotLight)
	{
		SpotLight->Data.Direction = appSetting.SpotLightDirection;
		SpotLight->Data.Position = appSetting.SpotLightPosition;
	}

	if (DirectionalLightShadowMapUIDebug)
		DirectionalLightShadowMapUIDebug->Visible = appSetting.ShowDirectionalLightMap;
}

void jGame::OnMouseMove(int32 xOffset, int32 yOffset)
{
	if (g_MouseState[EMouseButtonType::LEFT])
	{
		jShadowAppSettingProperties& Settings = jShadowAppSettingProperties::GetInstance();

		auto CurrentCamera = Settings.PossessMockCamera ? MockCamera : MainCamera;
		JASSERT(CurrentCamera);
		if (!CurrentCamera)
			return;

		if (abs(xOffset))
			CurrentCamera->RotateYAxis(xOffset * -0.005f);
		if (abs(yOffset))
			CurrentCamera->RotateRightAxis(yOffset * -0.005f);

		if (Settings.PossessMockCamera)
		{
			Settings.MockCameraPos = MockCamera->Pos;
			Settings.MockCameraTarget = MockCamera->Target;
		}
	}
}

void jGame::Teardown()
{
	Renderer->Teardown();
}

void jGame::SpawnHairObjects()
{
	RemoveSpawnedObjects();

	auto hairObject = jHairModelLoader::GetInstance().CreateHairObject("Model/straight.hair");
	//g_HairObjectArray.push_back(hairObject);
	jObject::AddObject(hairObject);
	SpawnedObjects.push_back(hairObject);

	auto headModel = jModelLoader::GetInstance().LoadFromFile("Model/woman.x");
	//g_StaticObjectArray.push_back(headModel);
	jObject::AddObject(headModel);
	SpawnedObjects.push_back(headModel);
}

void jGame::SpawnTestPrimitives()
{
	RemoveSpawnedObjects();

	auto quad = jPrimitiveUtil::CreateQuad(Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), Vector(1000.0f, 1000.0f, 1000.0f), Vector4(0.3f, 0.3f, 0.3f, 1.0f));
	quad->SetPlane(jPlane(Vector(0.0, 1.0, 0.0), -0.1f));
	//quad->SkipShadowMapGen = true;
	quad->SkipUpdateShadowVolume = true;
	jObject::AddObject(quad);
	SpawnedObjects.push_back(quad);

	//auto gizmo = jPrimitiveUtil::CreateGizmo(Vector::ZeroVector, Vector::ZeroVector, Vector::OneVector);
	//gizmo->SkipShadowMapGen = true;
	//jObject::AddObject(gizmo);
	//SpawnedObjects.push_back(gizmo);

	//auto triangle = jPrimitiveUtil::CreateTriangle(Vector(60.0, 100.0, 20.0), Vector::OneVector, Vector(40.0, 40.0, 40.0), Vector4(0.5f, 0.1f, 1.0f, 1.0f));
	//triangle->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	//{
	//	thisObject->RenderObject->Rot.x += 0.05f;
	//};
	//jObject::AddObject(triangle);
	//SpawnedObjects.push_back(triangle);

	Cube = jPrimitiveUtil::CreateCube(Vector(0.0f, 100.0f, 0.0f), Vector::OneVector, Vector(50.0f, 50.0f, 50.0f), Vector4(0.7f, 0.7f, 0.7f, 1.0f));
	Cube->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.z += 0.005f;
	};
	jObject::AddObject(Cube);
	SpawnedObjects.push_back(Cube);

	CubePP = jPrimitiveUtil::CreateCube(Vector(0.0f, 100.0f, 0.0f), Vector::OneVector, Vector(50.0f, 50.0f, 50.0f), Vector4(0.7f, 0.7f, 0.7f, 1.0f));
	//jObject::AddObject(CubePP);
	SpawnedObjects.push_back(CubePP);

	Sphere = jPrimitiveUtil::CreateSphere(Vector(0.0f, 100.0f, 0.0f), 1.0, 16, Vector(30.0f), Vector4(0.8f, 0.0f, 0.0f, 1.0f));
	Sphere->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.z = DegreeToRadian(180.0f);
	};
	jObject::AddObject(Sphere);
	SpawnedObjects.push_back(Sphere);

	SpherePP = jPrimitiveUtil::CreateSphere(Vector(0.0f, 100.0f, 0.0f), 1.0, 16, Vector(30.0f), Vector4(0.8f, 0.0f, 0.0f, 1.0f));
	//jObject::AddObject(SpherePP);
	SpawnedObjects.push_back(SpherePP);

	//auto cube2 = jPrimitiveUtil::CreateCube(Vector(-65.0f, 35.0f, 10.0f), Vector::OneVector, Vector(50.0f, 50.0f, 50.0f), Vector4(0.7f, 0.7f, 0.7f, 1.0f));
	//jObject::AddObject(cube2);
	//SpawnedObjects.push_back(cube2);

	//auto capsule = jPrimitiveUtil::CreateCapsule(Vector(30.0f, 30.0f, -80.0f), 40.0f, 10.0f, 20, Vector(1.0f), Vector4(1.0f, 1.0f, 0.0f, 1.0f));
	//capsule->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	//{
	//	thisObject->RenderObject->Rot.x -= 0.01f;
	//};
	//jObject::AddObject(capsule);
	//SpawnedObjects.push_back(capsule);

	//auto cone = jPrimitiveUtil::CreateCone(Vector(0.0f, 50.0f, 60.0f), 40.0f, 20.0f, 15, Vector::OneVector, Vector4(1.0f, 1.0f, 0.0f, 1.0f));
	//cone->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	//{
	//	thisObject->RenderObject->Rot.y += 0.03f;
	//};
	//jObject::AddObject(cone);
	//SpawnedObjects.push_back(cone);

	//auto cylinder = jPrimitiveUtil::CreateCylinder(Vector(-30.0f, 60.0f, -60.0f), 20.0f, 10.0f, 20, Vector::OneVector, Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	//cylinder->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	//{
	//	thisObject->RenderObject->Rot.x += 0.05f;
	//};
	//jObject::AddObject(cylinder);
	//SpawnedObjects.push_back(cylinder);

	//auto quad2 = jPrimitiveUtil::CreateQuad(Vector(-20.0f, 80.0f, 40.0f), Vector::OneVector, Vector(20.0f, 20.0f, 20.0f), Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	//quad2->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	//{
	//	thisObject->RenderObject->Rot.z += 0.08f;
	//};
	//jObject::AddObject(quad2);
	//SpawnedObjects.push_back(quad2);

	//auto sphere = jPrimitiveUtil::CreateSphere(Vector(65.0f, 35.0f, 10.0f), 1.0, 16, Vector(30.0f), Vector4(0.8f, 0.0f, 0.0f, 1.0f));
	//sphere->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	//{
	//	//thisObject->RenderObject->Rot.z += 0.01f;
	//	thisObject->RenderObject->Rot.z = DegreeToRadian(180.0f);
	//};
	//Sphere = sphere;

	//jObject::AddObject(sphere);
	//SpawnedObjects.push_back(sphere);

	//auto sphere2 = jPrimitiveUtil::CreateSphere(Vector(150.0f, 5.0f, 0.0f), 1.0, 16, Vector(10.0f), Vector4(0.8f, 0.4f, 0.6f, 1.0f));
	//sphere2->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	//{
	//	const float startY = 5.0f;
	//	const float endY = 100;
	//	const float speed = 1.5f;
	//	static bool dir = true;
	//	thisObject->RenderObject->Pos.y += dir ? speed : -speed;
	//	if (thisObject->RenderObject->Pos.y < startY || thisObject->RenderObject->Pos.y > endY)
	//	{
	//		dir = !dir;
	//		thisObject->RenderObject->Pos.y += dir ? speed : -speed;
	//	}
	//};
	//Sphere = sphere2;
	//jObject::AddObject(sphere2);
	//SpawnedObjects.push_back(sphere2);

	//auto billboard = jPrimitiveUtil::CreateBillobardQuad(Vector(0.0f, 60.0f, 80.0f), Vector::OneVector, Vector(20.0f, 20.0f, 20.0f), Vector4(1.0f, 0.0f, 1.0f, 1.0f), MainCamera);
	//jObject::AddObject(billboard);
	//SpawnedObjects.push_back(billboard);
}

void jGame::SpawnGraphTestFunc()
{
	Vector PerspectiveVector[90];
	Vector OrthographicVector[90];
	{
		{
			static jCamera* pCamera = jCamera::CreateCamera(Vector(0.0), Vector(0.0, 0.0, 1.0), Vector(0.0, 1.0, 0.0), DegreeToRadian(90), 10.0, 100.0, 100.0, 100.0, true);
			pCamera->UpdateCamera();
			int cnt = 0;
			auto MV = pCamera->Projection * pCamera->View;
			for (int i = 0; i < 90; ++i)
			{
				PerspectiveVector[cnt++] = MV.Transform(Vector({ 0.0f, 0.0f, 10.0f + static_cast<float>(i) }));
			}

			for (int i = 0; i < _countof(PerspectiveVector); ++i)
				PerspectiveVector[i].z = (PerspectiveVector[i].z + 1.0f) * 0.5f;
		}
		{
			static jCamera* pCamera = jCamera::CreateCamera(Vector(0.0), Vector(0.0, 0.0, 1.0), Vector(0.0, 1.0, 0.0), DegreeToRadian(90), 10.0, 100.0, 100.0, 100.0, false);
			pCamera->UpdateCamera();
			int cnt = 0;
			auto MV = pCamera->Projection * pCamera->View;
			for (int i = 0; i < 90; ++i)
			{
				OrthographicVector[cnt++] = MV.Transform(Vector({ 0.0f, 0.0f, 10.0f + static_cast<float>(i) }));
			}

			for (int i = 0; i < _countof(OrthographicVector); ++i)
				OrthographicVector[i].z = (OrthographicVector[i].z + 1.0f) * 0.5f;
		}
	}
	std::vector<Vector2> graph1;
	std::vector<Vector2> graph2;

	float scale = 100.0f;
	for (int i = 0; i < _countof(PerspectiveVector); ++i)
		graph1.push_back(Vector2((float)i * 2, PerspectiveVector[i].z * scale));
	for (int i = 0; i < _countof(OrthographicVector); ++i)
		graph2.push_back(Vector2((float)i * 2, OrthographicVector[i].z * scale));

	auto graphObj1 = jPrimitiveUtil::CreateGraph2D({ 360, 350 }, { 360, 300 }, graph1);
	jObject::AddUIDebugObject(graphObj1);

	auto graphObj2 = jPrimitiveUtil::CreateGraph2D({ 360, 700 }, { 360, 300 }, graph2);
	jObject::AddUIDebugObject(graphObj2);
}

void jGame::SapwnCubePrimitives()
{
	RemoveSpawnedObjects();

	for (int i = 0; i < 20; ++i)
	{
		float height = 5.0f * i;
		auto cube = jPrimitiveUtil::CreateCube(Vector(-500.0f + i * 50.0f, height / 2.0f, 20.0f), Vector::OneVector, Vector(10.0f, height, 20.0f), Vector4(0.7f, 0.7f, 0.7f, 1.0f));
		jObject::AddObject(cube);
		SpawnedObjects.push_back(cube);
		cube = jPrimitiveUtil::CreateCube(Vector(-500.0f + i * 50.0f, height / 2.0f, 20.0f + i * 20.0f), Vector::OneVector, Vector(10.0f, height, 10.0f), Vector4(0.7f, 0.7f, 0.7f, 1.0f));
		jObject::AddObject(cube);
		SpawnedObjects.push_back(cube);
		cube = jPrimitiveUtil::CreateCube(Vector(-500.0f + i * 50.0f, height / 2.0f, 20.0f - i * 20.0f), Vector::OneVector, Vector(20.0f, height, 10.0f), Vector4(0.7f, 0.7f, 0.7f, 1.0f));
		jObject::AddObject(cube);
		SpawnedObjects.push_back(cube);
	}

	auto quad = jPrimitiveUtil::CreateQuad(Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), Vector(1000.0f, 1000.0f, 1000.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	quad->SetPlane(jPlane(Vector(0.0, 1.0, 0.0), -0.1f));
	jObject::AddObject(quad);
	SpawnedObjects.push_back(quad);
}
