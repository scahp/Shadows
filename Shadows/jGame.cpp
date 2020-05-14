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

jRHI* g_rhi = nullptr;
static jMeshObject* headModel = nullptr;
static jTexture* headModelWorldNormalTexture = nullptr;

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
	//////////////////////////////////////////////////////////////////////////
	const Vector mainCameraPos(0.0f, 0.0f, 150.63f);
	//const Vector mainCameraTarget(171.96f, 166.02f, -180.05f);
	//const Vector mainCameraPos(165.0f, 125.0f, -136.0f);
	//const Vector mainCameraPos(300.0f, 100.0f, 300.0f);
	const Vector mainCameraTarget(0.0f, -20.0f, 0.0f);
	MainCamera = jCamera::CreateCamera(mainCameraPos, mainCameraTarget, mainCameraPos + Vector(0.0, 1.0, 0.0), DegreeToRadian(45.0f), 10.0f, 1000.0f, SCR_WIDTH, SCR_HEIGHT, true);
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
	//jObject::AddDebugObject(PointLightInfo);

	//SpotLightInfo = jPrimitiveUtil::CreateSpotLightDebug(Vector(10.0f), MainCamera, SpotLight, "Image/spot.png");
	//jObject::AddDebugObject(SpotLightInfo);

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
}

void jGame::SpawnObjects(ESpawnedType spawnType)
{
	//if (spawnType != SpawnedType)
	//{
	//	SpawnedType = spawnType;
	//	switch (SpawnedType)
	//	{
	//	case ESpawnedType::Hair:
	//		SpawnHairObjects();
	//		break;
	//	case ESpawnedType::TestPrimitive:
	//		SpawnTestPrimitives();
	//		break;
	//	case ESpawnedType::CubePrimitive:
	//		SapwnCubePrimitives();
	//		break;
	//	}
	//}

	if (!headModel)
	{
		headModel = jModelLoader::GetInstance().LoadFromFile("SkinData/Infinite-Level_02.OBJ");
		//g_StaticObjectArray.push_back(headModel);
		headModel->RenderObject->Scale = Vector(300);
		{
			jImageData data;
			std::string temp = "SkinData/Infinite-Level_02_World_NoSmoothUV.jpg";
			jImageFileLoader::GetInstance().LoadTextureFromFile(data, temp.c_str(), true);
			//auto newMeshMaterial = new jMeshMaterial();
			headModelWorldNormalTexture = g_rhi->CreateTextureFromData(&data.ImageData[0], data.Width, data.Height, data.sRGB);
			//newMeshMaterial->TextureName = temp.c_str();

			headModel->RenderObject->tex_object[2] = headModelWorldNormalTexture;
		}
		jObject::AddObject(headModel);
		SpawnedObjects.push_back(headModel);
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

	jObject::FlushDirtyState();

	//Renderer->Render(MainCamera);
	Renderer->ShadowPrePass(MainCamera);

	jRenderTargetInfo info;
	info.TextureType = ETextureType::TEXTURE_2D;
	info.InternalFormat = ETextureFormat::RGBA32F;
	info.Format = ETextureFormat::RGBA;
	info.FormatType = EFormatType::FLOAT;
	info.DepthBufferType = EDepthBufferType::NONE;
	info.Width = 4096;
	info.Height = 4096;
	info.TextureCount = 1;
	info.Magnification = ETextureFilter::NEAREST;
	info.Minification = ETextureFilter::NEAREST;

	info.DepthBufferType = EDepthBufferType::DEPTH32;
	static auto TSMTarget = jRenderTargetPool::GetRenderTarget(info);
	if (TSMTarget->Begin())
	{
		auto ClearColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);	// blank space color
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("SkinTSMGen");
		auto EnableClear = true;
		bool EnableDepthBias = false;
		float DepthSlopeBias = 1.0f;
		float DepthConstantBias = 1.0f;

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->SetDepthFunc(DepthStencilFunc);

		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);

		g_rhi->EnableDepthBias(EnableDepthBias);
		g_rhi->SetDepthBias(DepthConstantBias, DepthSlopeBias);

		g_rhi->SetShader(Shader);

		std::list<const jLight*> lights;
		lights.insert(lights.end(), MainCamera->LightList.begin(), MainCamera->LightList.end());

		DirectionalLight->GetLightCamra()->BindCamera(Shader);
		jLight::BindLights(lights, Shader);

		headModel->Draw(DirectionalLight->GetLightCamra(), Shader, lights);

		TSMTarget->End();
	}

	info.InternalFormat = ETextureFormat::RGBA;
	info.Format = ETextureFormat::RGBA;
	info.FormatType = EFormatType::BYTE;
	info.DepthBufferType = EDepthBufferType::NONE;

	static auto IrrBlurTemp = jRenderTargetPool::GetRenderTarget(info);

	static auto IrrTarget = jRenderTargetPool::GetRenderTarget(info);
	static auto IrrBlurTarget2 = jRenderTargetPool::GetRenderTarget(info);
	static auto IrrBlurTarget4 = jRenderTargetPool::GetRenderTarget(info);
	static auto IrrBlurTarget8 = jRenderTargetPool::GetRenderTarget(info);
	static auto IrrBlurTarget16 = jRenderTargetPool::GetRenderTarget(info);
	static auto IrrBlurTarget32 = jRenderTargetPool::GetRenderTarget(info);

	if (IrrTarget->Begin())
	{
		auto ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);	// blank space color
		auto ClearType = ERenderBufferType::COLOR;
		auto EnableDepthTest = false;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("SkinIrrGen");
		auto EnableClear = true;
		bool EnableDepthBias = false;
		float DepthSlopeBias = 1.0f;
		float DepthConstantBias = 1.0f;

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->SetDepthFunc(DepthStencilFunc);

		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);

		g_rhi->EnableDepthBias(EnableDepthBias);
		g_rhi->SetDepthBias(DepthConstantBias, DepthSlopeBias);

		g_rhi->SetShader(Shader);

		std::list<const jLight*> lights;
		lights.insert(lights.end(), MainCamera->LightList.begin(), MainCamera->LightList.end());

		MainCamera->BindCamera(Shader);
		jLight::BindLights(lights, Shader);

		headModel->RenderObject->tex_object[3] = TSMTarget->GetTexture();
		headModel->Draw(MainCamera, Shader, lights);

		IrrTarget->End();
	}

	jRenderTargetInfo StretchTargetInfo;
	StretchTargetInfo.TextureType = ETextureType::TEXTURE_2D;
	StretchTargetInfo.InternalFormat = ETextureFormat::RG32F;
	StretchTargetInfo.Format = ETextureFormat::RG;
	StretchTargetInfo.FormatType = EFormatType::FLOAT;
	StretchTargetInfo.DepthBufferType = EDepthBufferType::NONE;
	StretchTargetInfo.Width = 4096;
	StretchTargetInfo.Height = 4096;
	StretchTargetInfo.TextureCount = 1;
	StretchTargetInfo.Magnification = ETextureFilter::NEAREST;
	StretchTargetInfo.Minification = ETextureFilter::NEAREST;
	static auto StrechTarget = jRenderTargetPool::GetRenderTarget(StretchTargetInfo);
	if (StrechTarget->Begin())
	{
		auto ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);	// blank space color
		auto ClearType = ERenderBufferType::COLOR;
		auto EnableDepthTest = false;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("SkinStrechGen");
		auto EnableClear = true;
		bool EnableDepthBias = false;
		float DepthSlopeBias = 1.0f;
		float DepthConstantBias = 1.0f;

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->SetDepthFunc(DepthStencilFunc);

		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);

		g_rhi->EnableDepthBias(EnableDepthBias);
		g_rhi->SetDepthBias(DepthConstantBias, DepthSlopeBias);

		g_rhi->SetShader(Shader);

		std::list<const jLight*> lights;
		lights.insert(lights.end(), MainCamera->LightList.begin(), MainCamera->LightList.end());

		MainCamera->BindCamera(Shader);
		jLight::BindLights(lights, Shader);

		headModel->Draw(MainCamera, Shader, lights);

		StrechTarget->End();
	}

	static jFullscreenQuadPrimitive* FullScreenQuad = jPrimitiveUtil::CreateFullscreenQuad(nullptr);
#define BLUR(RENDERTARGET, SRCTEXTURE, Scale, TextureSize)  \
	FullScreenQuad->RenderObject->tex_object[0] = SRCTEXTURE; \
	FullScreenQuad->RenderObject->tex_object[1] = StrechTarget->GetTexture();\
	if (IrrBlurTemp->Begin())\
	{\
		auto ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);\
		auto ClearType = ERenderBufferType::COLOR;\
		auto EnableClear = true;\
		auto EnableDepthTest = false;\
		auto DepthStencilFunc = EComparisonFunc::LESS;\
		auto EnableBlend = false;\
		auto BlendSrc = EBlendSrc::ONE;\
		auto BlendDest = EBlendDest::ZERO;\
		auto Shader = jShader::GetShader("SkinGaussianBlurX");\
		if (EnableClear)\
		{\
			g_rhi->SetClearColor(ClearColor);\
			g_rhi->SetClear(ClearType);\
		}\
		g_rhi->EnableDepthTest(false);\
		g_rhi->EnableBlend(EnableBlend);\
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);\
		g_rhi->SetShader(Shader);\
		g_rhi->SetUniformbuffer(&jUniformBuffer<float>("Scale", Scale), Shader);\
		g_rhi->SetUniformbuffer(&jUniformBuffer<float>("TextureSize", TextureSize), Shader);\
		MainCamera->BindCamera(Shader);\
		FullScreenQuad->Draw(MainCamera, Shader, {});\
		IrrBlurTemp->End();\
	}\
	FullScreenQuad->RenderObject->tex_object[0] = IrrBlurTemp->GetTexture();\
	if (RENDERTARGET->Begin())\
	{\
		auto ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);\
		auto ClearType = ERenderBufferType::COLOR;\
		auto EnableClear = true;\
		auto EnableDepthTest = false;\
		auto DepthStencilFunc = EComparisonFunc::LESS;\
		auto EnableBlend = false;\
		auto BlendSrc = EBlendSrc::ONE;\
		auto BlendDest = EBlendDest::ZERO;\
		auto Shader = jShader::GetShader("SkinGaussianBlurY");\
		if (EnableClear)\
		{\
			g_rhi->SetClearColor(ClearColor);\
			g_rhi->SetClear(ClearType);\
		}\
		g_rhi->EnableDepthTest(false);\
		g_rhi->EnableBlend(EnableBlend);\
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);\
		g_rhi->SetShader(Shader);\
		g_rhi->SetUniformbuffer(&jUniformBuffer<float>("Scale", Scale), Shader);\
		g_rhi->SetUniformbuffer(&jUniformBuffer<float>("TextureSize", TextureSize), Shader);\
		MainCamera->BindCamera(Shader);\
		FullScreenQuad->Draw(MainCamera, Shader, {});\
		RENDERTARGET->End();\
	}

	static auto IrrBlurTemp2 = jRenderTargetPool::GetRenderTarget(info);

	BLUR(IrrBlurTarget2, IrrTarget->GetTexture(), 2, 4096);   // 2
	BLUR(IrrBlurTarget4, IrrBlurTarget2->GetTexture(), 2, 4096);  // 4
	BLUR(IrrBlurTarget8, IrrBlurTarget4->GetTexture(), 4, 4096);  // 8
	BLUR(IrrBlurTarget16, IrrBlurTarget8->GetTexture(), 8, 4096); // 16
	BLUR(IrrBlurTarget32, IrrBlurTarget16->GetTexture(), 16, 4096); // 32

	//{
	//	auto ClearColor = Vector4(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);	// light sky blue
	//	auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
	//	auto EnableDepthTest = true;
	//	auto DepthStencilFunc = EComparisonFunc::LESS;
	//	auto EnableBlend = true;
	//	auto BlendSrc = EBlendSrc::ONE;
	//	auto BlendDest = EBlendDest::ZERO;
	//	auto Shader = jShader::GetShader("Skin");
	//	auto EnableClear = true;
	//	bool EnableDepthBias = false;
	//	float DepthSlopeBias = 1.0f;
	//	float DepthConstantBias = 1.0f;

	//	if (EnableClear)
	//	{
	//		g_rhi->SetClearColor(ClearColor);
	//		g_rhi->SetClear(ClearType);
	//	}

	//	g_rhi->EnableDepthTest(EnableDepthTest);
	//	g_rhi->SetDepthFunc(DepthStencilFunc);

	//	g_rhi->EnableBlend(EnableBlend);
	//	g_rhi->SetBlendFunc(BlendSrc, BlendDest);

	//	g_rhi->EnableDepthBias(EnableDepthBias);
	//	g_rhi->SetDepthBias(DepthConstantBias, DepthSlopeBias);

	//	g_rhi->SetShader(Shader);

	//	std::list<const jLight*> lights;
	//	lights.insert(lights.end(), MainCamera->LightList.begin(), MainCamera->LightList.end());

	//	MainCamera->BindCamera(Shader);
	//	jLight::BindLights(lights, Shader);

	//	for (const auto& iter : jObject::GetStaticObject())
	//		iter->Draw(MainCamera, Shader, lights);
	//}

	jTexture* SelectedIrrTexture = nullptr;
	static int32 Sel = 0;

	for (int i = 0; i < 6; ++i)
	{
		if (g_KeyState['1' + i])
		{
			Sel = i;
			break;
		}
	}

	switch (Sel)
	{
	case 0:
		SelectedIrrTexture = IrrTarget->GetTexture();
		break;
	case 1:
		SelectedIrrTexture = IrrBlurTarget2->GetTexture();
		break;
	case 2:
		SelectedIrrTexture = IrrBlurTarget4->GetTexture();
		break;
	case 3:
		SelectedIrrTexture = IrrBlurTarget8->GetTexture();
		break;
	case 4:
		SelectedIrrTexture = IrrBlurTarget16->GetTexture();
		break;
	case 5:
	default:
		SelectedIrrTexture = IrrBlurTarget32->GetTexture();
		break;
	}

	{
		auto ClearColor = Vector4(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);	// light sky blue
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;
		auto Shader = jShader::GetShader("SkinFinal");
		auto EnableClear = true;
		bool EnableDepthBias = false;
		float DepthSlopeBias = 1.0f;
		float DepthConstantBias = 1.0f;

		if (EnableClear)
		{
			g_rhi->SetClearColor(ClearColor);
			g_rhi->SetClear(ClearType);
		}

		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->SetDepthFunc(DepthStencilFunc);

		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);

		g_rhi->EnableDepthBias(EnableDepthBias);
		g_rhi->SetDepthBias(DepthConstantBias, DepthSlopeBias);

		g_rhi->SetShader(Shader);
		g_rhi->SetUniformbuffer(&jUniformBuffer<float>("TextureSize", 4096), Shader);

		std::list<const jLight*> lights;
		lights.insert(lights.end(), MainCamera->LightList.begin(), MainCamera->LightList.end());

		MainCamera->BindCamera(Shader);
		jLight::BindLights(lights, Shader);

		//for (const auto& iter : jObject::GetStaticObject())
		   // iter->Draw(MainCamera, Shader, lights);

		headModel->SetMaterialOverride = [SelectedIrrTexture](jMeshObject* InMeshObject)
		{
			InMeshObject->RenderObject->tex_object[0] = IrrTarget->GetTexture();
			InMeshObject->RenderObject->tex_object[1] = IrrBlurTarget2->GetTexture();
			InMeshObject->RenderObject->tex_object[2] = IrrBlurTarget4->GetTexture();
			InMeshObject->RenderObject->tex_object[3] = IrrBlurTarget8->GetTexture();
			InMeshObject->RenderObject->tex_object[4] = IrrBlurTarget16->GetTexture();
			InMeshObject->RenderObject->tex_object[5] = IrrBlurTarget32->GetTexture();
			InMeshObject->RenderObject->tex_object[6] = InMeshObject->MeshData->Materials[1]->Texture;
			//InMeshObject->RenderObject->tex_object[7] = StrechTarget->GetTexture();
			headModel->RenderObject->tex_object[7] = headModelWorldNormalTexture;
			headModel->RenderObject->tex_object[8] = TSMTarget->GetTexture();
			headModel->RenderObject->tex_object[9] = StrechTarget->GetTexture();
		};
		headModel->Draw(MainCamera, Shader, { lights });
		headModel->SetMaterialOverride = nullptr;

		headModel->RenderObject->tex_object[0] = nullptr;
		headModel->RenderObject->tex_object[1] = nullptr;
		headModel->RenderObject->tex_object[2] = nullptr;
		headModel->RenderObject->tex_object[3] = nullptr;
		headModel->RenderObject->tex_object[4] = nullptr;
		headModel->RenderObject->tex_object[5] = nullptr;
		headModel->RenderObject->tex_object[6] = nullptr;
		headModel->RenderObject->tex_object[7] = nullptr;
		headModel->RenderObject->tex_object[8] = nullptr;
		headModel->RenderObject->tex_object[9] = nullptr;

		headModel->RenderObject->tex_object[2] = headModelWorldNormalTexture;
	}

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
		PreviewUI->RenderObject->tex_object[0] = TEXTURE;\
		PreviewUI->Draw(MainCamera, Shader, {});\
	}

	static bool Preview = false;
	if (g_KeyState['z'])
		Preview = true;
	if (g_KeyState['x'])
		Preview = false;
	if (Preview)
	{
		PreviewUI->Pos = Vector2(SCR_WIDTH - PreviewSize.x * 4, SCR_HEIGHT - PreviewSize.y);
		PREVIEW_TEXTURE(MainCamera->GetLight(ELightType::DIRECTIONAL)->GetShadowMapRenderTarget()->GetTexture());

		PreviewUI->Pos.x += PreviewSize.x;
		PREVIEW_TEXTURE(IrrTarget->GetTexture());

		PreviewUI->Pos.x += PreviewSize.x;
		PREVIEW_TEXTURE(IrrBlurTarget2->GetTexture());

		PreviewUI->Pos.x += PreviewSize.x;
		PREVIEW_TEXTURE(IrrBlurTarget4->GetTexture());

		PreviewUI->Pos = Vector2(SCR_WIDTH - PreviewSize.x * 4, SCR_HEIGHT - PreviewSize.y * 2);
		PREVIEW_TEXTURE(StrechTarget->GetTexture());

		PreviewUI->Pos.x += PreviewSize.x;
		PREVIEW_TEXTURE(IrrBlurTarget8->GetTexture());

		PreviewUI->Pos.x += PreviewSize.x;
		PREVIEW_TEXTURE(IrrBlurTarget16->GetTexture());

		PreviewUI->Pos.x += PreviewSize.x;
		PREVIEW_TEXTURE(IrrBlurTarget32->GetTexture());
	}
	else
	{
		static bool Preview2 = false;
		if (g_KeyState['c'])
			Preview2 = true;
		if (g_KeyState['v'])
			Preview2 = false;
		if (Preview2)
		{
			PreviewUI->Pos = Vector2(SCR_WIDTH - PreviewSize.x, SCR_HEIGHT - PreviewSize.y);
			PREVIEW_TEXTURE(TSMTarget->GetTexture());
		}
	}
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
