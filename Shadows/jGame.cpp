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
	const Vector mainCameraPos(-56.6983185f, -28.4189625f, -139.300461f);
	const Vector mainCameraTarget(-56.2905045f, -28.4190006f, -138.387177f);
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

	PointLight = jLight::CreatePointLight(jShadowAppSettingProperties::GetInstance().PointLightPosition, Vector4(2.0f, 0.7f, 0.7f, 1.0f), 500.0f, Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), 64.0f);
	SpotLight = jLight::CreateSpotLight(jShadowAppSettingProperties::GetInstance().SpotLightPosition, jShadowAppSettingProperties::GetInstance().SpotLightDirection, Vector4(0.0f, 1.0f, 0.0f, 1.0f), 500.0f, 0.7f, 1.0f, Vector(1.0f, 1.0f, 1.0f), Vector(1.0f), 64.0f);

	DirectionalLightInfo = jPrimitiveUtil::CreateDirectionalLightDebug(Vector(250, 260, 0)*0.5f, Vector::OneVector * 10.0f, 10.0f, MainCamera, DirectionalLight, "Image/sun.png");
	jObject::AddDebugObject(DirectionalLightInfo);

	DirectionalLightShadowMapUIDebug = jPrimitiveUtil::CreateUIQuad({ 0.0f, 0.0f }, { 150, 150 }, DirectionalLight->GetShadowMap());
	jObject::AddUIDebugObject(DirectionalLightShadowMapUIDebug);

	PointLightInfo = jPrimitiveUtil::CreatePointLightDebug(Vector(10.0f), MainCamera, PointLight, "Image/bulb.png");
	//jObject::AddDebugObject(PointLightInfo);

	SpotLightInfo = jPrimitiveUtil::CreateSpotLightDebug(Vector(10.0f), MainCamera, SpotLight, "Image/spot.png");
	//jObject::AddDebugObject(SpotLightInfo);

	MainCamera->AddLight(DirectionalLight);
	MainCamera->AddLight(PointLight);
	MainCamera->AddLight(SpotLight);
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

	//////////////////////////////////////////////////////////////////////////
	// Get the 8 points of the view frustum in world space
	if (jShadowAppSettingProperties::GetInstance().ShadowMapType != EShadowMapType::DeepShadowMap_DirectionalLight)
	{
		Vector frustumCornersWS[8] =
		{
			Vector(-1.0f,  1.0f, -1.0f),
			Vector(1.0f,  1.0f, -1.0f),
			Vector(1.0f, -1.0f, -1.0f),
			Vector(-1.0f, -1.0f, -1.0f),
			Vector(-1.0f,  1.0f, 1.0f),
			Vector(1.0f,  1.0f, 1.0f),
			Vector(1.0f, -1.0f, 1.0f),
			Vector(-1.0f, -1.0f, 1.0f),
		};

		Vector frustumCenter(0.0f);
		Matrix invViewProj = (MainCamera->Projection * MainCamera->View).GetInverse();
		for (uint32 i = 0; i < 8; ++i)
		{
			frustumCornersWS[i] = invViewProj.Transform(frustumCornersWS[i]);
			frustumCenter = frustumCenter + frustumCornersWS[i];
		}
		frustumCenter = frustumCenter * (1.0f / 8.0f);

		auto upDir = Vector::UpVector;

		float width = SM_WIDTH;
		float height = SM_HEIGHT;
		float nearDist = 10.0f;
		float farDist = 1000.0f;

		// Get position of the shadow camera
		Vector shadowCameraPos = frustumCenter + DirectionalLight->Data.Direction * -(farDist - nearDist) / 2.0f;
	
		auto shadowCamera = jOrthographicCamera::CreateCamera(shadowCameraPos, frustumCenter, shadowCameraPos + upDir
			, -width / 2.0f, -height / 2.0f, width / 2.0f, height / 2.0f, farDist, nearDist);
		shadowCamera->UpdateCamera();
		DirectionalLight->GetLightCamra()->Projection = shadowCamera->Projection;
		DirectionalLight->GetLightCamra()->View = shadowCamera->View;
	}
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

	//Renderer->Render(MainCamera);

	static jTexture* GraceProbeTexture = nullptr;
	static jTexture* GraceProbeRGBATexture = nullptr;
	static bool IsLoadTexture = false;
	if (!IsLoadTexture)
	{
		IsLoadTexture = true;

		jImageData dataTwoMirrorBall;
		jImageData dataTwoMirrorBallRGBA;

		jImageFileLoader::GetInstance().LoadTextureFromFile(dataTwoMirrorBall, "Image/grace_probe.hdr", false);
		GraceProbeTexture = g_rhi->CreateTextureFromData(&dataTwoMirrorBall.ImageData[0], dataTwoMirrorBall.Width, dataTwoMirrorBall.Height
			, dataTwoMirrorBall.sRGB, EFormatType::FLOAT, ETextureFormat::RGB16F);

		dataTwoMirrorBallRGBA.Width = dataTwoMirrorBall.Width;
		dataTwoMirrorBallRGBA.Height = dataTwoMirrorBall.Height;

		const int32 TotalPixelCount = dataTwoMirrorBall.Width * dataTwoMirrorBall.Height;
		dataTwoMirrorBallRGBA.ImageData.resize(TotalPixelCount * sizeof(Vector4));

		Vector4* DestPtr = (Vector4*)&dataTwoMirrorBallRGBA.ImageData[0];
		Vector* SourcePtr = (Vector*)&dataTwoMirrorBall.ImageData[0];
		for (int32 i = 0; i < TotalPixelCount; ++i)
			*(DestPtr + i) = Vector4(*(SourcePtr + i), 1.0);
		GraceProbeRGBATexture = g_rhi->CreateTextureFromData(&dataTwoMirrorBallRGBA.ImageData[0], dataTwoMirrorBallRGBA.Width, dataTwoMirrorBallRGBA.Height
			, dataTwoMirrorBallRGBA.sRGB, EFormatType::FLOAT, ETextureFormat::RGBA16F);

#if 0 // Generate SH Llm on CPU
		auto GetSphericalMap_TwoMirrorBall = [](const Vector& InDir)
		{
			// Convert from Direction3D to UV[-1, 1]
			// - Direction : (Dx, Dy, Dz)
			// - r=(1/pi)*acos(Dz)/sqrt(Dx^2 + Dy^2)
			// - (Dx*r,Dy*r)
			//
			// To rerange UV from [-1, 1] to [0, 1] below explanation with code needed.
			// 0.159154943 == 1.0 / (2.0 * PI)
			// Original r is "r=(1/pi)*acos(Dz)/sqrt(Dx^2 + Dy^2)"
			// To adjust number's range from [-1.0, 1.0] to [0.0, 1.0] for TexCoord, we need this "[-1.0, 1.0] / 2.0 + 0.5"
			// - This is why we use (1.0 / 2.0 * PI) instead of (1.0 / PI) in "r".
			// - adding 0.5 execute next line. (here : "float u = 0.5 + InDir.x * r" and "float v = 0.5 + InDir.y * r")
			float d = sqrt(InDir.x * InDir.x + InDir.y * InDir.y);
			float r = (d > 0.0f) ? (0.159154943f * acos(InDir.z) / d) : 0.0f;
			float u = 0.5f + InDir.x * r;
			float v = 0.5f + InDir.y * r;
			//color = texture(tex_object, vec2(u, v));
			return Vector2(u, v);
		};

		auto GenerateLlm = [&dataTwoMirrorBall, &GetSphericalMap_TwoMirrorBall](Vector (&OutLlm)[9]){
			memset(&OutLlm[0], 0, sizeof(OutLlm));

			Vector* dataPtr = (Vector*)&dataTwoMirrorBall.ImageData[0];
			float sampleDelta = 0.015f;
			int32 nrSamples = 0;
			float SHBasisR = 1.0f;
			for (float phi = sampleDelta; phi <= 2.0 * PI; phi += sampleDelta)
			{
				for (float theta = sampleDelta; theta <= PI; theta += sampleDelta)
				{
					// spherical to cartesian (in tangent space)
					Vector sampleVec = Vector(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta)); // tangent space to world
					sampleVec = sampleVec.GetNormalize();

					float LocalYlm[9];
					LocalYlm[0] = 0.5f * sqrt(1.0f / PI);       // 0
					LocalYlm[1] = 0.5f * sqrt(3.0f / PI) * sampleVec.y / SHBasisR;      // 1
					LocalYlm[2] = 0.5f * sqrt(3.0f / PI) * sampleVec.z / SHBasisR;      // 2
					LocalYlm[3] = 0.5f * sqrt(3.0f / PI) * sampleVec.x / SHBasisR;      // 3
					LocalYlm[4] = 0.5f * sqrt(15.0f / PI) * sampleVec.x * sampleVec.y / (SHBasisR * SHBasisR);      // 4
					LocalYlm[5] = 0.5f * sqrt(15.0f / PI) * sampleVec.y * sampleVec.z / (SHBasisR * SHBasisR);      // 5
					LocalYlm[6] = 0.25f * sqrt(5.0f / PI) * (-sampleVec.x * sampleVec.x - sampleVec.y * sampleVec.y + 2.0f * sampleVec.z * sampleVec.z) / (SHBasisR * SHBasisR);     // 6
					LocalYlm[7] = 0.5f * sqrt(15.0f / PI) * sampleVec.z * sampleVec.x / (SHBasisR * SHBasisR);  // 7
					LocalYlm[8] = 0.25f * sqrt(15.0f / PI) * (sampleVec.x * sampleVec.x - sampleVec.y * sampleVec.y) / (SHBasisR * SHBasisR); // 8

					Vector2 TargetUV = GetSphericalMap_TwoMirrorBall(sampleVec);
					int32 DataX = (int32)(TargetUV.x * dataTwoMirrorBall.Width);
					int32 DataY = (int32)((1.0 - TargetUV.y) * dataTwoMirrorBall.Height);
					Vector curRGB = *(dataPtr + (DataY * dataTwoMirrorBall.Width + DataX));
					//Vector InnerIntegrate = curRGB * cos(t) * sin(t);

					OutLlm[0] += LocalYlm[0] * curRGB * sin(theta);
					OutLlm[1] += LocalYlm[1] * curRGB * sin(theta);
					OutLlm[2] += LocalYlm[2] * curRGB * sin(theta);
					OutLlm[3] += LocalYlm[3] * curRGB * sin(theta);
					OutLlm[4] += LocalYlm[4] * curRGB * sin(theta);
					OutLlm[5] += LocalYlm[5] * curRGB * sin(theta);
					OutLlm[6] += LocalYlm[6] * curRGB * sin(theta);
					OutLlm[7] += LocalYlm[7] * curRGB * sin(theta);
					OutLlm[8] += LocalYlm[8] * curRGB * sin(theta);
					nrSamples++;
				}
			}
			for (int i = 0; i < 9; ++i)
				OutLlm[i] = 2.0f * PI * OutLlm[i] * (1.0f / float(nrSamples));
		};
		Vector ResultLlm[9];
		GenerateLlm(ResultLlm);
#endif
	}

	// Origin paper's Llm list.
	const Vector PreComputedLlm[9] =
	{
		Vector(0.79f, 0.44f, 0.54f),         // L00
		Vector(0.39f, 0.35f, 0.60f),         // L1-1
		Vector(-0.34f, -0.18f, -0.27f),      // L10
		Vector(-0.29f, -0.06f, 0.01f),       // L11
		Vector(-0.11f, -0.05f, -0.12f),      // L2-2
		Vector(-0.26f, -0.22f, -0.47f),      // L2-1
		Vector(-0.16f, -0.09f, -0.15f),      // L20
		Vector(0.56f, 0.21f, 0.14f),         // L21
		Vector(0.21f, -0.05f, -0.30f),       // L22
	};

	g_rhi->EnableDepthTest(true);

	auto appSettings = jShadowAppSettingProperties::GetInstance();
	jAppSettings::GetInstance().Get("MainPannel")->SetVisible("IrrMapSH", (appSettings.SHIrradianceType == ESHIrradianceType::GenIrrMapSH) ? 1 : 0);

	switch (appSettings.SHIrradianceType)
	{
	case ESHIrradianceType::SHPlot:
	{
		// Render SH Plot3D
		g_rhi->SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });

		static auto sphere = jPrimitiveUtil::CreateSphere(Vector(0.0f, 0.0f, 0.0f), 0.5, 32, Vector(100.0f), Vector4(1.0f, 1.0f, 1.0f, 1.0f));
		sphere->RenderObject->tex_object = GraceProbeTexture;
		auto shader = jShader::GetShader("SphericalHarmonicsPlot");

		float Interval = 30.0f;

		sphere->RenderObject->Rot.x = RadianToDegree(90.0f);
		for (int32 l = 0; l <= 2; l++) 
		{
			sphere->RenderObject->Pos.y = -l * Interval;

			for (int32 m = -l; m <= l; m++)
			{
				sphere->RenderObject->Pos.x = -m * Interval;
				SET_UNIFORM_BUFFER_STATIC(int32, "l", l, shader);
				SET_UNIFORM_BUFFER_STATIC(int32, "m", m, shader);
				sphere->Draw(MainCamera, shader, {});
			}
		}

		break;
	}
	case ESHIrradianceType::EnvMap:
	case ESHIrradianceType::IrrEnvMap:
	{
		// Render EnvMap
		static auto TeapotModel = jModelLoader::GetInstance().LoadFromFile("model/teapot.x");

		static auto EnvSphere = jPrimitiveUtil::CreateSphere(Vector::ZeroVector, 0.5f, 30, Vector(1.0f), Vector4::ColorWhite);
		const bool IsIrrSHEnv = (appSettings.SHIrradianceType == ESHIrradianceType::IrrEnvMap);

		auto ModelShader = jShader::GetShader("SimpleIrrMap");

		g_rhi->SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });

		jShader* shader = nullptr;
		shader = jShader::GetShader("SphereEnv");

		float Al[3] = { 3.141593f, 2.094395f, 0.785398f };
		char szTemp[128] = { 0, };

		MainCamera->IsInfinityFar = true;
		MainCamera->UpdateCamera();

		g_rhi->SetShader(shader);
		for (int32 i = 0; i < 3; ++i)
		{
			sprintf_s(szTemp, sizeof(szTemp), "Al[%d]", i);
			jUniformBuffer<float> temp(szTemp, Al[i]);
			g_rhi->SetUniformbuffer(&temp, shader);
		}

		for (int32 i = 0; i < 9; ++i)
		{
			sprintf_s(szTemp, sizeof(szTemp), "Llm[%d]", i);
			jUniformBuffer<Vector> temp(szTemp, PreComputedLlm[i]);
			g_rhi->SetUniformbuffer(&temp, shader);
		}

		EnvSphere->RenderObject->tex_object = GraceProbeTexture;
		EnvSphere->Update(deltaTime);
		EnvSphere->Draw(MainCamera, shader, {});

		MainCamera->IsInfinityFar = false;
		MainCamera->UpdateCamera();

		g_rhi->SetShader(ModelShader);
		for (int32 i = 0; i < 3; ++i)
		{
			sprintf_s(szTemp, sizeof(szTemp), "Al[%d]", i);
			jUniformBuffer<float> temp(szTemp, Al[i]);
			g_rhi->SetUniformbuffer(&temp, ModelShader);
		}

		for (int32 i = 0; i < 9; ++i)
		{
			sprintf_s(szTemp, sizeof(szTemp), "Llm[%d]", i);
			jUniformBuffer<Vector> temp(szTemp, PreComputedLlm[i]);
			g_rhi->SetUniformbuffer(&temp, ModelShader);
		}
		SET_UNIFORM_BUFFER_STATIC(int, "IsIrranceMap", IsIrrSHEnv, ModelShader);
		EnvSphere->RenderObject->tex_object = GraceProbeTexture;
		EnvSphere->RenderObject->Scale = Vector(40.0f);
		EnvSphere->RenderObject->Pos.x = 100.0f;
		EnvSphere->Update(deltaTime);
		EnvSphere->Draw(MainCamera, ModelShader, {DirectionalLight});

		EnvSphere->RenderObject->Pos.x = 0.0f;

		TeapotModel->RenderObject->tex_object = GraceProbeTexture;
		TeapotModel->RenderObject->Scale = Vector(40.0f);
		TeapotModel->Update(deltaTime);
		TeapotModel->Draw(MainCamera, ModelShader, { DirectionalLight });

		break;
	}
	case ESHIrradianceType::GenIrrMapBruteForce:
	case ESHIrradianceType::GenIrrMapSH:
	{
		Vector SelectedLlm[9];

		// Generate Llm from ComputeShader on Realtime
		const bool IsGenIrradianceMapFromSH = (appSettings.SHIrradianceType == ESHIrradianceType::GenIrrMapSH);
		if (IsGenIrradianceMapFromSH && appSettings.IsGenerateLlmRealtime)
		{
			auto GenSHLlmShader = jShader::GetShader("GenSHLlm");
			g_rhi->SetShader(GenSHLlmShader);

			float Al[3] = { 3.141593f, 2.094395f, 0.785398f };
			char szTemp[128] = { 0, };
			for (int32 i = 0; i < 3; ++i)
			{
				sprintf_s(szTemp, sizeof(szTemp), "Al[%d]", i);
				jUniformBuffer<float> temp(szTemp, Al[i]);
				g_rhi->SetUniformbuffer(&temp, GenSHLlmShader);
			}

			Vector4 LocalLlm[9];
			memset(LocalLlm, 0, sizeof(LocalLlm));

			static auto LlmBuffer = static_cast<jShaderStorageBufferObject_OpenGL*>(g_rhi->CreateShaderStorageBufferObject("LlmBuffer"));
			LlmBuffer->UpdateBufferData(LocalLlm, sizeof(LocalLlm));
			LlmBuffer->Bind(GenSHLlmShader);

			SET_UNIFORM_BUFFER_STATIC(int, "TexWidth", GraceProbeRGBATexture->Width, GenSHLlmShader);
			SET_UNIFORM_BUFFER_STATIC(int, "TexHeight", GraceProbeRGBATexture->Height, GenSHLlmShader);

			g_rhi->SetImageTexture(0, GraceProbeRGBATexture, EImageTextureAccessType::READ_ONLY);
			g_rhi->DispatchCompute(1, 1, 1);

			LlmBuffer->GetBufferData(LocalLlm, sizeof(LocalLlm));
			for (int32 i = 0; i < 9; ++i)
				SelectedLlm[i] = Vector(LocalLlm[i]);
		}
		else
		{
			JASSERT(sizeof(SelectedLlm) == sizeof(PreComputedLlm));
			memcpy(SelectedLlm, PreComputedLlm, sizeof(SelectedLlm));
		}

		// Render IrradianceMap as "TwoMirrorBall SphereMap(Support 360 degree)"
		g_rhi->SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });
		g_rhi->EnableDepthTest(true);
		auto Shader = jShader::GetShader("GenIrrSphereMap");

		static auto UIQuad = jPrimitiveUtil::CreateUIQuad({ 300.0, 100.0 }, { 600, 600 }, GraceProbeTexture);
		if (UIQuad)
		{
			SET_UNIFORM_BUFFER_STATIC(bool, "IsGenIrradianceMapFromSH", IsGenIrradianceMapFromSH, Shader);
			SET_UNIFORM_BUFFER_STATIC(bool, "IsGenerateLlmRealtime", appSettings.IsGenerateLlmRealtime, Shader);

			float Al[3] = { 3.141593f, 2.094395f, 0.785398f };

			g_rhi->SetShader(Shader);

			char szTemp[128] = { 0, };
			for (int32 i = 0; i < 3; ++i)
			{
				sprintf_s(szTemp, sizeof(szTemp), "Al[%d]", i);
				jUniformBuffer<float> temp(szTemp, Al[i]);
				g_rhi->SetUniformbuffer(&temp, Shader);
			}

			for (int32 i = 0; i < 9; ++i)
			{
				sprintf_s(szTemp, sizeof(szTemp), "Llm[%d]", i);
				jUniformBuffer<Vector> temp(szTemp, PreComputedLlm[i]);
				g_rhi->SetUniformbuffer(&temp, Shader);
			}

			UIQuad->SetTexture(GraceProbeTexture);
			UIQuad->RenderObject->samplerState = jSamplerStatePool::GetSamplerState("LinearClamp").get();
			UIQuad->RenderObject->samplerState2 = jSamplerStatePool::GetSamplerState("LinearClamp").get();

			UIQuad->Update(deltaTime);
			UIQuad->Draw(MainCamera, Shader, {});
		}

		break;
	}
	case ESHIrradianceType::MAX:
	default:
		break;
	}
}

void jGame::UpdateAppSetting()
{
	auto& appSetting =  jShadowAppSettingProperties::GetInstance();

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
