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
#include "jSamplerStatePool.h"

jRHI* g_rhi = nullptr;
jMeshObject* Shed = nullptr;

jGame::jGame()
{
	g_rhi = new jRHI_OpenGL();
}

jGame::~jGame()
{
}

void jGame::ProcessInput()
{
	static float speed = 10.0f;

	// Process Key Event
	if (g_KeyState['a'] || g_KeyState['A']) MainCamera->MoveShift(-speed);
	if (g_KeyState['d'] || g_KeyState['D']) MainCamera->MoveShift(speed);
	//if (g_KeyState['1']) MainCamera->RotateForwardAxis(-0.1f);
	//if (g_KeyState['2']) MainCamera->RotateForwardAxis(0.1f);
	//if (g_KeyState['3']) MainCamera->RotateUpAxis(-0.1f);
	//if (g_KeyState['4']) MainCamera->RotateUpAxis(0.1f);
	//if (g_KeyState['5']) MainCamera->RotateRightAxis(-0.1f);
	//if (g_KeyState['6']) MainCamera->RotateRightAxis(0.1f);
	if (g_KeyState['w'] || g_KeyState['W']) MainCamera->MoveForward(speed);
	if (g_KeyState['s'] || g_KeyState['S']) MainCamera->MoveForward(-speed);
	if (g_KeyState['+']) speed = Max(speed + 0.1f, 0.0f);
	if (g_KeyState['-']) speed = Max(speed - 0.1f, 0.0f);
}

void jGame::Setup()
{
	//////////////////////////////////////////////////////////////////////////
	const Vector mainCameraPos(812.58f, 338.02f, -433.87f);
	//const Vector mainCameraTarget(171.96f, 166.02f, -180.05f);
	//const Vector mainCameraPos(165.0f, 125.0f, -136.0f);
	//const Vector mainCameraPos(300.0f, 100.0f, 300.0f);
	const Vector mainCameraTarget(811.81f, 337.78f, -433.36f);
	const Vector mainCameraUp(812.35f, 338.98f, -433.72f);
	MainCamera = jCamera::CreateCamera(mainCameraPos, mainCameraTarget, mainCameraUp
		, DegreeToRadian(45.0f), 10.0f, 5000.0f, SCR_WIDTH, SCR_HEIGHT, true);
	jCamera::AddCamera(0, MainCamera);

	// Light creation step
	NormalDirectionalLight = jLight::CreateDirectionalLight(jShadowAppSettingProperties::GetInstance().DirecionalLightDirection
		, Vector4(0.6f)
		, Vector(1.0f), Vector(1.0f), 64);
	CascadeDirectionalLight = jLight::CreateCascadeDirectionalLight(jShadowAppSettingProperties::GetInstance().DirecionalLightDirection
		, Vector4(0.6f)
		, Vector(1.0f), Vector(1.0f), 64);

	DirectionalLight = NormalDirectionalLight;

	//AmbientLight = jLight::CreateAmbientLight(Vector(0.7f, 0.8f, 0.8f), Vector(0.1f));
	AmbientLight = jLight::CreateAmbientLight(Vector(0.2f, 0.5f, 1.0f), Vector(0.05f));		// sky light color

	//PointLight = jLight::CreatePointLight(jShadowAppSettingProperties::GetInstance().PointLightPosition, Vector4(2.0f, 0.7f, 0.7f, 1.0f), 500.0f, Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), 64.0f);
	//SpotLight = jLight::CreateSpotLight(jShadowAppSettingProperties::GetInstance().SpotLightPosition, jShadowAppSettingProperties::GetInstance().SpotLightDirection, Vector4(0.0f, 1.0f, 0.0f, 1.0f), 500.0f, 0.7f, 1.0f, Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), 64.0f);

	DirectionalLightInfo = jPrimitiveUtil::CreateDirectionalLightDebug(Vector(250, 260, 0)*0.5f, Vector::OneVector * 10.0f, 10.0f, MainCamera, DirectionalLight, "Image/sun.png");
	jObject::AddDebugObject(DirectionalLightInfo);

	DirectionalLightShadowMapUIDebug = jPrimitiveUtil::CreateUIQuad({ 0.0f, 0.0f }, { 150, 150 }, DirectionalLight->GetShadowMap());
	jObject::AddUIDebugObject(DirectionalLightShadowMapUIDebug);

	//PointLightInfo = jPrimitiveUtil::CreatePointLightDebug(Vector(10.0f), MainCamera, PointLight, "Image/bulb.png");
	////jObject::AddDebugObject(PointLightInfo);

	//SpotLightInfo = jPrimitiveUtil::CreateSpotLightDebug(Vector(10.0f), MainCamera, SpotLight, "Image/spot.png");
	////jObject::AddDebugObject(SpotLightInfo);

	MainCamera->AddLight(DirectionalLight);
	//MainCamera->AddLight(PointLight);
	//MainCamera->AddLight(SpotLight);
	MainCamera->AddLight(AmbientLight);

	//SpawnObjects(ESpawnedType::TestPrimitive);

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

	CurrentShadowMapType = jShadowAppSettingProperties::GetInstance().ShadowMapType;

	//ForwardRenderer = new jForwardRenderer(ShadowPipelineSetMap[CurrentShadowMapType]);
	ShadowVolumePipelineSet = CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_ShadowVolume);

	// todo 정리 필요
	const auto currentShadowPipelineSet = (jShadowAppSettingProperties::GetInstance().ShadowType == EShadowType::ShadowMap) 
		? ShadowPipelineSetMap[CurrentShadowMapType]  : ShadowVolumePipelineSet;
	ForwardRenderer = new jForwardRenderer(currentShadowPipelineSet);
	ForwardRenderer->Setup();

	DeferredRenderer = new jDeferredRenderer({ ETextureType::TEXTURE_2D, ETextureFormat::RGBA32F, ETextureFormat::RGBA, EFormatType::FLOAT, EDepthBufferType::DEPTH16, SCR_WIDTH, SCR_HEIGHT, 4 });
	DeferredRenderer->Setup();

	//for (int32 i = 0; i < NUM_CASCADES; ++i)
	//{
	//	jObject::AddUIDebugObject(jPrimitiveUtil::CreateUIQuad({ i * 150.0f, 0.0f }, { 150.0f, 150.0f }, DirectionalLight->ShadowMapData->CascadeShadowMapRenderTarget[i]->GetTexture()));
	//}

	Shed = jModelLoader::GetInstance().LoadFromFile("LightVolumeModel/shed.sdkmesh.obj");
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

	//DirectionalLight->ShadowMapData->ShadowMapCamera->Pos = MainCamera->Pos - Vector(1000.0f, 500.0f, 1000.0f) * DirectionalLight->Data.Direction;
	//jLightUtil::MakeDirectionalLightViewInfoWithPos(DirectionalLight->ShadowMapData->ShadowMapCamera->Target, DirectionalLight->ShadowMapData->ShadowMapCamera->Up
	//	, DirectionalLight->ShadowMapData->ShadowMapCamera->Pos, DirectionalLight->Data.Direction);
	//DirectionalLight->ShadowMapData->ShadowMapCamera->UpdateCamera();

	const int32 numOfLights = MainCamera->GetNumOfLight();
	for (int32 i = 0; i < numOfLights; ++i)
	{
		auto light = MainCamera->GetLight(i);
		JASSERT(light);
		light->Update(deltaTime);
	}

	////////////////////////////////////////////////////////////////////////////
	//// Get the 8 points of the view frustum in world space
	//if (jShadowAppSettingProperties::GetInstance().ShadowMapType != EShadowMapType::DeepShadowMap_DirectionalLight)
	//{
	//	Vector frustumCornersWS[8] =
	//	{
	//		Vector(-1.0f,  1.0f, -1.0f),
	//		Vector(1.0f,  1.0f, -1.0f),
	//		Vector(1.0f, -1.0f, -1.0f),
	//		Vector(-1.0f, -1.0f, -1.0f),
	//		Vector(-1.0f,  1.0f, 1.0f),
	//		Vector(1.0f,  1.0f, 1.0f),
	//		Vector(1.0f, -1.0f, 1.0f),
	//		Vector(-1.0f, -1.0f, 1.0f),
	//	};

	//	Vector frustumCenter(0.0f);
	//	Matrix invViewProj = (MainCamera->Projection * MainCamera->View).GetInverse();
	//	for (uint32 i = 0; i < 8; ++i)
	//	{
	//		frustumCornersWS[i] = invViewProj.Transform(frustumCornersWS[i]);
	//		frustumCenter = frustumCenter + frustumCornersWS[i];
	//	}
	//	frustumCenter = frustumCenter * (1.0f / 8.0f);

	//	auto upDir = Vector::UpVector;

	//	float width = SM_WIDTH;
	//	float height = SM_HEIGHT;
	//	float nearDist = 10.0f;
	//	float farDist = 1000.0f;

	//	// Get position of the shadow camera
	//	Vector shadowCameraPos = frustumCenter + DirectionalLight->Data.Direction * -(farDist - nearDist) / 2.0f;
	//
	//	auto shadowCamera = jOrthographicCamera::CreateCamera(shadowCameraPos, frustumCenter, shadowCameraPos + upDir
	//		, -width / 2.0f, -height / 2.0f, width / 2.0f, height / 2.0f, farDist, nearDist);
	//	shadowCamera->UpdateCamera();
	//	DirectionalLight->GetLightCamra()->Projection = shadowCamera->Projection;
	//	DirectionalLight->GetLightCamra()->View = shadowCamera->View;
	//}
	//////////////////////////////////////////////////////////////////////////

	for (auto iter : jObject::GetStaticObject())
		iter->Update(deltaTime);

	for (auto& iter : jObject::GetBoundBoxObject())
		iter->Update(deltaTime);

	for (auto& iter : jObject::GetBoundSphereObject())
		iter->Update(deltaTime);

	for (auto& iter : jObject::GetDebugObject())
		iter->Update(deltaTime);

	jObject::FlushDirtyState();

	jRenderTargetInfo ShadowMapWorldRTInfo;
	ShadowMapWorldRTInfo.TextureType = ETextureType::TEXTURE_2D;
	ShadowMapWorldRTInfo.InternalFormat = ETextureFormat::R32F;
	ShadowMapWorldRTInfo.Format = ETextureFormat::R;
	ShadowMapWorldRTInfo.FormatType = EFormatType::FLOAT;
	ShadowMapWorldRTInfo.DepthBufferType = EDepthBufferType::DEPTH32;
	ShadowMapWorldRTInfo.Width = SM_WIDTH;
	ShadowMapWorldRTInfo.Height = SM_HEIGHT;
	ShadowMapWorldRTInfo.TextureCount = 1;
	ShadowMapWorldRTInfo.Magnification = ETextureFilter::NEAREST;
	ShadowMapWorldRTInfo.Minification = ETextureFilter::NEAREST;

	static auto ShadowMapWorldRT = jRenderTargetPool::GetRenderTarget(ShadowMapWorldRTInfo);

	static constexpr int32 SHADOW_MIPS = 4;
	static std::shared_ptr<jRenderTarget> ShadowMapWorldMipsRT[SHADOW_MIPS];
	static std::shared_ptr<jRenderTarget> ShadowMapWorldScaledOptRT;
	static std::shared_ptr<jRenderTarget> ShadowMapHoleRT[2];

	static bool InitializedShadowMaps = false;
	if (!InitializedShadowMaps)
	{
		for (int32 i = 0; i < SHADOW_MIPS; ++i)
		{
			ShadowMapWorldRTInfo.InternalFormat = ETextureFormat::RG32F;
			ShadowMapWorldRTInfo.Format = ETextureFormat::RG;
			ShadowMapWorldRTInfo.Width /= 2;
			ShadowMapWorldRTInfo.Height /= 2;
			ShadowMapWorldMipsRT[i] = jRenderTargetPool::GetRenderTarget(ShadowMapWorldRTInfo);

			if (i == (SHADOW_MIPS - 1))
			{
				ShadowMapWorldScaledOptRT = jRenderTargetPool::GetRenderTarget(ShadowMapWorldRTInfo);

				ShadowMapWorldRTInfo.InternalFormat = ETextureFormat::R32F;
				ShadowMapWorldRTInfo.Format = ETextureFormat::R;
				ShadowMapHoleRT[0] = jRenderTargetPool::GetRenderTarget(ShadowMapWorldRTInfo);
				ShadowMapHoleRT[1] = jRenderTargetPool::GetRenderTarget(ShadowMapWorldRTInfo);
			}
		}
		InitializedShadowMaps = true;
	}

	// [1]. ShadowMap Render
	{
		g_rhi->BeginDebugEvent("[1]. ShadowMapRender");

		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto ShadowGenShader = jShader::GetShader("ShadowGen_SSM");
		JASSERT(ShadowGenShader);

		g_rhi->SetRenderTarget(DirectionalLight->GetShadowMapRenderTarget());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->SetDepthFunc(DepthStencilFunc);

		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(ShadowGenShader);

		DirectionalLight->GetLightCamra()->BindCamera(ShadowGenShader);

		Shed->Draw(DirectionalLight->GetLightCamra(), ShadowGenShader, {});

		g_rhi->SetRenderTarget(nullptr);

		g_rhi->EndDebugEvent();
	}

	jRenderTargetInfo MainSceneRTInfo;
	MainSceneRTInfo.TextureType = ETextureType::TEXTURE_2D;
	MainSceneRTInfo.InternalFormat = ETextureFormat::RGBA32F;
	MainSceneRTInfo.Format = ETextureFormat::RGBA;
	MainSceneRTInfo.FormatType = EFormatType::FLOAT;
	MainSceneRTInfo.DepthBufferType = EDepthBufferType::DEPTH32;
	MainSceneRTInfo.Width = SCR_WIDTH;
	MainSceneRTInfo.Height = SCR_HEIGHT;
	MainSceneRTInfo.TextureCount = 1;
	MainSceneRTInfo.Magnification = ETextureFilter::NEAREST;
	MainSceneRTInfo.Minification = ETextureFilter::NEAREST;

	static auto MainSceneRT = jRenderTargetPool::GetRenderTarget(MainSceneRTInfo);

	static bool IsDebug = false;
	if (g_KeyState['1'])
		IsDebug = true;
	if (g_KeyState['2'])
		IsDebug = false;

	// [2]. MainScene Render
	{
		g_rhi->BeginDebugEvent("[2]. MainScene Render");

		auto EnableClear = true;
		auto ClearColor = Vector4(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);	// light sky blue
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("SSM");

		//if (IsDebug)
			g_rhi->SetRenderTarget(MainSceneRT.get());
		//else
		//	g_rhi->SetRenderTarget(nullptr);

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->SetDepthFunc(DepthStencilFunc);

		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);

		g_rhi->EnableDepthBias(false);
		//g_rhi->SetDepthBias(DepthConstantBias, DepthSlopeBias);

		g_rhi->SetShader(Shader);

		MainCamera->BindCamera(Shader);
		DirectionalLight->BindLight(Shader);
		jLight::BindLights({ DirectionalLight }, Shader);

		Shed->Draw(MainCamera, Shader, { DirectionalLight });

		g_rhi->SetRenderTarget(nullptr);

		g_rhi->EndDebugEvent();
	}

	//if (!IsDebug)
	//	return;


	// [3]. Convert Depth to world space scale
	static jFullscreenQuadPrimitive* FullScreenQuad = jPrimitiveUtil::CreateFullscreenQuad(nullptr);
	{
		g_rhi->BeginDebugEvent("[3]. Depth to world scale");

		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("ConvertDepthWorld");

		g_rhi->SetRenderTarget(ShadowMapWorldRT.get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		auto LightCamera = DirectionalLight->GetLightCamra();
		auto LightVP = (LightCamera->Projection * LightCamera->View);
		auto LightVPInv = (LightCamera->Projection * LightCamera->View).GetInverse();

		SET_UNIFORM_BUFFER_STATIC(Matrix, "LightVPInv", LightVPInv, Shader);

		Vector LightPos = LightCamera->Pos;
		Vector LightForward = (LightCamera->Target - LightCamera->Pos).GetNormalize();

		SET_UNIFORM_BUFFER_STATIC(Vector, "LightPos", LightPos, Shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "LightForward", LightForward, Shader);

		auto PointSamplerPtr = jSamplerStatePool::GetSamplerState("Point");
		FullScreenQuad->SetTexture(DirectionalLight->GetTextureDepth(), PointSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
		g_rhi->EndDebugEvent();
	}

	// [4]. Generate Min/Max depth ShadowMap Mip level chain From 2048 to 128 for optimal tracing
	g_rhi->BeginDebugEvent("[4]. Generate Min/Max ShadowMap Mip");
	{

		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("MinMaxFromDepth");

		g_rhi->SetRenderTarget(ShadowMapWorldMipsRT[0].get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		Vector2 BufferSizeInv(
			1.0f / ShadowMapWorldMipsRT[0]->Info.Width,
			1.0f / ShadowMapWorldMipsRT[0]->Info.Height);
		SET_UNIFORM_BUFFER_STATIC(Vector2, "BufferSizeInv", BufferSizeInv, Shader);

		auto PointSamplerPtr = jSamplerStatePool::GetSamplerState("Point");
		FullScreenQuad->SetTexture(ShadowMapWorldRT->GetTexture(), PointSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
	}

	for (int32 i = 0; i < SHADOW_MIPS - 1; ++i)
	{
		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("MinMaxFromMinMax");

		g_rhi->SetRenderTarget(ShadowMapWorldMipsRT[i + 1].get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		Vector2 BufferSizeInv(
			1.0f / ShadowMapWorldMipsRT[i]->Info.Width,
			1.0f / ShadowMapWorldMipsRT[i]->Info.Height);
		SET_UNIFORM_BUFFER_STATIC(Vector2, "BufferSizeInv", BufferSizeInv, Shader);

		auto PointSamplerPtr = jSamplerStatePool::GetSamplerState("Point");
		FullScreenQuad->SetTexture(ShadowMapWorldMipsRT[i]->GetTexture(), PointSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
	}

	{
		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("MinMax_3x3");

		g_rhi->SetRenderTarget(ShadowMapWorldScaledOptRT.get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		auto PointSamplerPtr = jSamplerStatePool::GetSamplerState("Point");
		FullScreenQuad->SetTexture(ShadowMapWorldMipsRT[SHADOW_MIPS - 1]->GetTexture(), PointSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
	}
	g_rhi->EndDebugEvent();

	// [5]. Fill the holes
	g_rhi->BeginDebugEvent("[5]. Fill the holes");
	{

		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("Fill_The_Hole_Min");

		g_rhi->SetRenderTarget(ShadowMapHoleRT[0].get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		auto PointSamplerPtr = jSamplerStatePool::GetSamplerState("Point");
		FullScreenQuad->SetTexture(ShadowMapWorldMipsRT[SHADOW_MIPS - 1]->GetTexture(), PointSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
	}

	for (int32 i = 0; i < 2; ++i)
	{
		const int32 Flip = (i % 2);

		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("Fill_The_Hole_Min2");

		g_rhi->SetRenderTarget(ShadowMapHoleRT[1 - Flip].get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		auto PointSamplerPtr = jSamplerStatePool::GetSamplerState("Point");
		FullScreenQuad->SetTexture(ShadowMapHoleRT[Flip]->GetTexture(), PointSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
	}

	for (int32 i = 0; i < 5; ++i)
	{
		const int32 Flip = (i % 2);

		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("Fill_The_Hole_Max");

		g_rhi->SetRenderTarget(ShadowMapHoleRT[1 - Flip].get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		auto PointSamplerPtr = jSamplerStatePool::GetSamplerState("Point");
		FullScreenQuad->SetTexture(ShadowMapHoleRT[Flip]->GetTexture(), PointSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
	}
	g_rhi->EndDebugEvent();


	float Scale = 4.0f;
	jRenderTargetInfo LightVolumeHDRRTInfo;
	LightVolumeHDRRTInfo.TextureType = ETextureType::TEXTURE_2D;
	LightVolumeHDRRTInfo.InternalFormat = ETextureFormat::RGBA32F;
	LightVolumeHDRRTInfo.Format = ETextureFormat::RGBA;
	LightVolumeHDRRTInfo.FormatType = EFormatType::FLOAT;
	LightVolumeHDRRTInfo.DepthBufferType = EDepthBufferType::NONE;
	LightVolumeHDRRTInfo.Width = SCR_WIDTH / 4.0f;
	LightVolumeHDRRTInfo.Height = SCR_HEIGHT / 4.0f;
	LightVolumeHDRRTInfo.TextureCount = 1;
	LightVolumeHDRRTInfo.Magnification = ETextureFilter::NEAREST;
	LightVolumeHDRRTInfo.Minification = ETextureFilter::NEAREST;

	static auto LightVolumeHDRRT = jRenderTargetPool::GetRenderTarget(LightVolumeHDRRTInfo);

	// [6]. LightVolume 계산
	{
		g_rhi->BeginDebugEvent("[6]. LightVolume Calc");
		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("GenerateLightVolume");

		g_rhi->SetRenderTarget(LightVolumeHDRRT.get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		Vector2 BufferSizeInv(
			1.0f / LightVolumeHDRRTInfo.Width,
			1.0f / LightVolumeHDRRTInfo.Height);
		SET_UNIFORM_BUFFER_STATIC(Vector2, "BufferSizeInv", BufferSizeInv, Shader);
		SET_UNIFORM_BUFFER_STATIC(float, "CoarseDepthTexelSize", static_cast<float>(SM_WIDTH) / LightVolumeHDRRTInfo.Width, Shader);

		auto LightCamera = DirectionalLight->GetLightCamra();

		auto CameraPInv = (MainCamera->Projection).GetInverse();
		auto CameraVInv = (MainCamera->View).GetInverse();
		auto CameraVPInv = (MainCamera->Projection * MainCamera->View).GetInverse();

		SET_UNIFORM_BUFFER_STATIC(Matrix, "CameraPInv", CameraPInv, Shader);
		SET_UNIFORM_BUFFER_STATIC(Matrix, "CameraVInv", CameraVInv, Shader);
		SET_UNIFORM_BUFFER_STATIC(Matrix, "CameraVPInv", CameraVPInv, Shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "EyePos", MainCamera->Pos, Shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "EyeForward", (MainCamera->Target - MainCamera->Pos).GetNormalize(), Shader);

		SET_UNIFORM_BUFFER_STATIC(float, "CameraNear", MainCamera->Near, Shader);
		SET_UNIFORM_BUFFER_STATIC(float, "CameraFar", MainCamera->Far, Shader);

		SET_UNIFORM_BUFFER_STATIC(Vector, "LightPos", LightCamera->Pos, Shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "LightForward", LightCamera->GetForwardVector(), Shader);
		
		auto LightVP = (LightCamera->Projection * LightCamera->View);
		SET_UNIFORM_BUFFER_STATIC(Matrix, "LightVP", LightVP, Shader);

		Vector Loc(LightCamera->GetForwardVector());
		auto Result = LightVP.Transform(Loc);
		auto Result2 = LightVP.Transform(Loc * 10.0f);

		SET_UNIFORM_BUFFER_STATIC(Vector, "LightRight", LightCamera->GetRightVector(), Shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "LightUp", LightCamera->GetUpVector(), Shader);

		auto PointSamplerPtr = jSamplerStatePool::GetSamplerState("Point");
		FullScreenQuad->SetTexture(0, MainSceneRT->GetTextureDepth(), PointSamplerPtr.get());
		FullScreenQuad->SetTexture(1, ShadowMapWorldRT->GetTexture(), PointSamplerPtr.get());
		FullScreenQuad->SetTexture(2, ShadowMapWorldScaledOptRT->GetTexture(), PointSamplerPtr.get());
		FullScreenQuad->SetTexture(3, ShadowMapHoleRT[1]->GetTexture(), PointSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);

		g_rhi->EndDebugEvent();
	}

	//////////////////////////////////////////////////////////////////////////
	// [7]. ConvertDepthWorldNormalzed
	jRenderTargetInfo ConvertDepthWorldNormalizedInfo;
	ConvertDepthWorldNormalizedInfo.TextureType = ETextureType::TEXTURE_2D;
	ConvertDepthWorldNormalizedInfo.InternalFormat = ETextureFormat::R32F;
	ConvertDepthWorldNormalizedInfo.Format = ETextureFormat::R;
	ConvertDepthWorldNormalizedInfo.FormatType = EFormatType::FLOAT;
	ConvertDepthWorldNormalizedInfo.DepthBufferType = EDepthBufferType::NONE;
	ConvertDepthWorldNormalizedInfo.Width = SCR_WIDTH;
	ConvertDepthWorldNormalizedInfo.Height = SCR_HEIGHT;
	ConvertDepthWorldNormalizedInfo.TextureCount = 1;
	ConvertDepthWorldNormalizedInfo.Magnification = ETextureFilter::NEAREST;
	ConvertDepthWorldNormalizedInfo.Minification = ETextureFilter::NEAREST;

	static auto ConvertDepthWorldNormalizedRT = jRenderTargetPool::GetRenderTarget(ConvertDepthWorldNormalizedInfo);
	{
		g_rhi->BeginDebugEvent("[7]. ConvertDepthWorldNormalzed");

		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("ConvertDepthWorldNormalized");

		g_rhi->SetRenderTarget(ConvertDepthWorldNormalizedRT.get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		Matrix VPInv = (MainCamera->Projection * MainCamera->View).GetInverse();

		SET_UNIFORM_BUFFER_STATIC(Vector, "EyePos", MainCamera->Pos, Shader);
		SET_UNIFORM_BUFFER_STATIC(Matrix, "VPInv", VPInv, Shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "WorldFront", MainCamera->GetForwardVector(), Shader);

		auto PointSamplerPtr = jSamplerStatePool::GetSamplerState("Point");
		FullScreenQuad->SetTexture(MainSceneRT->GetTextureDepth(), PointSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);

		g_rhi->EndDebugEvent();
	}

	jRenderTargetInfo EdgeDetectionRTInfo;
	EdgeDetectionRTInfo.TextureType = ETextureType::TEXTURE_2D;
	EdgeDetectionRTInfo.InternalFormat = ETextureFormat::RG32F;
	EdgeDetectionRTInfo.Format = ETextureFormat::RG;
	EdgeDetectionRTInfo.FormatType = EFormatType::FLOAT;
	EdgeDetectionRTInfo.DepthBufferType = EDepthBufferType::NONE;
	EdgeDetectionRTInfo.Width = SCR_WIDTH / 4.0f;
	EdgeDetectionRTInfo.Height = SCR_HEIGHT / 4.0f;
	EdgeDetectionRTInfo.TextureCount = 1;
	EdgeDetectionRTInfo.Magnification = ETextureFilter::NEAREST;
	EdgeDetectionRTInfo.Minification = ETextureFilter::NEAREST;

	static auto EdgeDetectionRT = jRenderTargetPool::GetRenderTarget(EdgeDetectionRTInfo);

	// [8]. EdgeDetectionSobel
	{
		g_rhi->BeginDebugEvent("[8]. EdgeDetectionSobel");
		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("EdgeDetection");

		g_rhi->SetRenderTarget(EdgeDetectionRT.get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		SET_UNIFORM_BUFFER_STATIC(float, "CoarseTextureWidthInv", 1.0f / EdgeDetectionRTInfo.Width, Shader);
		SET_UNIFORM_BUFFER_STATIC(float, "CoarseTextureHeightInv", 1.0f / EdgeDetectionRTInfo.Height, Shader);

		auto LinearSamplerPtr = jSamplerStatePool::GetSamplerState("LinearClamp");
		FullScreenQuad->SetTexture(ConvertDepthWorldNormalizedRT->GetTexture(), LinearSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
		g_rhi->EndDebugEvent();
	}

	jRenderTargetInfo EdgeBlurRTInfo;
	EdgeBlurRTInfo.TextureType = ETextureType::TEXTURE_2D;
	EdgeBlurRTInfo.InternalFormat = ETextureFormat::R32F;
	EdgeBlurRTInfo.Format = ETextureFormat::R;
	EdgeBlurRTInfo.FormatType = EFormatType::FLOAT;
	EdgeBlurRTInfo.DepthBufferType = EDepthBufferType::NONE;
	EdgeBlurRTInfo.Width = SCR_WIDTH / 4.0f;
	EdgeBlurRTInfo.Height = SCR_HEIGHT / 4.0f;
	EdgeBlurRTInfo.TextureCount = 1;
	EdgeBlurRTInfo.Magnification = ETextureFilter::NEAREST;
	EdgeBlurRTInfo.Minification = ETextureFilter::NEAREST;

	static auto EdgeGradientBlurRT = jRenderTargetPool::GetRenderTarget(EdgeBlurRTInfo);
	static auto ImageBlurRT = jRenderTargetPool::GetRenderTarget(EdgeBlurRTInfo);

	// [9]. EdgeGradientBlur
	{
		g_rhi->BeginDebugEvent("[9]. EdgeGradientBlur");
		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("EdgeGradientBlur");

		g_rhi->SetRenderTarget(EdgeGradientBlurRT.get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		SET_UNIFORM_BUFFER_STATIC(float, "CoarseTextureWidthInv", 1.0f / EdgeDetectionRTInfo.Width, Shader);
		SET_UNIFORM_BUFFER_STATIC(float, "CoarseTextureHeightInv", 1.0f / EdgeDetectionRTInfo.Height, Shader);

		auto LinearSamplerPtr = jSamplerStatePool::GetSamplerState("LinearClamp");
		FullScreenQuad->SetTexture(EdgeDetectionRT->GetTexture(), LinearSamplerPtr.get());
		FullScreenQuad->SetTexture2(LightVolumeHDRRT->GetTexture(), LinearSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
		g_rhi->EndDebugEvent();
	}

	// [10]. ImageBlurRT
	{
		g_rhi->BeginDebugEvent("[10]. ImageBlurRT");
		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("EdgeImageBlur");

		g_rhi->SetRenderTarget(ImageBlurRT.get());

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		auto LinearSamplerPtr = jSamplerStatePool::GetSamplerState("LinearClamp");
		FullScreenQuad->SetTexture(EdgeGradientBlurRT->GetTexture(), LinearSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
		g_rhi->EndDebugEvent();
	}

	// [9]. 최종장면에 위에 만든 LightVolume 값을 블랜드 시킴
	//////////////////////////////////////////////////////////////////////////
	{
		g_rhi->BeginDebugEvent("[9]. FinalResultBlending");

		auto EnableClear = true;
		auto ClearColor = Vector4(0.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("LightVolumeFinal");

		g_rhi->SetRenderTarget(nullptr);

		EnableClear = false;
		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(false);
		g_rhi->EnableBlend(false);
		g_rhi->EnableDepthBias(false);

		g_rhi->SetShader(Shader);

		auto LinearSamplerPtr = jSamplerStatePool::GetSamplerState("LinearClamp");
		FullScreenQuad->SetTexture(MainSceneRT->GetTexture(), LinearSamplerPtr.get());
		FullScreenQuad->SetTexture2(ImageBlurRT->GetTexture(), LinearSamplerPtr.get());
		FullScreenQuad->Draw(MainCamera, Shader, { });

		g_rhi->SetRenderTarget(nullptr);
		g_rhi->EndDebugEvent();
	}
	//////////////////////////////////////////////////////////////////////////

	return;
	//////////////////////////////////////////////////////////////////////////
	// Debug Textures
	const Vector2 PreviewSize(300, 300);
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

	PreviewUI->Pos = Vector2(SCR_WIDTH - PreviewSize.x * 3, SCR_HEIGHT - PreviewSize.y);
	PREVIEW_TEXTURE(DirectionalLight->GetTextureDepth());

	PreviewUI->Pos.x += PreviewSize.x;
	PREVIEW_TEXTURE(ShadowMapWorldRT->GetTexture());
}

void jGame::UpdateAppSetting()
{
	auto& appSetting =  jShadowAppSettingProperties::GetInstance();

	appSetting.SpotLightDirection = Matrix::MakeRotateY(0.01).Transform(appSetting.SpotLightDirection);

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

			//SpawnObjects(ESpawnedType::Hair);
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

			//if (appSetting.ShadowMapType == EShadowMapType::CSM_SSM)
			//	SpawnObjects(ESpawnedType::CubePrimitive);
			//else
			//	SpawnObjects(ESpawnedType::TestPrimitive);
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

		//SpawnObjects(ESpawnedType::TestPrimitive);
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
		if (abs(xOffset))
			MainCamera->RotateYAxis(xOffset * -0.005f);
		if (abs(yOffset))
			MainCamera->RotateRightAxis(yOffset * -0.005f);
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

	auto quad = jPrimitiveUtil::CreateQuad(Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), Vector(1000.0f, 1000.0f, 1000.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	quad->SetPlane(jPlane(Vector(0.0, 1.0, 0.0), -0.1f));
	//quad->SkipShadowMapGen = true;
	quad->SkipUpdateShadowVolume = true;
	jObject::AddObject(quad);
	SpawnedObjects.push_back(quad);

	auto gizmo = jPrimitiveUtil::CreateGizmo(Vector::ZeroVector, Vector::ZeroVector, Vector::OneVector);
	gizmo->SkipShadowMapGen = true;
	jObject::AddObject(gizmo);
	SpawnedObjects.push_back(gizmo);

	auto triangle = jPrimitiveUtil::CreateTriangle(Vector(60.0, 100.0, 20.0), Vector::OneVector, Vector(40.0, 40.0, 40.0), Vector4(0.5f, 0.1f, 1.0f, 1.0f));
	triangle->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.x += 0.05f;
	};
	jObject::AddObject(triangle);
	SpawnedObjects.push_back(triangle);

	auto cube = jPrimitiveUtil::CreateCube(Vector(-60.0f, 55.0f, -20.0f), Vector::OneVector, Vector(50.0f, 50.0f, 50.0f), Vector4(0.7f, 0.7f, 0.7f, 1.0f));
	cube->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.z += 0.005f;
	};
	jObject::AddObject(cube);
	SpawnedObjects.push_back(cube);

	auto cube2 = jPrimitiveUtil::CreateCube(Vector(-65.0f, 35.0f, 10.0f), Vector::OneVector, Vector(50.0f, 50.0f, 50.0f), Vector4(0.7f, 0.7f, 0.7f, 1.0f));
	jObject::AddObject(cube2);
	SpawnedObjects.push_back(cube2);

	auto capsule = jPrimitiveUtil::CreateCapsule(Vector(30.0f, 30.0f, -80.0f), 40.0f, 10.0f, 20, Vector(1.0f), Vector4(1.0f, 1.0f, 0.0f, 1.0f));
	capsule->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.x -= 0.01f;
	};
	jObject::AddObject(capsule);
	SpawnedObjects.push_back(capsule);

	auto cone = jPrimitiveUtil::CreateCone(Vector(0.0f, 50.0f, 60.0f), 40.0f, 20.0f, 15, Vector::OneVector, Vector4(1.0f, 1.0f, 0.0f, 1.0f));
	cone->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.y += 0.03f;
	};
	jObject::AddObject(cone);
	SpawnedObjects.push_back(cone);

	auto cylinder = jPrimitiveUtil::CreateCylinder(Vector(-30.0f, 60.0f, -60.0f), 20.0f, 10.0f, 20, Vector::OneVector, Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	cylinder->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.x += 0.05f;
	};
	jObject::AddObject(cylinder);
	SpawnedObjects.push_back(cylinder);

	auto quad2 = jPrimitiveUtil::CreateQuad(Vector(-20.0f, 80.0f, 40.0f), Vector::OneVector, Vector(20.0f, 20.0f, 20.0f), Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	quad2->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->Rot.z += 0.08f;
	};
	jObject::AddObject(quad2);
	SpawnedObjects.push_back(quad2);

	auto sphere = jPrimitiveUtil::CreateSphere(Vector(65.0f, 35.0f, 10.0f), 1.0, 16, Vector(30.0f), Vector4(0.8f, 0.0f, 0.0f, 1.0f));
	sphere->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		//thisObject->RenderObject->Rot.z += 0.01f;
		thisObject->RenderObject->Rot.z = DegreeToRadian(180.0f);
	};
	Sphere = sphere;

	jObject::AddObject(sphere);
	SpawnedObjects.push_back(sphere);

	auto sphere2 = jPrimitiveUtil::CreateSphere(Vector(150.0f, 5.0f, 0.0f), 1.0, 16, Vector(10.0f), Vector4(0.8f, 0.4f, 0.6f, 1.0f));
	sphere2->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
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
	Sphere = sphere2;
	jObject::AddObject(sphere2);
	SpawnedObjects.push_back(sphere2);

	auto billboard = jPrimitiveUtil::CreateBillobardQuad(Vector(0.0f, 60.0f, 80.0f), Vector::OneVector, Vector(20.0f, 20.0f, 20.0f), Vector4(1.0f, 0.0f, 1.0f, 1.0f), MainCamera);
	jObject::AddObject(billboard);
	SpawnedObjects.push_back(billboard);
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
				PerspectiveVector[i].z = (PerspectiveVector[i].z + 1.0) * 0.5;
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
				OrthographicVector[i].z = (OrthographicVector[i].z + 1.0) * 0.5;
		}
	}
	std::vector<Vector2> graph1;
	std::vector<Vector2> graph2;

	float scale = 100.0f;
	for (int i = 0; i < _countof(PerspectiveVector); ++i)
		graph1.push_back(Vector2(i*2, PerspectiveVector[i].z * scale));
	for (int i = 0; i < _countof(OrthographicVector); ++i)
		graph2.push_back(Vector2(i*2, OrthographicVector[i].z* scale));

	auto graphObj1 = jPrimitiveUtil::CreateGraph2D({ 360, 350 }, {360, 300}, graph1);
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
