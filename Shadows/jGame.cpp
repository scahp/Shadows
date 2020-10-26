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
	const Vector mainCameraPos(172.66f, 160.0f, -180.63f);
	//const Vector mainCameraTarget(171.96f, 166.02f, -180.05f);
	//const Vector mainCameraPos(165.0f, 125.0f, -136.0f);
	//const Vector mainCameraPos(300.0f, 100.0f, 300.0f);
	const Vector mainCameraTarget(0.0f, 0.0f, 0.0f);
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

	DirectionalLightInfo = jPrimitiveUtil::CreateDirectionalLightDebug(Vector(250, 260, 0) * 0.5f, Vector::OneVector * 10.0f, 10.0f, MainCamera, DirectionalLight, "Image/sun.png");
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
		? ShadowPipelineSetMap[CurrentShadowMapType] : ShadowVolumePipelineSet;
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

	//////////////////////////////////////////////////////////////////////////
	static bool LoadCudbeMap = false;
	static jTexture* CubeMapTexture = nullptr;
	if (!LoadCudbeMap)
	{
		LoadCudbeMap = true;

		jImageData data[6];
		jImageFileLoader::GetInstance().LoadTextureFromFile(data[0], "Image/EnvCube/xp.png", true);
		jImageFileLoader::GetInstance().LoadTextureFromFile(data[1], "Image/EnvCube/xn.png", true);
		jImageFileLoader::GetInstance().LoadTextureFromFile(data[2], "Image/EnvCube/yp.png", true);
		jImageFileLoader::GetInstance().LoadTextureFromFile(data[3], "Image/EnvCube/yn.png", true);
		jImageFileLoader::GetInstance().LoadTextureFromFile(data[4], "Image/EnvCube/zp.png", true);
		jImageFileLoader::GetInstance().LoadTextureFromFile(data[5], "Image/EnvCube/zn.png", true);

		uint8* Datas[6];
		for (int32 i = 0; i < 6; ++i)
			Datas[i] = data[i].ImageData.data();

		CubeMapTexture = g_rhi->CreateCubeTextureFromData(Datas, data[0].Width, data[0].Height, data[0].sRGB);
	}

	static auto EnvCube = jPrimitiveUtil::CreateCube(Vector::ZeroVector, Vector(0.5f), Vector(1.0f), Vector4::ColorWhite);

	constexpr static int32 kNumGeoWaves = 4;
	class GeoWaveDesc
	{
	public:
		float		Phase;		// Speed * 2PI / Length = Speed * Frequency
		float		Amp;		// Amplitude
		float		Len;		// Length
		float		Freq;		// Frequency = w = 2PI / Length
		Vector2		Dir;		// Direction
		float		Fade;		// 
	};
	static GeoWaveDesc		GeoWaves[kNumGeoWaves];

	class GeoState
	{
	public:
		float		Chop;				// 가파름 파라메터
		float		AngleDeviation;
		Vector2		WindDir;
		float		MinLength;
		float		MaxLength;
		float		AmpOverLen;

		float		SpecAtten;
		float		SpecEnd;
		float		SpecTrans;

		float		EnvHeight;
		float		EnvRadius;
		float		WaterLevel;

		int32		TransIdx;
		float		TransDel;
	};
	static GeoState	GeoState;

	static bool IsGeoStateInit = false;
	if (!IsGeoStateInit)
	{
		IsGeoStateInit = true;

		GeoState.Chop = 2.5f;
		GeoState.AngleDeviation = 15.f;
		GeoState.WindDir.x = 0;
		GeoState.WindDir.y = 1.f;

		GeoState.MinLength = 15.f;
		GeoState.MaxLength = 25.f;
		GeoState.AmpOverLen = 0.1f;

		GeoState.EnvHeight = -50.f;
		GeoState.EnvRadius = 200.f;
		GeoState.WaterLevel = -2.f;

		GeoState.TransIdx = 0;
		GeoState.TransDel = -1.f / 6.f;

		GeoState.SpecAtten = 1.f;
		GeoState.SpecEnd = 200.f;
		GeoState.SpecTrans = 100.f;
	}

	static auto RandZeroToOne = []()
	{
		return float(double(rand()) / double(RAND_MAX));
	};

	static auto RandMinusOneToOne = []()
	{
		return float(double(rand()) / double(RAND_MAX) * 2.0 - 1.0);
	};

	auto InitGeoWave = [this](int32 InIndex)
	{
		// Speed * 2PI / Length
		GeoWaves[InIndex].Phase = RandZeroToOne() * PI * 2.f;

		// WaveLength : MinLength ~ MaxLength 사이의 값을 선택
		GeoWaves[InIndex].Len = GeoState.MinLength + RandZeroToOne() * (GeoState.MaxLength - GeoState.MinLength);

		// Amplitude / WaveCount : 진폭을 4개의 파도가 나눠가지도록 함. 아마 4개가 동시에 겹쳤을때를 고려하는게 아닐까?
		GeoWaves[InIndex].Amp = GeoWaves[InIndex].Len * GeoState.AmpOverLen / float(kNumGeoWaves);

		// 빈도 : 2PI / Length
		GeoWaves[InIndex].Freq = 2.f * PI / GeoWaves[InIndex].Len;

		// Fade : 없어지는 시간
		GeoWaves[InIndex].Fade = 1.f;

		// 바람의 방향을 랜덤으로 변경시켜주는 부분
		// Radian 각도로 변경을 좌우, 위아래로 변화시킬 정도를 정의 -rotBase ~ rotBase
		float rotBase = GeoState.AngleDeviation * PI / 180.f;

		float rads = rotBase * RandMinusOneToOne();
		float rx = float(cosf(rads));
		float ry = float(sinf(rads));

		float x = GeoState.WindDir.x;
		float y = GeoState.WindDir.y;
		GeoWaves[InIndex].Dir.x = x * rx + y * ry;
		GeoWaves[InIndex].Dir.y = x * -ry + y * rx;
	};

	static bool IsGeoWaveInit = false;
	if (!IsGeoWaveInit)
	{
		IsGeoWaveInit = true;

		for (int32 i = 0; i < kNumGeoWaves; ++i)
		{
			InitGeoWave(i);
		}
	}

	static const float kGravConst = 30.f;

	// UpdateGeoWave
	if (1)
	{
		for (int32 i = 0; i < kNumGeoWaves; ++i)
		{
			// 프레임당 파도 하나 업데이트 함
			// TransInx : 이번 프레임에 업데이트 될 Geo Wave 번호
			if (i == GeoState.TransIdx)
			{
				// Fade는 TransDel 만큼 매초당 Fade에 더해짐.
				// TransDel : 초당 Fade될 크기
				// TransDel이 양 <-> 음 수가 되면서 파도의 Fade in / out 구현
				GeoWaves[i].Fade += GeoState.TransDel * deltaTime;
				if (GeoWaves[i].Fade < 0)
				{
					// This wave is faded out. Re-init and fade it back up.
					InitGeoWave(i);
					GeoWaves[i].Fade = 0;
					GeoState.TransDel = -GeoState.TransDel;
				}
				else if (GeoWaves[i].Fade > 1.f)
				{
					// This wave is faded back up. Start fading another down.
					GeoWaves[i].Fade = 1.f;
					GeoState.TransDel = -GeoState.TransDel;
					if (++GeoState.TransIdx >= kNumGeoWaves)
						GeoState.TransIdx = 0;
				}
			}

			// ??? Phase function 계산이 생각과 다르다.
			const float speed = float(1.0 / sqrt(GeoWaves[i].Len / (2.f * PI * kGravConst)));

			// Speed는 초당 속도, deltatime 만큼 이동
			GeoWaves[i].Phase += speed * deltaTime;

			// Phase function 을 2PI의 배수로 설정
			GeoWaves[i].Phase = float(fmod(GeoWaves[i].Phase, 2.f * PI));

			// Amp의 크기를 4개의 파도가 서로 나눠 가지게 됨.
			GeoWaves[i].Amp = GeoWaves[i].Len * GeoState.AmpOverLen / float(kNumGeoWaves) * GeoWaves[i].Fade;

			//// 테스트 코드
			//static int TestIndex = 0;
			//if (i != TestIndex)
			//	GeoWaves[i].Amp = 0;
		}
	}

	class TexState
	{
	public:
		float		Noise;
		float		Chop;
		float		AngleDeviation;
		Vector2	WindDir;
		float		MaxLength;
		float		MinLength;
		float		AmpOverLen;
		float		RippleScale;
		float		SpeedDeviation;

		int32			TransIdx;
		float		TransDel;
	};
	static TexState		TexState;

	constexpr static int32 kNumBumpPerPass = 4;
	constexpr static int32 kNumTexWaves = 16;
	constexpr static int32 kNumBumpPasses = kNumTexWaves / kNumBumpPerPass;
	constexpr static int32 kBumpTexSize = 256;

	class TexWaveDesc
	{
	public:
		float		Phase;		// Speed * 2pi/L
		float		Amp;
		float		Len;
		float		Speed;
		float		Freq;
		Vector2		Dir;
		Vector2		RotScale;
		float		Fade;
	};
	static TexWaveDesc		TexWaves[kNumTexWaves];

	static bool IsInitTexState = false;
	if (!IsInitTexState)
	{
		IsInitTexState = true;

		TexState.Noise = 0.2f;
		TexState.Chop = 1.f;
		TexState.AngleDeviation = 15.f;
		TexState.WindDir.x = 0;
		TexState.WindDir.y = 1.f;
		TexState.MaxLength = 10.f;
		TexState.MinLength = 1.f;
		TexState.AmpOverLen = 0.1f;
		TexState.RippleScale = 25.f;
		TexState.SpeedDeviation = 0.1f;

		TexState.TransIdx = 0;
		TexState.TransDel = -1.f / 5.f;
	}

	auto InitTexWave = [this](int32 InIndex)
	{
		float rads = RandMinusOneToOne() * TexState.AngleDeviation * PI / 180.f;
		float dx = float(sin(rads));
		float dy = float(cos(rads));

		float tx = dx;
		dx = TexState.WindDir.y * dx - TexState.WindDir.x * dy;
		dy = TexState.WindDir.x * tx + TexState.WindDir.y * dy;

		float maxLen = TexState.MaxLength * kBumpTexSize / TexState.RippleScale;
		float minLen = TexState.MinLength * kBumpTexSize / TexState.RippleScale;
		float len = float(InIndex) / float(kNumTexWaves - 1) * (maxLen - minLen) + minLen;

		float reps = float(kBumpTexSize) / len;

		dx *= reps;
		dy *= reps;
		dx = float(int(dx >= 0 ? dx + 0.5f : dx - 0.5f));
		dy = float(int(dy >= 0 ? dy + 0.5f : dy - 0.5f));

		TexWaves[InIndex].RotScale.x = dx;
		TexWaves[InIndex].RotScale.y = dy;

		float effK = float(1.0 / sqrt(dx * dx + dy * dy));
		TexWaves[InIndex].Len = float(kBumpTexSize) * effK;
		TexWaves[InIndex].Freq = PI * 2.f / TexWaves[InIndex].Len;
		TexWaves[InIndex].Amp = TexWaves[InIndex].Len * TexState.AmpOverLen;
		TexWaves[InIndex].Phase = RandZeroToOne();

		TexWaves[InIndex].Dir.x = dx * effK;
		TexWaves[InIndex].Dir.y = dy * effK;

		TexWaves[InIndex].Fade = 1.f;

		float speed = float(1.0 / sqrt(TexWaves[InIndex].Len / (2.f * PI * kGravConst))) / 3.f;
		speed *= 1.f + RandMinusOneToOne() * TexState.SpeedDeviation;
		TexWaves[InIndex].Speed = speed;
	};

	// UpdateTexWave
	for (int32 i = 0; i < kNumTexWaves; ++i)
	{
		if (i == TexState.TransIdx)
		{
			TexWaves[i].Fade += TexState.TransDel * deltaTime;
			if (TexWaves[i].Fade < 0)
			{
				// This wave is faded out. Re-init and fade it back up.
				InitTexWave(i);
				TexWaves[i].Fade = 0;
				TexState.TransDel = -TexState.TransDel;
			}
			else if (TexWaves[i].Fade > 1.f)
			{
				// This wave is faded back up. Start fading another down.
				TexWaves[i].Fade = 1.f;
				TexState.TransDel = -TexState.TransDel;
				if (++TexState.TransIdx >= kNumTexWaves)
					TexState.TransIdx = 0;
			}
		}
		TexWaves[i].Phase -= deltaTime * TexWaves[i].Speed;
		TexWaves[i].Phase -= int(TexWaves[i].Phase);
	}

	static bool IsInitTexWave = false;
	if (!IsInitTexWave)
	{
		IsInitTexWave = true;

		for (int32 i = 0; i < kNumTexWaves; ++i)
			InitTexWave(i);
	}

	static auto MainRenderTarget = jRenderTargetPool::GetRenderTarget(
		{ ETextureType::TEXTURE_2D, ETextureFormat::RGBA32F, ETextureFormat::RGBA, EFormatType::FLOAT
		, EDepthBufferType::DEPTH32, SCR_WIDTH, SCR_HEIGHT, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR }
	);

	static auto RenderBumpTarget = jRenderTargetPool::GetRenderTarget(
		{ ETextureType::TEXTURE_2D, ETextureFormat::RGBA32F, ETextureFormat::RGBA, EFormatType::FLOAT
		, EDepthBufferType::DEPTH32, SCR_WIDTH, SCR_HEIGHT, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR }
	);

	if (0)
		if (RenderBumpTarget->Begin())
		{
			static Vector4 m_UTrans[16];
			static Vector4 m_Coef[16];
			static Vector4 ReScale;
			static Vector4 NoiseXform[4];

			auto TexWaveShader = jShader::GetShader("DrawTexWave");

			char szTemp[1024];
			int i;
			for (i = 0; i < 16; i++)
			{
				Vector4 UTrans(TexWaves[i].RotScale.x, TexWaves[i].RotScale.y, 0.f, TexWaves[i].Phase);
				sprintf_s(szTemp, sizeof(szTemp), "UTrans[%d]", i);
				SET_UNIFORM_BUFFER_STATIC(Vector4, szTemp, UTrans, TexWaveShader);

				float normScale = TexWaves[i].Fade / float(kNumBumpPasses);
				Vector4 Coef(TexWaves[i].Dir.x * normScale, TexWaves[i].Dir.y * normScale, 1.f, 1.f);
				sprintf_s(szTemp, sizeof(szTemp), "Coef[%d]", i);
				SET_UNIFORM_BUFFER_STATIC(Vector4, szTemp, Coef, TexWaveShader);

			}

			Vector4 xform;

			const float kRate = 0.1f;
			xform.w += deltaTime * kRate;
			SET_UNIFORM_BUFFER_STATIC(Vector4, "NoiseXform[0]", NoiseXform[0], TexWaveShader);

			xform.w += deltaTime * kRate;
			SET_UNIFORM_BUFFER_STATIC(Vector4, "NoiseXform[3]", NoiseXform[3], TexWaveShader);

			float s = 0.5f / (float(kNumBumpPerPass) + TexState.Noise);
			Vector4 reScale(s, s, 1.f, 1.f);
			SET_UNIFORM_BUFFER_STATIC(Vector4, "ReScale", ReScale, TexWaveShader);

			float scaleBias = 0.5f * TexState.Noise / (float(kNumBumpPasses) + TexState.Noise);
			Vector4 scaleBiasVec(scaleBias, scaleBias, 0.f, 1.f);
			SET_UNIFORM_BUFFER_STATIC(Vector4, "ScaleBias", scaleBiasVec, TexWaveShader);

			//m_CompCosinesEff->SetTexture(m_CompCosineParams.m_CosineLUT, m_CosineLUT);
			//m_CompCosinesEff->SetTexture(m_CompCosineParams.m_BiasNoise, m_BiasNoiseMap);

			static auto pFullscreenQuad = jPrimitiveUtil::CreateFullscreenQuad(nullptr);
			if (pFullscreenQuad)
			{
				// pFullscreenQuad->RenderObject->tex_object = CosLUT;
				// pFullscreenQuad->RenderObject->tex_object2 = BiasNoise;

				pFullscreenQuad->Update(deltaTime);
				pFullscreenQuad->Draw(MainCamera, TexWaveShader, {});
			}

			RenderBumpTarget->End();
		}

	//if (MainRenderTarget->Begin())
	{
		//SET_UNIFORM_BUFFER_STATIC(Vector, "ScatterColor", AppSettingInst.ScatterColor, Shader);

		g_rhi->BeginDebugEvent("[1]. MainScene Render");

		auto ClearColor = Vector4(0.1f, 0.1f, 0.1f, 1.0f);
		auto ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
		auto EnableDepthTest = true;
		auto DepthStencilFunc = EComparisonFunc::LESS;
		auto EnableBlend = true;
		auto BlendSrc = EBlendSrc::ONE;
		auto BlendDest = EBlendDest::ZERO;

		g_rhi->EnableDepthTest(EnableDepthTest);
		g_rhi->SetDepthFunc(DepthStencilFunc);

		g_rhi->EnableBlend(EnableBlend);
		g_rhi->SetBlendFunc(BlendSrc, BlendDest);

		g_rhi->EnableDepthBias(false);
		g_rhi->EnableCullFace(true);
		g_rhi->EnableDepthClip(false);

		g_rhi->SetClearColor(ClearColor);
		g_rhi->SetClear(ClearType);

		/// 
		static bool IsCreatedWaterMesh = false;
		static jMeshObject* pWaterMesh = nullptr;
		static jMeshObject* pLandMesh = nullptr;
		if (!IsCreatedWaterMesh)
		{
			IsCreatedWaterMesh = true;
			pWaterMesh = jModelLoader::GetInstance().LoadFromFile("model/WaterMesh.x");

			jStreamParam<float>& VertexParams
				= *static_cast<jStreamParam<float>*>(pWaterMesh->RenderObject->VertexStream->Params[0]);
			for (uint32 i = 2; i < VertexParams.Data.size(); i += 3)
				VertexParams.Data[i] = 0.0f;
			pWaterMesh->RenderObject->UpdateVertexStream();

			//pWaterMesh->RenderObject->VertexBuffer

			//pLandMesh = jModelLoader::GetInstance().LoadFromFile("model/LandMesh.x");
			//pWaterMesh->RenderObject->Rot.x = DegreeToRadian(90.0f);

			//jImageData data;
			//jImageFileLoader::GetInstance().LoadTextureFromFile(data, "Image/Sandy.png", true);
			//if (data.ImageData.size() > 0)
			//	pLandMesh->RenderObject->tex_object2 = g_rhi->CreateTextureFromData(&data.ImageData[0], data.Width, data.Height, data.sRGB);

		}

		{
			//auto shader = jShader::GetShader("DebugObjectShader");
			//DirectionalLight->BindLight(shader);
			//pLandMesh->Update(deltaTime);
			//pLandMesh->Draw(MainCamera, shader, { DirectionalLight });
		}

		if (1)
		{
			auto shader = jShader::GetShader("GeoWave");

			{
				g_rhi->SetShader(shader);
				// 1. ViewProjection Matrix
				SET_UNIFORM_BUFFER_STATIC(Matrix, "World2NDC", (MainCamera->Projection * MainCamera->View), shader);

				// 2. 물 색깔
				Vector4 waterTint(0.05f, 0.1f, 0.1f, 0.5f);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "WaterTint", waterTint, shader);

				// 3. 4개 파도의 Frequency
				Vector4 freq(GeoWaves[0].Freq, GeoWaves[1].Freq, GeoWaves[2].Freq, GeoWaves[3].Freq);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "Frequency", freq, shader);

				// 4. 4개 파도의 Phase (2PI / Lengh)
				Vector4 phase(GeoWaves[0].Phase, GeoWaves[1].Phase, GeoWaves[2].Phase, GeoWaves[3].Phase);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "Phase", phase, shader);

				// 5. 4개 파도의 Amplitude
				Vector4 amp(GeoWaves[0].Amp, GeoWaves[1].Amp, GeoWaves[2].Amp, GeoWaves[3].Amp);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "Amplitude", amp, shader);

				// 6. 4개 파도의 DirX
				Vector4 dirX(GeoWaves[0].Dir.x, GeoWaves[1].Dir.x, GeoWaves[2].Dir.x, GeoWaves[3].Dir.x);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DirX", dirX, shader);

				// 7. 4개 파도의 DirY
				Vector4 dirY(GeoWaves[0].Dir.y, GeoWaves[1].Dir.y, GeoWaves[2].Dir.y, GeoWaves[3].Dir.y);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DirY", dirY, shader);

				float normScale = GeoState.SpecAtten * TexState.AmpOverLen * 2.f * PI;
				normScale *= (float(kNumBumpPasses) + TexState.Noise);
				normScale *= (TexState.Chop + 1.f);

				// 8. ??? SpecAtten 
				Vector4 specAtten(GeoState.SpecEnd, 1.f / GeoState.SpecTrans, normScale, 1.f / TexState.RippleScale);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "SpecAtten", specAtten, shader);

				// 9. 카메라 위치
				SET_UNIFORM_BUFFER_STATIC(Vector4, "CameraPos", Vector4(MainCamera->Pos, 1.f), shader);

				Vector envCenter(0.f, 0.f, GeoState.EnvHeight); // Just happens to be centered at origin.
				Vector camToCen = envCenter - MainCamera->Pos;

				float G = camToCen.LengthSQ() - GeoState.EnvRadius * GeoState.EnvRadius;		// 0 < 이면? 환경맵 바깥, 0 > 이면? 화경맵 안.
				// 10. 카메라에서 환경맵의 중심방향의 벡터 (정규화 안함) + 환경맵 안인지 밖인지 정보 추가.
				SET_UNIFORM_BUFFER_STATIC(Vector4, "EnvAdjust", Vector4(camToCen.x, camToCen.y, camToCen.z, G), shader);

				// 11. 환경맵 색상
				Vector4 envTint(1.f, 1.f, 1.f, 1.f);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "EnvTint", envTint, shader);

				// 12. 월드 매트릭스
				SET_UNIFORM_BUFFER_STATIC(Matrix, "Local2World", pWaterMesh->RenderObject->World, shader);

				// 13. 4개 파도의 Wave Length
				Vector4 lengths(GeoWaves[0].Len, GeoWaves[1].Len, GeoWaves[2].Len, GeoWaves[3].Len);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "Lengths", lengths, shader);

				// 14. 4개 파도의 물 높이
				static Vector4 depthOffset(GeoState.WaterLevel + 1.f,
					GeoState.WaterLevel + 1.f,
					GeoState.WaterLevel + 0.f,
					GeoState.WaterLevel);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DepthOffset", depthOffset, shader);

				// 15. 4개 파도의 Depth Scale 값
				Vector4 depthScale(1.f / 2.f, 1.f / 2.f, 1.f / 2.f, 1.f);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DepthScale", depthScale, shader);

				// 16. Fog 파라메터들
				Vector4 fogParams(-200.f, 1.f / (100.f - 200.f), 0.f, 1.f);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "FogParams", fogParams, shader);

				// Equation 9 아래 항목 아래에 나오는 식과 일치 Qi = Q/(wi * Ai x numWaves)
				// Q를 0~1 사이 값으로 변경하므로써 아디스트에게 부드러운 <-> 날카로운 파도를 만들 수 있게 해준다.
				static float K = 5.f;
				if (GeoState.AmpOverLen > GeoState.Chop / (2.f * PI * kNumGeoWaves * K))
					K = GeoState.Chop / (2.f * PI * GeoState.AmpOverLen * kNumGeoWaves);
				Vector4 dirXK(GeoWaves[0].Dir.x * K,
					GeoWaves[1].Dir.x * K,
					GeoWaves[2].Dir.x * K,
					GeoWaves[3].Dir.x * K);
				Vector4 dirYK(GeoWaves[0].Dir.y * K,
					GeoWaves[1].Dir.y * K,
					GeoWaves[2].Dir.y * K,
					GeoWaves[3].Dir.y * K);

				// 17. 4개 파도의 X 방향 * 가파름 파라메터
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DirXK", dirXK, shader);

				// 18. 4개 파도의 Y 방향 * 가파름 파라메터
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DirYK", dirYK, shader);

				// 4개의 파도의 TBN 벡터들을 구하는데 사용함
				Vector4 dirXW(GeoWaves[0].Dir.x * GeoWaves[0].Freq,
					GeoWaves[1].Dir.x * GeoWaves[1].Freq,
					GeoWaves[2].Dir.x * GeoWaves[2].Freq,
					GeoWaves[3].Dir.x * GeoWaves[3].Freq);
				Vector4 dirYW(GeoWaves[0].Dir.y * GeoWaves[0].Freq,
					GeoWaves[1].Dir.y * GeoWaves[1].Freq,
					GeoWaves[2].Dir.y * GeoWaves[2].Freq,
					GeoWaves[3].Dir.y * GeoWaves[3].Freq);

				// 19. 4개의 파도의 X방향과 Frequency를 곱한 파라메터
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DirXW", dirXW, shader);

				// 20. 4개의 파도의 Y방향과 Frequency를 곱한 파라메터
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DirYW", dirYW, shader);

				Vector4 KW(K * GeoWaves[0].Freq,
					K * GeoWaves[1].Freq,
					K * GeoWaves[2].Freq,
					K * GeoWaves[3].Freq);
				// 21. 4개의 파도의 Qi (부드러움 <-> 가파름 조정) * Frequency, TBN 벡터 만드는데 사용
				SET_UNIFORM_BUFFER_STATIC(Vector4, "KW", KW, shader);

				// 22. 4개의 파도의 TBN 벡터 만드는데 사용하는 파라메터
				Vector4 dirXSqKW(GeoWaves[0].Dir.x * GeoWaves[0].Dir.x * K * GeoWaves[0].Freq,
					GeoWaves[1].Dir.x * GeoWaves[1].Dir.x * K * GeoWaves[1].Freq,
					GeoWaves[2].Dir.x * GeoWaves[2].Dir.x * K * GeoWaves[2].Freq,
					GeoWaves[3].Dir.x * GeoWaves[3].Dir.x * K * GeoWaves[3].Freq);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DirXSqKW", dirXSqKW, shader);

				// 23. 4개의 파도의 TBN 벡터 만드는데 사용하는 파라메터
				Vector4 dirYSqKW(GeoWaves[0].Dir.y * GeoWaves[0].Dir.y * K * GeoWaves[0].Freq,
					GeoWaves[1].Dir.y * GeoWaves[1].Dir.y * K * GeoWaves[1].Freq,
					GeoWaves[2].Dir.y * GeoWaves[2].Dir.y * K * GeoWaves[2].Freq,
					GeoWaves[3].Dir.y * GeoWaves[3].Dir.y * K * GeoWaves[3].Freq);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DirYSqKW", dirYSqKW, shader);

				// 24. 4개의 파도의 TBN 벡터 만드는데 사용하는 파라메터
				Vector4 dirXdirYKW(GeoWaves[0].Dir.y * GeoWaves[0].Dir.x * K * GeoWaves[0].Freq,
					GeoWaves[1].Dir.x * GeoWaves[1].Dir.y * K * GeoWaves[1].Freq,
					GeoWaves[2].Dir.x * GeoWaves[2].Dir.y * K * GeoWaves[2].Freq,
					GeoWaves[3].Dir.x * GeoWaves[3].Dir.y * K * GeoWaves[3].Freq);
				SET_UNIFORM_BUFFER_STATIC(Vector4, "DirXdirYKW", dirXdirYKW, shader);

				//pWaterMesh->RenderObject->tex_object = m_EnvMap;
				//pWaterMesh->RenderObject->tex_object2 = m_BumpTex;
			}

			//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			static float OffsetX = 0.0f;
			static float OffsetZ = -10.0f;
			pWaterMesh->RenderObject->Pos.x = OffsetX;
			pWaterMesh->RenderObject->Pos.z = OffsetZ;
			pWaterMesh->RenderObject->tex_object = CubeMapTexture;
			pWaterMesh->RenderObject->samplerState = jSamplerStatePool::GetSamplerState("LinearWrap").get();

			pWaterMesh->Update(deltaTime);
			pWaterMesh->Draw(MainCamera, shader, { DirectionalLight });

			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		static bool test = false;
		if (test)
		{
			auto shader = jShader::GetShader("CubeEnv");
			EnvCube->RenderObject->tex_object = CubeMapTexture;

			MainCamera->IsInfinityFar = true;
			MainCamera->UpdateCamera();
			EnvCube->Update(deltaTime);
			EnvCube->Draw(MainCamera, shader, { DirectionalLight });
			MainCamera->IsInfinityFar = false;
			MainCamera->UpdateCamera();
		}

		/// 

		g_rhi->EndDebugEvent();
		MainRenderTarget->End();
	}
	//////////////////////////////////////////////////////////////////////////
}

void jGame::UpdateAppSetting()
{
	auto& appSetting = jShadowAppSettingProperties::GetInstance();

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
		graph1.push_back(Vector2(i * 2, PerspectiveVector[i].z * scale));
	for (int i = 0; i < _countof(OrthographicVector); ++i)
		graph2.push_back(Vector2(i * 2, OrthographicVector[i].z * scale));

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