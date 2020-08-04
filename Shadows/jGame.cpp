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

// AABB(Axis-Aligned Bounding Box)
struct BoundingBox
{
	Vector MinPoint = Vector(FLT_MAX, FLT_MAX, FLT_MAX);
	Vector MaxPoint = Vector(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	Vector GetPoint(int32 index) const
	{
		return Vector((index & 1) ? MinPoint.x : MaxPoint.x, (index & 2) ? MinPoint.y : MaxPoint.y, (index & 4) ? MinPoint.z : MaxPoint.z);
	}

	void XFormBoundingBox(const BoundingBox& src, const Matrix& matrix)
	{
		Vector pts[8];
		for (int i = 0; i < 8; i++)
			pts[i] = src.GetPoint(i);

		MinPoint = Vector(3.3e33f, 3.3e33f, 3.3e33f);
		MaxPoint = Vector(-3.3e33f, -3.3e33f, -3.3e33f);

		for (int i = 0; i < 8; i++)
		{
			Vector tmp;
			pts[i] = matrix.Transform(pts[i]);
			Merge(tmp);
		}
	}

	void Merge(const Vector& newPoint)
	{
		MinPoint.x = Min(MinPoint.x, newPoint.x);
		MinPoint.y = Min(MinPoint.y, newPoint.y);
		MinPoint.z = Min(MinPoint.z, newPoint.z);
		MaxPoint.x = Max(MaxPoint.x, newPoint.x);
		MaxPoint.y = Max(MaxPoint.y, newPoint.y);
		MaxPoint.z = Max(MaxPoint.z, newPoint.z);
	}

	bool Intersect(float* hitDist, const Vector& origPt, const Vector& dir) const
	{
		jPlane sides[6] = { jPlane(1, 0, 0,-MinPoint.x), jPlane(-1, 0, 0, MaxPoint.x),
							jPlane(0, 1, 0,-MinPoint.y), jPlane(0,-1, 0, MaxPoint.y),
							jPlane(0, 0, 1,-MinPoint.z), jPlane(0, 0,-1, MaxPoint.z) };

		*hitDist = 0.f;  // safe initial value
		Vector hitPt = origPt;

		bool inside = false;

		for (int i = 0; (i < 6) && !inside; i++)
		{
			float cosTheta = sides[i].DotProductWithNormal(dir);
			float dist = sides[i].DotProductWithPosition(origPt);

			//  if we're nearly intersecting, just punt and call it an intersection
			if (IsNearlyZero(dist)) return true;
			//  skip nearly (&actually) parallel rays
			if (IsNearlyZero(cosTheta)) continue;
			//  only interested in intersections along the ray, not before it.
			*hitDist = -dist / cosTheta;
			if (*hitDist < 0.f) continue;

			hitPt = (*hitDist) * (dir)+(origPt);

			inside = true;

			for (int j = 0; (j < 6) && inside; j++)
			{
				if (j == i)
					continue;

				float d = sides[j].DotProductWithPosition(hitPt);

				inside = ((d + 0.00015) >= 0.f);
			}
		}

		return inside;
	}

	bool IsInside(const Vector& InPoint) const
	{
		return (InPoint.x >= MinPoint.x && InPoint.x <= MaxPoint.x) &&
			(InPoint.y >= MinPoint.y && InPoint.y <= MaxPoint.y) &&
			(InPoint.z >= MinPoint.z && InPoint.z <= MaxPoint.z);
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
	if (IsMovingUnitCubeWindow)
		CurrentCamera = UnitCubeCamera;
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
	MainCamera = jCamera::CreateCamera(mainCameraPos, mainCameraPos + Vector::FowardVector, mainCameraPos + mainCameraUp
		, DegreeToRadian(60.0f), 10.0f, 2000.0f, SCR_WIDTH, SCR_HEIGHT, true);
	MainCamera->IsEnableCullMode = true;
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
	MockCamera->IsEnableCullMode = true;

	float UnitCubeSize = SCR_WIDTH / 4;
	UnitCubeRect = { SCR_WIDTH - UnitCubeSize, SCR_HEIGHT - UnitCubeSize, UnitCubeSize, UnitCubeSize };

	UnitCubeCamera = jCamera::CreateCamera(Vector(0.0f, 0.0f, -500.0f), Vector(0.0f, 0.0f, 0.0f)
		, Vector(0.0f, 0.0f, -400.0f) + Vector::UpVector, MainCamera->FOVRad, MainCamera->Near, MainCamera->Far, UnitCubeRect.W, UnitCubeRect.H, true);
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

	static float YRotSpeed = 0.0f;
	if (g_KeyState['i'] || g_KeyState['I'])
		YRotSpeed = 0.03f;
	else if (g_KeyState['o'] || g_KeyState['O'])
		YRotSpeed = -0.03f;
	else if (g_KeyState['p'] || g_KeyState['P'])
		YRotSpeed = 0.0f;

	jShadowAppSettingProperties& Settings = jShadowAppSettingProperties::GetInstance();
	const auto transformMatrix = Matrix::MakeRotate(Vector::UpVector, YRotSpeed);
	Settings.DirecionalLightDirection = transformMatrix.Transform(Settings.DirecionalLightDirection);

	MainCamera->Near = Settings.MainCameraNear;
	MainCamera->Far = Settings.MainCameraFar;
	MainCamera->UpdateCamera();

	UnitCubeCamera->UpdateCamera();

	const int32 numOfLights = MainCamera->GetNumOfLight();
	for (int32 i = 0; i < numOfLights; ++i)
	{
		auto light = MainCamera->GetLight(i);
		JASSERT(light);
		light->Update(deltaTime, MainCamera);
	}

	//////////////////////////////////////////////////////////////////////////
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

	Matrix MockView = MockCamera->View;
	Matrix MockProj = MockCamera->Projection;

	std::list<const jLight*> lights;
	lights.insert(lights.end(), MainCamera->LightList.begin(), MainCamera->LightList.end());

	for (auto iter : jObject::GetStaticObject())
		iter->Update(deltaTime);
	
	for (auto iter : jObject::GetDebugObject())
		iter->Update(deltaTime);

	jObject::FlushDirtyState();

	//////////////////////////////////////////////////////////////////////////

	// 0. 라이트 방향은 라이트쪽을 바라보는 방향임.
	Vector LightDir = -DirectionalLight->Data.Direction.GetNormalize();

	// 1. ShadowReciver의 AABB 바운드 박스를 생성합니다. 
	BoundingBox ShadowReceiversBoundBox;
	for (auto obj : jObject::GetStaticObject())
	{
		if (obj->SkipShadowMapGen)
			continue;

		IStreamParam* StreamParam = obj->RenderObject->VertexStream->Params[0];
		jStreamParam<float>* FloatStreamParam = static_cast<jStreamParam<float>*>(StreamParam);
		auto ElementCount = FloatStreamParam->Data.size() / 3;
		std::vector<Vector> Vertices;
		Vertices.resize(ElementCount);
		memcpy(&Vertices[0], FloatStreamParam->GetBufferData(), sizeof(Vector) * ElementCount);

		auto posMatrix = Matrix::MakeTranslate(obj->RenderObject->Pos);
		auto rotMatrix = Matrix::MakeRotate(obj->RenderObject->Rot);
		auto scaleMatrix = Matrix::MakeScale(obj->RenderObject->Scale);

		Matrix World = posMatrix * rotMatrix * scaleMatrix;
		Matrix ViewWorld = MainCamera->View * World;

		for (const Vector& Vertex : Vertices)
		{
			Vector TransformedVertex = World.Transform(Vertex);
			ShadowReceiversBoundBox.Merge(TransformedVertex);
		}
	}

	// 2. CameraFit 모드인 경우. MainCamera의 Near을 ShadowReciver의 AABB 바운드 박스에 맞게 Near을 키워줌.
	float MinZ = FLT_MAX;
	float MaxZ = -FLT_MAX;
	if (Settings.CameraFit)
	{
		Vector MainCameraForward = MainCamera->GetForwardVector();
		Vector MainCameraRight = MainCamera->GetRightVector();

		for (int i = 0; i < 8; ++i)
		{
			auto ToBB = (ShadowReceiversBoundBox.GetPoint(i) - MainCamera->Pos);
			float Z = MainCameraForward.DotProduct(ToBB);
			float X = MainCameraRight.DotProduct(ToBB);

			MinZ = Min(Z, MinZ);
			MaxZ = Max(Z, MaxZ);
		}

		if (MainCamera->Near < MinZ)
			MainCamera->Near = MinZ;
	}

	// 3. VirtualCamera의 Projection, View Matrix를 구한다.
	Matrix VCView;
	Matrix VCProj;

	if (Settings.PossessMockCamera)
	{
		VCView = MockView;
		VCProj = MockProj;
	}
	else
	{
		// MainCamera에 Virtual Slide Back 을 반영 함.
		auto PrevUp = MainCamera->GetUpVector();
		Vector Pos = MainCamera->Pos - Settings.VirtualSliderBack * MainCamera->GetForwardVector();
		Vector Target = MainCamera->Target;
		Vector Up = Pos + PrevUp;
		VCView = jCameraUtil::CreateViewMatrix(Pos, Target, Up);
		VCProj = jCameraUtil::CreatePerspectiveMatrix(SCR_WIDTH, SCR_HEIGHT, MainCamera->FOVRad
			, MainCamera->Far, MainCamera->Near + Settings.VirtualSliderBack);
	}

	// 4. PostPerspective 공간에서 사용할 Projection, View Marix를 구한다.
	auto MakePPInfo = [&](std::shared_ptr<jCamera>& OutPPCamera, Matrix& OutProjPP, Matrix& OutViewPP
						, const Matrix& InView, const Matrix& InProj, bool updatePPCamera)
	{
		Vector CubeCenterPP = Vector::ZeroVector;
		float CubeRadiusPP = Vector::OneVector.Length();	// 6. PP 공간의 큐브의 반지름을 구함. (OpenGL은 NDC 공간이 X, Y, Z 모두 길이 2임)
		Vector LightPosPP;
		float FovPP = 0.0f;
		float NearPP = 0.0f;
		float FarPP = 0.0f;

		// Camera 공간의 LightDir 구함
		Vector EyeLightDir = InView.Transform(Vector4(LightDir, 0.0f));

		// Camera 공간의 LightDir을 PP 공간으로 이동시킴
		Vector4 LightPP = InProj.Transform(Vector4(EyeLightDir, 0.0f));

		// 라이트가 Eye의 뒤쪽에 있는지 판단한다.
		bool LightIsBehindOfEye = (LightPP.w < 0.0f);

		// 시야 방향과 라이트가 직교하는 상태인지 확인하고, 직교하면 OrthoMatrix 사용
		static float W_EPSILON = 0.001f;
		bool IsOrthoMatrix = (fabsf(LightPP.w) <= W_EPSILON);

		float WidthPP = 1.0f;
		float HeightPP = 1.0f;
		if (IsOrthoMatrix)
		{
			Vector LightDirPP(LightPP.x, LightPP.y, LightPP.z);

			// NDC Unit Cube를 딱 감쌀 수 있는 View와 Projection Matrix를 생성합니다.
			LightPosPP = CubeCenterPP + 2.0f * CubeRadiusPP * LightDirPP;
			float DistToCenter = LightPosPP.Length();

			NearPP = DistToCenter - CubeRadiusPP;
			FarPP = DistToCenter + CubeRadiusPP;

			Vector UpVector = Vector::UpVector;
			if (fabsf(UpVector.DotProduct((CubeCenterPP - LightPosPP).GetNormalize())) > 0.99f)
				UpVector = Vector::RightVector;

			OutViewPP = jCameraUtil::CreateViewMatrix(LightPosPP, CubeCenterPP, (LightPosPP + UpVector));
			OutProjPP = jCameraUtil::CreateOrthogonalMatrix(CubeRadiusPP * 2, CubeRadiusPP * 2, FarPP, NearPP);

			// PP 공간 디버깅용 카메라 업데이트
			if (updatePPCamera)
			{
				OutPPCamera = std::shared_ptr<jCamera>(jCamera::CreateCamera(LightPosPP, CubeCenterPP, (LightPosPP + UpVector)
					, FovPP, NearPP, FarPP, CubeRadiusPP * 2, CubeRadiusPP * 2, !IsOrthoMatrix));
				OutPPCamera->UpdateCamera();
			}
		}
		else
		{
			// PP 공간의 LightDir로 변경한 후, LightDir을 LightPos으로 변경함.
			float wRecip = 1.0f / LightPP.w;
			LightPosPP.x = LightPP.x * wRecip;
			LightPosPP.y = LightPP.y * wRecip;
			LightPosPP.z = LightPP.z * wRecip;

			// LightPP위치에서 CubeCenter를 바라보는 벡터와 그 벡터의 거리를 구함.
			Vector LookAtCubePP = (CubeCenterPP - LightPosPP);
			float DistLookAtCubePP = LookAtCubePP.Length();
			LookAtCubePP /= DistLookAtCubePP;

			if (LightIsBehindOfEye)
			{
				Vector ToBSphereDirection = CubeCenterPP - LightPosPP;
				const float DistToBSphereDirection = ToBSphereDirection.Length();
				ToBSphereDirection = ToBSphereDirection.GetNormalize();

				NearPP = DistToBSphereDirection - CubeRadiusPP;
				FovPP = 2.0f * atanf(CubeRadiusPP / DistToBSphereDirection);

				// Perspective Matrix의 Near를 마이너스로 두는 트릭을 사용함.
				NearPP = Max(0.1f, NearPP);
				FarPP = NearPP;
				NearPP = -NearPP;

				// PostPerspective 공간에서 사용할 Projection 을 계산
				OutProjPP = jCameraUtil::CreatePerspectiveMatrix(WidthPP, HeightPP, FovPP, FarPP, NearPP);
			}
			else
			{
				// NDC Unit Cube 공간의 바운드박스를 만듬
				FovPP = 2.0f * atanf(CubeRadiusPP / DistLookAtCubePP);
				float AspectPP = 1.0f;

				NearPP = std::max(0.1f, DistLookAtCubePP - CubeRadiusPP);
				FarPP = DistLookAtCubePP + CubeRadiusPP;

				// PostPerspective 공간에서 사용할 Projection 을 계산
				OutProjPP = jCameraUtil::CreatePerspectiveMatrix(1.0f, 1.0f, FovPP, FarPP, NearPP);
			}

			Vector UpVector = Vector::UpVector;
			if (fabsf(Vector::UpVector.DotProduct(LookAtCubePP)) > 0.99f)
				UpVector = Vector::RightVector;

			// PP에서의 라이트 위치, PP의 중심, 위에서 구한 Up벡터를 사용해서 PP에서의 ViewMatrix 구함.
			OutViewPP = jCameraUtil::CreateViewMatrix(LightPosPP, CubeCenterPP, (LightPosPP + UpVector));

			// PP 공간 디버깅용 카메라 업데이트
			if (updatePPCamera)
			{
				OutPPCamera = std::shared_ptr<jCamera>(jCamera::CreateCamera(LightPosPP, CubeCenterPP, (LightPosPP + UpVector)
					, FovPP, NearPP, FarPP, WidthPP, HeightPP, !IsOrthoMatrix));
				OutPPCamera->UpdateCamera();
			}
		}
	};

	Matrix ProjPP;
	Matrix ViewPP;
	std::shared_ptr<jCamera> PPCamera = nullptr;

	if (Settings.PossessMockCamera)
	{
		MakePPInfo(PPCamera, ProjPP, ViewPP, MockView, MockProj, true);
	}
	else
	{
		if (Settings.PossessOnlyMockCameraPP)
		{
			MakePPInfo(PPCamera, ProjPP, ViewPP, MockView, MockProj, true);
			MakePPInfo(PPCamera, ProjPP, ViewPP, VCView, VCProj, false);
		}
		else
		{
			MakePPInfo(PPCamera, ProjPP, ViewPP, VCView, VCProj, true);
		}
	}

	// 5. PSM 용 Matrix 생성
	// ProjPP * ViewPP * VirtualCameraProj(별도로 만든 Proj) * VirtualCameraView(현재 카메라의 View)
	Matrix PSM_Mat = ProjPP * ViewPP * VCProj * VCView;

	// 6. PSM ShadowMap 생성
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
		{
			if (!iter->SkipShadowMapGen)
				iter->Draw(ShadowMapCamera, Shader, lights);
		}

		DirectionalLight->GetShadowMapRenderTarget()->End();
	}

	// 7. 메인장면 렌더링
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
		g_rhi->EnableCullFace(true);
		g_rhi->EnableDepthBias(true);
		g_rhi->SetDepthBias(Settings.ConstBias, Settings.SlopBias);
		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);
		g_rhi->SetShader(Shader);

		auto CurrentCamera = Settings.PossessMockCamera ? MockCamera : MainCamera;
		if (Settings.PossessMockCamera)
		{
			g_rhi->SetViewport((SCR_WIDTH - SCR_HEIGHT) / 2, 0, SCR_HEIGHT, SCR_HEIGHT);
		}
		else
		{
			g_rhi->SetViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
		}


		CurrentCamera->BindCamera(Shader);
		jLight::BindLights(lights, Shader);

		SET_UNIFORM_BUFFER_STATIC(Matrix, "PSM", PSM_Mat, Shader);

		for (auto iter : jObject::GetStaticObject())
			iter->Draw(CurrentCamera, Shader, lights);

		// ShadowReciver AABB를 렌더링 해줄 오브젝트 생성
		static auto ShadowReceiverBoundBoxObject = jPrimitiveUtil::CreateBoundBox(
			{ ShadowReceiversBoundBox.MinPoint, ShadowReceiversBoundBox.MaxPoint }, nullptr, Vector4(0.37f, 0.62f, 0.47f, 1.0f));
		ShadowReceiverBoundBoxObject->Update(deltaTime);
		ShadowReceiverBoundBoxObject->UpdateBoundBox({ ShadowReceiversBoundBox.MinPoint, ShadowReceiversBoundBox.MaxPoint });

		// ShadowReciver AABB 렌더링
		if (Settings.ShowMergedBoundBox)
		{
			Shader = jShader::GetShader("BoundVolumeShader");
			g_rhi->SetShader(Shader);
			CurrentCamera->BindCamera(Shader);
			jLight::BindLights(lights, Shader);
			ShadowReceiverBoundBoxObject->SetUniformBuffer(Shader);
			ShadowReceiverBoundBoxObject->Draw(CurrentCamera, Shader, lights);
		}

		Shader = jShader::GetShader("DebugObjectShader");
		for (auto iter : jObject::GetDebugObject())
			iter->Draw(MainCamera, Shader, lights);
	}

	//////////////////////////////////////////////////////////////////////////
	// 8. PP 공간 시각화 디버깅용 오브젝트 갱신
	static auto UnitCubeRT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::RGBA
		, ETextureFormat::RGBA, EFormatType::BYTE, EDepthBufferType::DEPTH24_STENCIL8, static_cast<int32>(UnitCubeRect.W), static_cast<int32>(UnitCubeRect.H), 1 }));

	std::vector<jObject*> ObjectPPs;
	Cube->RenderObject->Pos = Settings.CubePos;
	Cube->RenderObject->Scale = Settings.CubeScale;

	auto TransformPP = [&](jObject* ObjectPP, const jObject* Source)
	{
		ObjectPP->RenderObject->Pos = Settings.OffsetPP;
		ObjectPP->RenderObject->Rot = Vector::ZeroVector;
		ObjectPP->RenderObject->Scale = Vector(Settings.ScalePP);

		auto posMatrix = Matrix::MakeTranslate(Source->RenderObject->Pos);
		auto rotMatrix = Matrix::MakeRotate(Source->RenderObject->Rot);
		auto scaleMatrix = Matrix::MakeScale(Source->RenderObject->Scale);
		Matrix World = posMatrix * rotMatrix * scaleMatrix;

		jCamera* CurrentCamera = nullptr;
		Matrix ProjView;
		if (Settings.PossessMockCamera || Settings.PossessOnlyMockCameraPP)
			ProjView = MockCamera->Projection * MockCamera->View * World;
		else
			ProjView = VCProj * VCView * World;

		jStreamParam<float>* thisObjectParam = dynamic_cast<jStreamParam<float>*>(ObjectPP->RenderObject->VertexStream->Params[0]);
		jStreamParam<float>* sourceObjectParam = dynamic_cast<jStreamParam<float>*>(Source->RenderObject->VertexStream->Params[0]);

		int32 ElementCount = (int32)sourceObjectParam->GetBufferSize() / sourceObjectParam->Stride;
		Vector* psourceData = (Vector*)sourceObjectParam->GetBufferData();
		Vector* pThisObjectData = (Vector*)thisObjectParam->GetBufferData();
		for (int32 i = 0; i < ElementCount; ++i)
		{
			const Vector& Pos = *(psourceData + i);
			Vector& ThisObjectPos = *(pThisObjectData + i);

			ThisObjectPos = ProjView.Transform(Pos);
			ThisObjectPos.x = -ThisObjectPos.x;				// NDC 좌표계 -> OpenGL 좌표계로 변환, X축이 서로 반전되어있으므로 그 부분을 추가 해줌.
		}
		ObjectPP->RenderObject->UpdateVertexStream();
		ObjectPPs.push_back(ObjectPP);
	};

	TransformPP(CubePP, Cube);
	TransformPP(SpherePP, Sphere);
	TransformPP(Cube2PP, Cube2);
	TransformPP(CapsulePP, Capsule);
	TransformPP(ConePP, Cone);
	TransformPP(CylinderPP, Cylinder);
	TransformPP(Quad2PP, Quad2);
	TransformPP(Sphere2PP, Sphere2);
	TransformPP(Sphere3PP, Sphere3);
	TransformPP(BillboardPP, Billboard);
	//////////////////////////////////////////////////////////////////////////

	// MockCamera로 화면을 렌더링하는 상황이 아닌 경우만 디버깅용 PP 공간의 오브젝트들이나 Frustum 정보를 출력
	{
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
		MockCameraPPFrustumDebug->Scale = Vector(Settings.ScalePP);

		static auto PPCameraFrustumDebug = jPrimitiveUtil::CreateFrustumDebug(PPCamera.get());
		PPCameraFrustumDebug->TargetCamera = PPCamera.get();
		PPCameraFrustumDebug->PostPerspective = false;
		PPCameraFrustumDebug->DrawPlane = false;
		PPCameraFrustumDebug->Update(deltaTime);
		PPCameraFrustumDebug->Offset = Settings.OffsetPP;
		PPCameraFrustumDebug->Scale = Vector(Settings.ScalePP);

		if (UnitCubeRT->Begin())
		{
			auto ClearColor = Vector4(0.1f, 0.1f, 0.1f, 1.0f);
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

			UnitCubeCamera->BindCamera(Shader);
			jLight::BindLights(lights, Shader);

			MockCameraPPFrustumDebug->Draw(UnitCubeCamera, Shader, lights);
			PPCameraFrustumDebug->Draw(UnitCubeCamera, Shader, lights);

			for (uint32 i = 0; i < ObjectPPs.size(); ++i)
				ObjectPPs[i]->Draw(UnitCubeCamera, Shader, lights);
			
			UnitCubeRT->End();
		}

		if (!Settings.PossessMockCamera)
		{
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
			MockCameraFrustumDebug->Draw(MainCamera, Shader, lights);
		}
	}

	// MockCamera 기준 화면을 텍스쳐에 렌더링
	static auto MockCameraRT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::RGBA
		, ETextureFormat::RGBA, EFormatType::BYTE, EDepthBufferType::DEPTH24_STENCIL8, 300, 300, 1 }));
	if (MockCameraRT->Begin())
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

		MockCameraRT->End();
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
	g_rhi->SetViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	if (DirectionalLight->GetShadowMapRenderTarget()->GetTexture())
	{
		PreviewUI->Pos = Vector2(SCR_WIDTH - PreviewSize.x, SCR_HEIGHT - PreviewSize.y);
		PREVIEW_TEXTURE(DirectionalLight->GetShadowMapRenderTarget()->GetTexture());
	}

	PreviewUI->Pos = Vector2(SCR_WIDTH - PreviewSize.x * 2, SCR_HEIGHT - PreviewSize.y);
	PREVIEW_TEXTURE(MockCameraRT->GetTexture());

	// NDC UnitCube Window
	static auto UnitCubePreViewUI = jPrimitiveUtil::CreateUIQuad(
		Vector2(UnitCubeRect.X, UnitCubeRect.Y - SCR_HEIGHT + UnitCubeRect.H), Vector2(UnitCubeRect.W, UnitCubeRect.H), nullptr);
	{
		auto EnableClear = false; 
		auto EnableDepthTest = false; 
		auto DepthStencilFunc = EComparisonFunc::LESS; 
		auto EnableBlend = false; 
		auto BlendSrc = EBlendSrc::ONE; 
		auto BlendDest = EBlendDest::ZERO; 
		auto Shader = jShader::GetShader("UIShader"); 
		g_rhi->EnableDepthTest(false); 
		g_rhi->EnableBlend(EnableBlend); 
		g_rhi->SetBlendFunc(BlendSrc, BlendDest); 
		g_rhi->SetShader(Shader); 
		MainCamera->BindCamera(Shader);
		UnitCubePreViewUI->RenderObject->tex_object = UnitCubeRT->GetTexture();
		UnitCubePreViewUI->Draw(MainCamera, Shader, {});
	}	
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
		if (IsMovingUnitCubeWindow)
			CurrentCamera = UnitCubeCamera;
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

void jGame::OnMouseInput(EMouseButtonType buttonType, Vector2 pos, bool buttonDown, bool isChanged)
{
	if (isChanged)
	{
		if (buttonDown && (EMouseButtonType::LEFT == buttonType))
			IsMovingUnitCubeWindow = IsInUnitCubeWindow(pos);
		if (!buttonDown)
			IsMovingUnitCubeWindow = false;
	}
}

void jGame::Teardown()
{
	Renderer->Teardown();
}

bool jGame::IsInUnitCubeWindow(const Vector2& pos) const
{
	return (UnitCubeRect.X <= pos.x) && ((UnitCubeRect.X + UnitCubeRect.W) >= pos.x)
		&& (UnitCubeRect.Y <= pos.y) && ((UnitCubeRect.Y + UnitCubeRect.H) >= pos.y);
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
	quad->SkipShadowMapGen = false;
	quad->SkipUpdateShadowVolume = true;
	jObject::AddObject(quad);
	SpawnedObjects.push_back(quad);

	// 1
	Vector CubePos = Vector(0.0f, 100.0f, 0.0f);
	Vector CubeScale = Vector(50.0f, 50.0f, 50.0f);
	Vector4 CubeColor = Vector4(0.7f, 0.7f, 0.7f, 1.0f);
	Cube = jPrimitiveUtil::CreateCube(CubePos, Vector::OneVector, CubeScale, CubeColor);
	Cube->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.z += 0.005f;
	};
	jObject::AddObject(Cube);
	SpawnedObjects.push_back(Cube);

	CubePP = jPrimitiveUtil::CreateCube(CubePos, Vector::OneVector, CubeScale, CubeColor);
	//jObject::AddObject(CubePP);
	SpawnedObjects.push_back(CubePP);

	// 2
	Vector SpherePos = Vector(0.0f, 100.0f, 0.0f);
	Vector SphereScale = Vector(30.0f);
	Vector4 SphereColor = Vector4(0.8f, 0.0f, 0.0f, 1.0f);
	Sphere = jPrimitiveUtil::CreateSphere(SpherePos, 1.0, 16, SphereScale, SphereColor);
	Sphere->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.z = DegreeToRadian(180.0f);
	};
	jObject::AddObject(Sphere);
	SpawnedObjects.push_back(Sphere);

	SpherePP = jPrimitiveUtil::CreateSphere(SpherePos, 1.0, 16, SphereScale, SphereColor);
	//jObject::AddObject(SpherePP);
	SpawnedObjects.push_back(SpherePP);

	// 3
	Vector Cube2Pos = Vector(-65.0f, 35.0f, 10.0f);
	Vector Cube2Scale = Vector(50.0f, 50.0f, 50.0f);
	Vector4 Cube2Color = Vector4(0.7f, 0.7f, 0.7f, 1.0f);
	Cube2 = jPrimitiveUtil::CreateCube(Cube2Pos, Vector::OneVector, Cube2Scale, Cube2Color);
	jObject::AddObject(Cube2);
	SpawnedObjects.push_back(Cube2);

	Cube2PP = jPrimitiveUtil::CreateCube(Cube2Pos, Vector::OneVector, Cube2Scale, Cube2Color);
	SpawnedObjects.push_back(Cube2PP);

	// 4
	Vector CapsulePos = Vector(30.0f, 30.0f, -80.0f);
	Vector CapsuleScale = Vector(1.0f);
	Vector4 CapsuleColor = Vector4(1.0f, 1.0f, 0.0f, 1.0f);
	Capsule = jPrimitiveUtil::CreateCapsule(CapsulePos, 40.0f, 10.0f, 20, CapsuleScale, CapsuleColor);
	Capsule->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.x -= 0.01f;
	};
	jObject::AddObject(Capsule);
	SpawnedObjects.push_back(Capsule);

	CapsulePP = jPrimitiveUtil::CreateCapsule(CapsulePos, 40.0f, 10.0f, 20, CapsuleScale, CapsuleColor);
	CapsulePP->PostUpdateFunc = [&](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.x = Capsule->RenderObject->Rot.x;
	};
	SpawnedObjects.push_back(CapsulePP);

	// 5
	Vector ConePos = Vector(0.0f, 50.0f, 60.0f);
	Vector ConeScale = Vector(1.0f);
	Vector4 ConeColor = Vector4(1.0f, 1.0f, 0.0f, 1.0f);
	Cone = jPrimitiveUtil::CreateCone(ConePos, 40.0f, 20.0f, 15, ConeScale, ConeColor);
	Cone->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.y += 0.03f;
	};
	jObject::AddObject(Cone);
	SpawnedObjects.push_back(Cone);

	ConePP = jPrimitiveUtil::CreateCone(ConePos, 40.0f, 20.0f, 15, ConeScale, ConeColor);
	ConePP->PostUpdateFunc = [&](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.y = Cone->RenderObject->Rot.y;
	};
	SpawnedObjects.push_back(ConePP);

	// 6
	Vector CylinderPos = Vector(-30.0f, 60.0f, -60.0f);
	Vector CylinderScale = Vector(1.0f);
	Vector4 CylinderColor = Vector4(0.0f, 0.0f, 1.0f, 1.0f);
	Cylinder = jPrimitiveUtil::CreateCylinder(CylinderPos, 20.0f, 10.0f, 20, CylinderScale, CylinderColor);
	Cylinder->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.x += 0.05f;
	};
	jObject::AddObject(Cylinder);
	SpawnedObjects.push_back(Cylinder);

	CylinderPP = jPrimitiveUtil::CreateCylinder(CylinderPos, 20.0f, 10.0f, 20, CylinderScale, CylinderColor);
	CylinderPP->PostUpdateFunc = [&](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.x = Cylinder->RenderObject->Rot.x;
	};
	SpawnedObjects.push_back(CylinderPP);

	// 7
	Vector Quad2Pos = Vector(-20.0f, 80.0f, 40.0f);
	Vector Quad2Scale = Vector(20.0f, 20.0f, 20.0f);
	Vector4 Quad2Color = Vector4(0.0f, 0.0f, 1.0f, 1.0f);
	Quad2 = jPrimitiveUtil::CreateQuad(Quad2Pos, Vector::OneVector, Quad2Scale, Quad2Color);
	Quad2->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.z += 0.08f;
	};
	jObject::AddObject(Quad2);
	SpawnedObjects.push_back(Quad2);

	Quad2PP = jPrimitiveUtil::CreateQuad(Quad2Pos, Vector::OneVector, Quad2Scale, Quad2Color);
	Quad2PP->PostUpdateFunc = [&](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.z = Quad2->RenderObject->Rot.z;
	};
	SpawnedObjects.push_back(Quad2PP);

	// 8
	Vector Sphere2Pos = Vector(65.0f, 35.0f, 10.0f);
	Vector Sphere2Scale = Vector(30.0f);
	Vector4 Sphere2Color = Vector4(0.8f, 0.0f, 0.0f, 1.0f);
	Sphere2 = jPrimitiveUtil::CreateSphere(Sphere2Pos, 1.0, 16, Sphere2Scale, Sphere2Color);
	Sphere2->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.z = DegreeToRadian(180.0f);
	};
	jObject::AddObject(Sphere2);
	SpawnedObjects.push_back(Sphere2);

	Sphere2PP = jPrimitiveUtil::CreateSphere(Sphere2Pos, 1.0, 16, Sphere2Scale, Sphere2Color);
	Sphere2PP->PostUpdateFunc = [&](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.z = Sphere2->RenderObject->Rot.z;
	};
	SpawnedObjects.push_back(Sphere2PP);

	// 9
	Vector Sphere3Pos = Vector(150.0f, 5.0f, 0.0f);
	Vector Sphere3Scale = Vector(10.0f);
	Vector4 Sphere3Color = Vector4(0.8f, 0.4f, 0.6f, 1.0f);
	Sphere3 = jPrimitiveUtil::CreateSphere(Sphere3Pos, 1.0, 16, Sphere3Scale, Sphere3Color);
	Sphere3->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		const float startY = 5.0f;
		const float endY = 100;
		const float speed = 1.5f;
		static bool dir = true;
		thisObject->RenderObject->Pos.y += dir ? speed : -speed;
		if (thisObject->RenderObject->Pos.y < startY || thisObject->RenderObject->Pos.y > endY)
		{
			dir = !dir;
			thisObject->RenderObject->Pos.y += dir ? speed : -speed;
		}
	};
	jObject::AddObject(Sphere3);
	SpawnedObjects.push_back(Sphere3);

	Sphere3PP = jPrimitiveUtil::CreateSphere(Sphere3Pos, 1.0, 16, Sphere3Scale, Sphere3Color);
	Sphere3PP->PostUpdateFunc = [&](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Pos.y = Sphere3->RenderObject->Pos.y;
	};
	SpawnedObjects.push_back(Sphere3PP);

	// 10
	Vector BillboardPos = Vector(0.0f, 60.0f, 80.0f);
	Vector BillboardScale = Vector(20.0f, 20.0f, 20.0f);
	Vector4 BillboardColor = Vector4(1.0f, 0.0f, 1.0f, 1.0f);
	Billboard = jPrimitiveUtil::CreateBillobardQuad(BillboardPos, Vector::OneVector, BillboardScale, BillboardColor, MainCamera);
	jObject::AddObject(Billboard);
	SpawnedObjects.push_back(Billboard);

	BillboardPP = jPrimitiveUtil::CreateBillobardQuad(BillboardPos, Vector::OneVector, BillboardScale, BillboardColor, MainCamera);
	SpawnedObjects.push_back(BillboardPP);
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