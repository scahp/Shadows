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
#include <time.h>
#include <stdlib.h>
#include "jSamplerStatePool.h"
#include <regex>

jRHI* g_rhi = nullptr;

jGame::jGame()
{
	g_rhi = new jRHI_OpenGL();
}

jGame::~jGame()
{
}

void jGame::ProcessInput()
{
	static float speed = 1.0f;

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
	srand(static_cast<uint32>(time(NULL)));

	auto& AppSettings = jShadowAppSettingProperties::GetInstance();

	// Create main camera
	const Vector mainCameraPos(-0.725829422f, 289.250427f, 1.61028826f);
	const Vector mainCameraTarget(-0.713820636f, 207.391312f, -3.19371367f);
	MainCamera = jCamera::CreateCamera(mainCameraPos, mainCameraTarget, mainCameraPos + Vector(0.0, -1.0, 0.0), DegreeToRadian(45.0f), 10.0f, 1000.0f, SCR_WIDTH, SCR_HEIGHT, true);
	jCamera::AddCamera(0, MainCamera);

	// Create lights
	NormalDirectionalLight = jLight::CreateDirectionalLight(AppSettings.DirecionalLightDirection
		, Vector4(0.6f), Vector(1.0f), Vector(1.0f), 64);
	CascadeDirectionalLight = jLight::CreateCascadeDirectionalLight(AppSettings.DirecionalLightDirection
		, Vector4(0.6f), Vector(1.0f), Vector(1.0f), 64);
	AmbientLight = jLight::CreateAmbientLight(Vector(0.2f, 0.5f, 1.0f), Vector(0.05f));		// sky light color
	PointLight = jLight::CreatePointLight(jShadowAppSettingProperties::GetInstance().PointLightPosition, Vector4(2.0f, 0.7f, 0.7f, 1.0f), 500.0f, Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), 64.0f);
	SpotLight = jLight::CreateSpotLight(jShadowAppSettingProperties::GetInstance().SpotLightPosition, jShadowAppSettingProperties::GetInstance().SpotLightDirection, Vector4(0.0f, 1.0f, 0.0f, 1.0f), 500.0f, 0.7f, 1.0f, Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), 64.0f);

	// Select one of directional light
	DirectionalLight = NormalDirectionalLight;

	// Create light info for debugging light infomation
	if (DirectionalLight)
	{
		DirectionalLightInfo = jPrimitiveUtil::CreateDirectionalLightDebug(Vector(250, 260, 0) * 0.4f, Vector::OneVector * 10.0f, 10.0f, MainCamera, DirectionalLight, "Image/sun.png");
		if (AppSettings.ShowDirectionalLightInfo)
			jObject::AddDebugObject(DirectionalLightInfo);
	}

	if (PointLight)
	{
		PointLightInfo = jPrimitiveUtil::CreatePointLightDebug(Vector(10.0f), MainCamera, PointLight, "Image/bulb.png");
		if (AppSettings.ShowPointLightInfo)
			jObject::AddDebugObject(PointLightInfo);
	}

	if (SpotLight)
	{
		SpotLightInfo = jPrimitiveUtil::CreateSpotLightDebug(Vector(10.0f), MainCamera, SpotLight, "Image/spot.png");
		if (AppSettings.ShowSpotLightInfo)
			jObject::AddDebugObject(SpotLightInfo);
	}

	// Main camera is linked with lights which will be used.
	if (DirectionalLight)
		MainCamera->AddLight(DirectionalLight);
	if (PointLight)
		MainCamera->AddLight(PointLight);
	if (SpotLight)
		MainCamera->AddLight(SpotLight);
	MainCamera->AddLight(AmbientLight);

	// Create UI primitive to visualize shadowmap for debugging
	DirectionalLightShadowMapUIDebug = jPrimitiveUtil::CreateUIQuad({ 0.0f, 0.0f }, { 150, 150 }, DirectionalLight->GetShadowMap());
	if (DirectionalLightShadowMapUIDebug)
		jObject::AddUIDebugObject(DirectionalLightShadowMapUIDebug);

	// Select spawning object type
	SpawnObjects(ESpawnedType::TestPrimitive);

	// Prepare pipeline which have rendering passes.
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

	ShadowVolumePipelineSet = CREATE_PIPELINE_SET_WITH_SETUP(jForwardPipelineSet_ShadowVolume);

	CurrentShadowMapType = jShadowAppSettingProperties::GetInstance().ShadowMapType;

	// Prepare renderer which are both forward and deferred. It can be switched depend on shadow type
	const auto currentShadowPipelineSet = (jShadowAppSettingProperties::GetInstance().ShadowType == EShadowType::ShadowMap) 
		? ShadowPipelineSetMap[CurrentShadowMapType]  : ShadowVolumePipelineSet;
	ForwardRenderer = new jForwardRenderer(currentShadowPipelineSet);
	ForwardRenderer->Setup();

	DeferredRenderer = new jDeferredRenderer({ ETextureType::TEXTURE_2D, ETextureFormat::RGBA32F, ETextureFormat::RGBA, EFormatType::FLOAT, EDepthBufferType::DEPTH16, SCR_WIDTH, SCR_HEIGHT, 4 });
	DeferredRenderer->Setup();
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

	// Update application property by using UI Pannel.
	UpdateAppSetting();

	// Update main camera
	MainCamera->UpdateCamera();

	// Update lights
	const int32 numOfLights = MainCamera->GetNumOfLight();
	for (int32 i = 0; i < numOfLights; ++i)
	{
		auto light = MainCamera->GetLight(i);
		JASSERT(light);
		light->Update(deltaTime);
	}

	for (auto iter : jObject::GetStaticObject())
		iter->Update(deltaTime);

	for (auto& iter : jObject::GetBoundBoxObject())
		iter->Update(deltaTime);

	for (auto& iter : jObject::GetBoundSphereObject())
		iter->Update(deltaTime);

	for (auto& iter : jObject::GetDebugObject())
		iter->Update(deltaTime);

	// Update object which have dirty flag
	jObject::FlushDirtyState();

	//// Render all objects by using selected renderer
	//Renderer->Render(MainCamera);

	auto& appSetting = jShadowAppSettingProperties::GetInstance();

	static bool s_Initialized = false;
	static std::weak_ptr<jTexture> ColorTexture;
	static std::weak_ptr<jTexture> ReliefTexture;
	static std::weak_ptr<jTexture> NormalTexture;
	static jObject* Cube = nullptr;
	static jObject* Quad = nullptr;
	static std::weak_ptr<jTexture> DualDepthRelief_ColorTexture;
	static std::weak_ptr<jTexture> DualDepthRelief_DepthTexture;
	static std::shared_ptr<jRenderTarget> RT;
	static jObject* FullQuad = nullptr;
	if (!s_Initialized)
	{
		s_Initialized = true;

		ColorTexture = jImageFileLoader::GetInstance().LoadTextureFromFile(jName("Image/tile1_color.jpg"));
		ReliefTexture = jImageFileLoader::GetInstance().LoadTextureFromFile(jName("Image/tile1_relief.tga"));
		//NormalTexture = jImageFileLoader::GetInstance().LoadTextureFromFile(jName("Image/tile1_relief.tga"));
		
		//NormalTexture = jImageFileLoader::GetInstance().LoadTextureFromFile(jName("Image/rockbump_relief.tga"));
		//ColorTexture = jImageFileLoader::GetInstance().LoadTextureFromFile(jName("Image/rockbump_color.jpg"));
		

		//ColorTexture = jImageFileLoader::GetInstance().LoadTextureFromFile("Image/brick_diffuse.png");
		//NormalTexture = jImageFileLoader::GetInstance().LoadTextureFromFile("Image/brick_normal.png");

		static const auto LinearClampSamplerState = jSamplerStatePool::GetSamplerState(jName("LinearClamp"));

		const float Width = 100.0f;
		Cube = jPrimitiveUtil::CreateCube(Vector(0.0f, 0.0f, 0.0f), Vector::OneVector, Vector(Width, Width, Width), Vector4(0.0f, 0.0f, 1.0f, 0.5f));
		Cube->RenderObject->SetTexture(0, jName("ColorTexture"), ColorTexture.lock().get(), LinearClampSamplerState.get());
		Cube->RenderObject->SetTexture(1, jName("ReliefTexture"), ReliefTexture.lock().get(), LinearClampSamplerState.get());
		Cube->RenderObject->SetTexture(2, jName("NormalTexture"), NormalTexture.lock().get(), LinearClampSamplerState.get());

		//DualDepthRelief_ColorTexture = jImageFileLoader::GetInstance().LoadTextureFromFile(jName("Image/DualDepthReliefTexture/supermarket4x4x2a_CaptureFG.HDR"));
		//DualDepthRelief_DepthTexture = jImageFileLoader::GetInstance().LoadTextureFromFile(jName("Image/DualDepthReliefTexture/supermarket4x4x2a_DepthFG.TGA"));
		//UpdateReliefTexture();

		Quad = jPrimitiveUtil::CreateQuad(Vector(0.0f, 0.0f, 0.0f), Vector::OneVector, Vector(Width), Vector4(0.0f, 1.0f, 0.0f, 0.5f));
		Quad->RenderObject->SetTexture(0, jName("ColorTexture"), DualDepthRelief_ColorTexture.lock().get(), LinearClampSamplerState.get());
		Quad->RenderObject->SetTexture(1, jName("ReliefTexture"), DualDepthRelief_DepthTexture.lock().get(), LinearClampSamplerState.get());
		Quad->RenderObject->SetTexture(2, jName("NormalTexture"), NormalTexture.lock().get(), LinearClampSamplerState.get());

		//Quad->RenderObject->SetRot({ 0.0f, 0.0f, -DegreeToRadian(90.0f) });
        //Quad->RenderObject->SetRot({ -DegreeToRadian(90.0f), 0.0f, 0.0f });

		RT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget(
			{ ETextureType::TEXTURE_2D, ETextureFormat::RGBA8, ETextureFormat::RGBA, EFormatType::FLOAT, EDepthBufferType::DEPTH24, SCR_WIDTH, SCR_HEIGHT, 1 }));

		FullQuad = jPrimitiveUtil::CreateFullscreenQuad(RT->GetTexture());

		g_rhi->SetLineWidth(2.5f);		// Boudnbox debugging
	}

	static auto SelectedTexture = EDualDepthReliefTexture::MAX;
	auto UpdateReliefTexture = [&]()
	{
		std::string color = "Image/DualDepthReliefTexture/" + std::string(EDualDepthReliefTextureString[(int32)SelectedTexture]) + ".HDR";
		std::string dualdepth = std::regex_replace(color, std::regex("_Capture"), "_Depth");
		dualdepth = std::regex_replace(dualdepth, std::regex(".HDR"), ".TGA");

		DualDepthRelief_ColorTexture = jImageFileLoader::GetInstance().LoadTextureFromFile(jName(color.c_str()));
		DualDepthRelief_DepthTexture = jImageFileLoader::GetInstance().LoadTextureFromFile(jName(dualdepth.c_str()));

		static const auto LinearClampSamplerState = jSamplerStatePool::GetSamplerState(jName("LinearClamp"));
		Quad->RenderObject->SetTexture(0, jName("ColorTexture"), DualDepthRelief_ColorTexture.lock().get(), LinearClampSamplerState.get());
		Quad->RenderObject->SetTexture(1, jName("ReliefTexture"), DualDepthRelief_DepthTexture.lock().get(), LinearClampSamplerState.get());
	};

	if (SelectedTexture != appSetting.ReliefTexture)
	{
		SelectedTexture = appSetting.ReliefTexture;
		UpdateReliefTexture();
	}

	const bool ShouldUseQuad = appSetting.ReliefType == EReliefTracingType::DualDepth;

	if (RT->Begin())
	{
		g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);
		g_rhi->EnableDepthTest(true);
		g_rhi->SetDepthFunc(EComparisonFunc::LEQUAL);
		g_rhi->EnableCullFace(true);
		g_rhi->EnableCullMode(ECullMode::BACK);
		//g_rhi->SetFrontFace(EFrontFace::CCW);

		MainCamera->IsEnableCullMode = true;

		if (!ShouldUseQuad)
		{
			jShader* shader = jShader::GetShader("ReliefMapping");
						
			//Cube->RenderObject->SetPos(Vector(10.0f, 100.0f, 100.0f));
			Cube->RenderObject->SetRot(Cube->RenderObject->GetRot() + Vector(0.35f * deltaTime, 0.0f, 0.5f * deltaTime));
			//Cube->RenderObject->SetRot(Vector(0.0f, DegreeToRadian(90), 0.0f));

			Cube->RenderObject->UpdateWorldMatrix();
			auto InvWorld = Cube->RenderObject->GetWorld().GetInverse();

			auto MV = MainCamera->View;
			auto MVP = MainCamera->Projection * MV;
			auto InvMPV = MVP.GetInverse();
			auto LocalCameraPos = InvMPV.TransformPoint(MainCamera->Pos);
			auto LocalLightDir = InvMPV.Transform(Vector4(DirectionalLight->Data.Direction, 0.0f));
			
			g_rhi->SetShader(shader);
			g_rhi->SetUniformbuffer(jName("WorldSpace_LightDir_ToSurface"), DirectionalLight->Data.Direction, shader);
			g_rhi->SetUniformbuffer(jName("WorldSpace_CameraPos"), MainCamera->Pos, shader);
			g_rhi->SetUniformbuffer(jName("ReliefTracingType"), (int32)appSetting.ReliefType, shader);
			g_rhi->SetUniformbuffer(jName("DepthBias"), appSetting.DepthBias, shader);
			g_rhi->SetUniformbuffer(jName("DepthScale"), appSetting.DepthScale, shader);
			g_rhi->SetUniformbuffer(jName("UseShadow"), appSetting.ReliefShadowOn, shader);

			g_rhi->SetUniformbuffer(jName("LocalSpace_LightDir_ToSurface"), InvWorld.TransformDirection(DirectionalLight->Data.Direction).GetNormalize(), shader);
			g_rhi->SetUniformbuffer(jName("LocalSpace_CameraPos"), InvWorld.TransformPoint(MainCamera->Pos), shader);

			Cube->Update(deltaTime);
			Cube->Draw(MainCamera, shader, { DirectionalLight });

			if (appSetting.ShowBoundBox)
			{
				jShader* shader = jShader::GetShader("BoundVolumeShader");
				g_rhi->SetShader(shader);
				SET_UNIFORM_BUFFER_STATIC("Color", Vector4::ColorWhite, shader);
				Cube->BoundBoxObject->Draw(MainCamera, shader, {});
			}
		}

		if (ShouldUseQuad)
		{
			auto shader = jShader::GetShader("DualDepthReliefMapping");

			//Quad->RenderObject->SetRot(Vector(0.0f, 0.0f, DegreeToRadian(90)));
			//Quad->RenderObject->SetRot(Quad->RenderObject->GetRot() + Vector(0.0f, 0.01f, 0.0f));
			Quad->RenderObject->UpdateWorldMatrix();
			auto InvWorld = Quad->RenderObject->GetWorld().GetInverse();

			auto MV = MainCamera->View;
			auto MVP = MainCamera->Projection * MV;
			auto InvMPV = MVP.GetInverse();
			auto LocalCameraPos = InvMPV.TransformPoint(MainCamera->Pos);
			auto LocalLightDir = InvMPV.Transform(Vector4(DirectionalLight->Data.Direction, 0.0f));

			g_rhi->SetShader(shader);
			g_rhi->SetUniformbuffer(jName("WorldSpace_LightDir_ToSurface"), DirectionalLight->Data.Direction, shader);
			g_rhi->SetUniformbuffer(jName("WorldSpace_CameraPos"), MainCamera->Pos, shader);
			g_rhi->SetUniformbuffer(jName("ReliefTracingType"), (int32)appSetting.ReliefType, shader);
			g_rhi->SetUniformbuffer(jName("DepthBias"), appSetting.DualDepth_DepthBias, shader);
			g_rhi->SetUniformbuffer(jName("DepthScale"), appSetting.DualDepth_DepthScale, shader);
			g_rhi->SetUniformbuffer(jName("UseShadow"), appSetting.ReliefShadowOn, shader);

			g_rhi->SetUniformbuffer(jName("LocalSpace_LightDir_ToSurface"), InvWorld.TransformDirection(DirectionalLight->Data.Direction), shader);
            g_rhi->SetUniformbuffer(jName("LocalSpace_CameraPos"), InvWorld.TransformPoint(MainCamera->Pos), shader);

			Quad->Update(deltaTime);
			Quad->Draw(MainCamera, shader, { DirectionalLight });

			if (appSetting.ShowBoundBox)
			{
				jShader* shader = jShader::GetShader("BoundVolumeShader");
				g_rhi->SetShader(shader);
				SET_UNIFORM_BUFFER_STATIC("Color", Vector4::ColorWhite, shader);
				Quad->BoundBoxObject->Draw(MainCamera, shader, {});
			}
		}

		{
			jShader* shader = jShader::GetShader("DebugObjectShader");
			for (auto& iter : jObject::GetDebugObject())
				iter->Draw(MainCamera, shader, {});
		}

		RT->End();
	}

	{
		g_rhi->SetClear(ERenderBufferType::COLOR);
		g_rhi->EnableDepthTest(false);

		jShader* shader = jShader::GetShader("Scale");
		FullQuad->Update(deltaTime);
		FullQuad->Draw(MainCamera, shader, {});
	}
}

void jGame::UpdateAppSetting()
{
	auto& appSetting =  jShadowAppSettingProperties::GetInstance();

	// Rotating spot light
	appSetting.SpotLightDirection = Matrix::MakeRotateY(0.01f).TransformDirection(appSetting.SpotLightDirection);

	// Chaning normal or cascade shadow map depend on shadow type.
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
		// Update directional light info by using selected shadowmap
		if (DirectionalLightInfo)
			((jDirectionalLightPrimitive*)(DirectionalLightInfo))->Light = DirectionalLight;

		// Update debugging shadow map of directional light
		const auto shaderMapRenderTarget = DirectionalLight->ShadowMapData->ShadowMapRenderTarget;
		const float aspect = static_cast<float>(shaderMapRenderTarget->Info.Height) / shaderMapRenderTarget->Info.Width;
		if (DirectionalLightShadowMapUIDebug)
		{
			DirectionalLightShadowMapUIDebug->SetTexture(DirectionalLight->ShadowMapData->ShadowMapRenderTarget->GetTexture());
			DirectionalLightShadowMapUIDebug->Size.y = DirectionalLightShadowMapUIDebug->Size.x * aspect;
		}
	}

	bool isChangedShadowMode = false;

	// Chaning ShadowType or ShadowMapType
	//  - Spawn object depending on new type
	//  - Switch pipeline set and renderer depending on new type
	const bool isChangedShadowType = CurrentShadowType != appSetting.ShadowType;
	const bool isChangedShadowMapType = (CurrentShadowMapType != appSetting.ShadowMapType);
	if (appSetting.ShadowType == EShadowType::ShadowMap)
	{
		if (appSetting.ShadowMapType == EShadowMapType::DeepShadowMap_DirectionalLight)
		{
			Renderer = DeferredRenderer;
			appSetting.ShowPointLightInfo = false;
			appSetting.ShowSpotLightInfo = false;

			SpawnObjects(ESpawnedType::Hair);
			isChangedShadowMode = true;
		}
		else
		{
			Renderer = ForwardRenderer;

			const bool isChangedPoisson = (UsePoissonSample != appSetting.UsePoissonSample);
			if (isChangedShadowType || isChangedPoisson || isChangedShadowMapType)
			{
				CurrentShadowType = appSetting.ShadowType;
				UsePoissonSample = appSetting.UsePoissonSample;
				CurrentShadowMapType = appSetting.ShadowMapType;
				auto newPipelineSet = UsePoissonSample ? ShadowPoissonSamplePipelineSetMap[CurrentShadowMapType] : ShadowPipelineSetMap[CurrentShadowMapType];
				Renderer->SetChangePipelineSet(newPipelineSet);
				isChangedShadowMode = true;
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
			isChangedShadowMode = true;
		}

		SpawnObjects(ESpawnedType::TestPrimitive);
		MainCamera->IsInfinityFar = true;
	}

	// Update Light's MaterialData to cache it again
	if (isChangedShadowMode)
	{
		for (int32 i = 0; i < MainCamera->GetNumOfLight(); ++i)
		{
			jLight* light = MainCamera->GetLight(i);
			if (light)
			{
				light->DirtyMaterialData = true;
			}
		}
	}

	// Update visibility of sub menu in UI Panel
	if (isChangedShadowType)
		appSetting.SwitchShadowType(jAppSettings::GetInstance().Get("MainPannel"));
	if (isChangedShadowMapType)
		appSetting.SwitchShadowMapType(jAppSettings::GetInstance().Get("MainPannel"));

	// Switching visibility of light info by using UI panel
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
	
	// Update light direction and position by using UI Panel.
	if (DirectionalLight)
		DirectionalLight->Data.Direction = appSetting.DirecionalLightDirection;
	if (PointLight)
		PointLight->Data.Position = appSetting.PointLightPosition;
	if (SpotLight)
	{
		SpotLight->Data.Direction = appSetting.SpotLightDirection;
		SpotLight->Data.Position = appSetting.SpotLightPosition;
	}

	// Update visibility of shadowmap debuging by using UI Panel
	if (DirectionalLightShadowMapUIDebug)
		DirectionalLightShadowMapUIDebug->Visible = appSetting.ShowDirectionalLightMap;
}

void jGame::OnMouseButton()
{

}

void jGame::OnMouseMove(int32 xOffset, int32 yOffset)
{
	if (g_MouseState[EMouseButtonType::LEFT])
	{
		if (abs(xOffset))
			MainCamera->RotateUpAxis(xOffset * -0.005f);
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
	jObject::AddObject(hairObject);
	SpawnedObjects.push_back(hairObject);

	auto headModel = jModelLoader::GetInstance().LoadFromFile("Model/woman.x");
	jObject::AddObject(headModel);
	SpawnedObjects.push_back(headModel);
}

void jGame::SpawnTestPrimitives()
{
	RemoveSpawnedObjects();

	auto quad = jPrimitiveUtil::CreateQuad(Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), Vector(1000.0f, 1000.0f, 1000.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f));
	quad->SetPlane(jPlane(Vector(0.0, 1.0, 0.0), -0.1f));
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
		thisObject->RenderObject->SetRot(thisObject->RenderObject->GetRot() + Vector(0.05f, 0.0f, 0.0f));
	};
	jObject::AddObject(triangle);
	SpawnedObjects.push_back(triangle);

	auto cube = jPrimitiveUtil::CreateCube(Vector(-60.0f, 55.0f, -20.0f), Vector::OneVector, Vector(50.0f, 50.0f, 50.0f), Vector4(0.7f, 0.7f, 0.7f, 1.0f));
	cube->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->SetRot(thisObject->RenderObject->GetRot() + Vector(0.0f, 0.0f, 0.005f));
	};
	jObject::AddObject(cube);
	SpawnedObjects.push_back(cube);

	auto cube2 = jPrimitiveUtil::CreateCube(Vector(-65.0f, 35.0f, 10.0f), Vector::OneVector, Vector(50.0f, 50.0f, 50.0f), Vector4(0.7f, 0.7f, 0.7f, 1.0f));
	jObject::AddObject(cube2);
	SpawnedObjects.push_back(cube2);

	auto capsule = jPrimitiveUtil::CreateCapsule(Vector(30.0f, 30.0f, -80.0f), 40.0f, 10.0f, 20, Vector(1.0f), Vector4(1.0f, 1.0f, 0.0f, 1.0f));
	capsule->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->SetRot(thisObject->RenderObject->GetRot() + Vector(-0.01f, 0.0f, 0.0f));
	};
	jObject::AddObject(capsule);
	SpawnedObjects.push_back(capsule);

	auto cone = jPrimitiveUtil::CreateCone(Vector(0.0f, 50.0f, 60.0f), 40.0f, 20.0f, 15, Vector::OneVector, Vector4(1.0f, 1.0f, 0.0f, 1.0f));
	cone->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->SetRot(thisObject->RenderObject->GetRot() + Vector(0.0f, 0.03f, 0.0f));
	};
	jObject::AddObject(cone);
	SpawnedObjects.push_back(cone);

	auto cylinder = jPrimitiveUtil::CreateCylinder(Vector(-30.0f, 60.0f, -60.0f), 20.0f, 10.0f, 20, Vector::OneVector, Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	cylinder->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->SetRot(thisObject->RenderObject->GetRot() + Vector(0.05f, 0.0f, 0.0f));
	};
	jObject::AddObject(cylinder);
	SpawnedObjects.push_back(cylinder);

	auto quad2 = jPrimitiveUtil::CreateQuad(Vector(-20.0f, 80.0f, 40.0f), Vector::OneVector, Vector(20.0f, 20.0f, 20.0f), Vector4(0.0f, 0.0f, 1.0f, 1.0f));
	quad2->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->SetRot(thisObject->RenderObject->GetRot() + Vector(0.0f, 0.0f, 0.08f));
	};
	jObject::AddObject(quad2);
	SpawnedObjects.push_back(quad2);

	auto sphere = jPrimitiveUtil::CreateSphere(Vector(65.0f, 35.0f, 10.0f), 1.0, 16, Vector(30.0f), Vector4(0.8f, 0.0f, 0.0f, 1.0f));
	sphere->PostUpdateFunc = [](jObject* thisObject, float deltaTime)
	{
		thisObject->RenderObject->SetRot(thisObject->RenderObject->GetRot() + Vector(0.0f, 0.0f, DegreeToRadian(180.0f)));
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
		auto Pos = thisObject->RenderObject->GetPos();
		Pos.y += dir ? speed : -speed;
		if (Pos.y < startY || Pos.y > endY)
		{
			dir = !dir;
			Pos.y += dir ? speed : -speed;
		}
		thisObject->RenderObject->SetPos(Pos);
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
				PerspectiveVector[cnt++] = MV.TransformPoint(Vector({ 0.0f, 0.0f, 10.0f + static_cast<float>(i) }));
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
				OrthographicVector[cnt++] = MV.TransformPoint(Vector({ 0.0f, 0.0f, 10.0f + static_cast<float>(i) }));
			}

			for (int i = 0; i < _countof(OrthographicVector); ++i)
				OrthographicVector[i].z = (OrthographicVector[i].z + 1.0f) * 0.5f;
		}
	}
	std::vector<Vector2> graph1;
	std::vector<Vector2> graph2;

	float scale = 100.0f;
	for (int i = 0; i < _countof(PerspectiveVector); ++i)
		graph1.push_back(Vector2(static_cast<float>(i*2), PerspectiveVector[i].z * scale));
	for (int i = 0; i < _countof(OrthographicVector); ++i)
		graph2.push_back(Vector2(static_cast<float>(i*2), OrthographicVector[i].z* scale));

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
