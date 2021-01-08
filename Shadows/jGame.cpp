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
#include "TinyXml2/tinyxml2.h"

#define WITH_REFLECTANCE 1
bool IsOnlyUseFormFactor = false;
int32 FixedFace = -1;
int32 StartShooterPatch = -1;
bool SelectedShooterPatchAsDrawLine = true;
bool WireFrame = false;

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
	static float speed = 3.0f;

	// Process Key Event
	if (g_KeyState['a'] || g_KeyState['A']) MainCamera->MoveShift(-speed);
	if (g_KeyState['d'] || g_KeyState['D']) MainCamera->MoveShift(speed);
	if (g_KeyState['1']) MainCamera->RotateForwardAxis(-0.1f);
	if (g_KeyState['2']) MainCamera->RotateForwardAxis(0.1f);
	if (g_KeyState['3']) MainCamera->RotateUpAxis(-0.1f);
	if (g_KeyState['4']) MainCamera->RotateUpAxis(0.1f);
	if (g_KeyState['5']) MainCamera->RotateRightAxis(-0.1f);
	if (g_KeyState['6']) MainCamera->RotateRightAxis(0.1f);
	if (g_KeyState['w'] || g_KeyState['W']) MainCamera->MoveForward(speed);
	if (g_KeyState['s'] || g_KeyState['S']) MainCamera->MoveForward(-speed);
	if (g_KeyState['+']) speed = Max(speed + 0.1f, 0.0f);
	if (g_KeyState['-']) speed = Max(speed - 0.1f, 0.0f);
}

void jGame::Setup()
{
	//////////////////////////////////////////////////////////////////////////
	//const Vector mainCameraPos(110.321266f, 98.3155670f, -198.161148f);
	//const Vector mainCameraTarget(110.311089f, 98.3600845f, -197.162262f);
	//const Vector mainCameraUp(110.321724f, 99.3145294f, -198.205750f);

	Vector mainCameraPos(100.015862f, 117.436371f, 401.567474f);
	Vector mainCameraTarget(100.047562f, 117.411041f, 400.568176f);
	Vector mainCameraUp(100.016640f, 118.436211f, 401.542267f);

	//mainCameraPos.x = 249.691452; mainCameraPos.y = 186.690109; mainCameraPos.z = 104.297935;
	//mainCameraTarget.x = 248.695450; mainCameraTarget.y = 186.614731; mainCameraTarget.z = 104.345070;
	//mainCameraUp.x = 249.616226; mainCameraUp.y = 187.687225; mainCameraUp.z = 104.301460;

	MainCamera = jCamera::CreateCamera(mainCameraPos, mainCameraTarget, mainCameraUp, DegreeToRadian(60.0f), 10.0f, 5000.0f, SCR_WIDTH, SCR_HEIGHT, true);
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

namespace Radiosity
{
	struct Quad
	{
		int32 Verts[4] = { 0, 0, 0, 0 };
		int32 PatchLevel = 0;
		int32 ElementLevel = 0;
		int32 Area = 0;
		Vector Normal = Vector::ZeroVector;
		Vector Reflectance = Vector::ZeroVector;
		Vector Emission = Vector::ZeroVector;
	};

	struct Patch
	{
		int32 QuadID = -1;
		Vector Reflectance = Vector::ZeroVector;
		Vector Emission = Vector::ZeroVector;
		Vector Center = Vector::ZeroVector;
		Vector Normal = Vector::ZeroVector;
		Vector UnShotRadiosity = Vector::ZeroVector;
		double Area = 0.0f;
		struct Element* RootElement = nullptr;

		struct LookAtAndUp
		{
			Vector LookAt[5];
			Vector Up[5];
		};

		LookAtAndUp GenerateHemicubeLookAtAndUp() const
		{
			return GenerateHemicubeLookAtAndUp(Center);
		}

		LookAtAndUp GenerateHemicubeLookAtAndUp(Vector InCenter) const
		{
			// hemi-cube를 Patch의 normal 축을 기준으로 랜덤하게 회전시킨다.
			// 이 것이 hemi-cube의 앨리어싱 아티팩트를 줄여준다.

			Vector TangentU = Vector::ZeroVector;
			//if (Vector::ZeroVector == TangentU)
			{
				do
				{
					//Vector RandVec(((float)rand() / float(RAND_MAX)), ((float)rand() / float(RAND_MAX)), ((float)rand() / float(RAND_MAX)));
					Vector RandVec = Vector::UpVector;
					auto temp = Normal.DotProduct(RandVec);
					if (fabs(temp) > 0.9)
						RandVec = Vector::FowardVector;
					TangentU = Normal.CrossProduct(RandVec).GetNormalize();

				} while (TangentU.Length() == 0);
			}

			// TangentV 를 계산
			Vector TangentV = Normal.CrossProduct(TangentU).GetNormalize();

			// Hemicube face를 계산한다.
			LookAtAndUp Result;

			Result.LookAt[0] = InCenter + Normal;
			Result.Up[0] = InCenter + TangentU;

			Result.LookAt[1] = InCenter + TangentU;
			Result.Up[1] = InCenter + Normal;

			Result.LookAt[2] = InCenter + TangentV;
			Result.Up[2] = InCenter + Normal;

			Result.LookAt[3] = InCenter - TangentU;
			Result.Up[3] = InCenter + Normal;

			Result.LookAt[4] = InCenter - TangentV;
			Result.Up[4] = InCenter + Normal;

			return Result;
		}
	};

	struct Element
	{
		int32 ID = -1;
		std::vector<int32> Indices;
		Vector Normal = Vector::ZeroVector;
		Vector Radiosity = Vector::ZeroVector;
		double Area = 0.0;
		Patch* ParentPatch = nullptr;

		bool HasUseChild() const
		{
			for (int32 i = 0; i < 4; ++i)
			{
				if (UseChild[i])
					return true;
			}
			return false;
		}

		template <typename T>
		void IterateOnlyLeaf(T func)
		{
			if (HasUseChild())
			{
				for (int32 i = 0; i < 4; ++i)
				{
					if (UseChild[i] && ChildElement[i])
						ChildElement[i]->IterateOnlyLeaf(func);
				}
				return;
			}

			func(this);
		}

		template <typename T>
		void Iterate(T func)
		{
			func(this);

			for (int32 i = 0; i < 4; ++i)
			{
				if (ChildElement[i])
					ChildElement[i]->Iterate(func);
			}
		}
		
		bool Subdivide()
		{
			bool IsSubDivided = false;
			for (int32 i = 0; i < 4; ++i)
			{
				if (ChildElement[i])
				{
					IsSubDivided = true;
					UseChild[i] = true;

					Radiosity = Vector::ZeroVector;
					ChildElement[i]->Radiosity = Radiosity;
				}
			}
			return IsSubDivided;
		}

		void GenerateEmptyChildElement()
		{
			for (int32 i = 0; i < 4; ++i)
			{
				if (ChildElement[i])
					ChildElement[i]->GenerateEmptyChildElement();
				else
					ChildElement[i] = new Radiosity::Element();
			}
		}

		static Radiosity::Element* GetNextEmptyElement(Radiosity::Element* InElement)
		{
			if (InElement->ID == -1)
				return InElement;

			for (int32 i = 0; i < 4; ++i)
			{
				if (!InElement->ChildElement[i])
					continue;

				auto Ret = InElement->GetNextEmptyElement(InElement->ChildElement[i]);
				if (Ret)
					return Ret;
			}

			return nullptr;
		}

		bool UseChild[4] = { false, false, false, false };
		Element* ChildElement[4] = { nullptr, nullptr, nullptr, nullptr };
	};

	struct InputParams
	{
		~InputParams()
		{
			delete DisplayCamera;
			delete HemicubeCamera;
		}

		double Threshold = 0.0001;
		std::vector<Patch> Patches;
		std::vector<Vector> AllVertices;
		std::vector<Vector> AllColors;
		jCamera* DisplayCamera = nullptr;
		uint32 HemicubeResolution = 512;
		float WorldSize = 500.0f;
		float IntensityScale = 20.0f;
		int32 AddAmbient = 0;

		double TotalEnergy = 0.0;
		std::vector<double> Formfactors;
		jCamera* HemicubeCamera = nullptr;

		std::vector<double> TopFactors;
		std::vector<double> SideFactors;
		std::vector<float> CurrentBuffer;

		int32 TotalElementCount = 0;
	};

	Vector red = { 0.80f, 0.10f, 0.075f };
	Vector yellow = { 0.9f, 0.8f, 0.1f };
	Vector blue = { 0.075f, 0.10f, 0.35f };
	Vector green = { 0.075f, 0.8f, 0.1f };
	//Vector red = { 1.0f, 0.0f, 0.0f };
	//Vector yellow = { 1.0f, 1.0f, 0.0f };
	//Vector blue = { 0.0f, 0.0f, 1.0f };
	//Vector green = { 0.0f, 1.0f, 0.0f };
	Vector white = { 1.0f, 1.0f, 1.0f };
	Vector lightGrey = { 0.9f, 0.9f, 0.9f };
	Vector black = { 0.0f, 0.0f, 0.0f };

#define numberOfPolys 	19
	//Quad roomPolys[numberOfPolys] = {
	//	{{4, 5, 6, 7},		2, 8, 216 * 215, {0, -1, 0}, lightGrey, black}, /* ceiling */
	//	{{0, 3, 2, 1},		3, 8, 216 * 215, {0, 1, 0}, lightGrey, black}, /* floor */
	//	{{0, 4, 7, 3},		2, 8, 221 * 215, {1, 0, 0}, red, black}, /* wall */
	//	{{0, 1, 5, 4},		2, 8, 221 * 216, {0, 0, 1}, lightGrey, black}, /* wall */
	//	{{2, 6, 5, 1},		2, 8, 221 * 215, {-1, 0, 0}, green, black}, /* wall */
	//	{{2, 3, 7, 6},		2, 8, 221 * 216, {0, 0,-1}, lightGrey, black}, /* ADDED wall */
	//	{{8, 9, 10, 11},	2, 1, 40 * 45, {0, -1, 0}, black, white}, /* light */
	//	{{16, 19, 18, 17},	1, 5, 65 * 65, {0, 1, 0}, yellow, black}, /* box 1 */
	//	{{12, 13, 14, 15},	1, 1, 65 * 65, {0, -1, 0}, yellow, black},
	//	{{12, 15, 19, 16},	1, 5, 65 * 65, {-0.866, 0, -0.5}, yellow, black},
	//	{{12, 16, 17, 13},	1, 5, 65 * 65, {0.5, 0, -0.866}, yellow, black},
	//	{{14, 13, 17, 18},	1, 5, 65 * 65, {0.866, 0, 0.5}, yellow, black},
	//	{{14, 18, 19, 15},	1, 5, 65 * 65, {-0.5, 0, 0.866}, yellow, black},
	//	{{24, 27, 26, 25},	1, 5, 65 * 65, {0, 1, 0}, lightGrey, black}, /* box 2 */
	//	{{20, 21, 22, 23},	1, 1, 65 * 65, {0, -1, 0}, lightGrey, black},
	//	{{20, 23, 27, 24},	1, 6, 65 * 130, {-0.866, 0, -0.5}, lightGrey, black},
	//	{{20, 24, 25, 21},	1, 6, 65 * 130, {0.5, 0, -0.866}, lightGrey, black},
	//	{{22, 21, 25, 26},	1, 6, 65 * 130, {0.866, 0, 0.5}, lightGrey, black},
	//	{{22, 26, 27, 23},	1, 6, 65 * 130, {-0.5, 0, 0.866}, lightGrey, black},
	//};
	Quad roomPolys[numberOfPolys] = {
		{{4, 5, 6, 7},		8, 1, 216 * 215, {0, -1, 0}, lightGrey, black}, /* ceiling */
		{{0, 3, 2, 1},		8, 1, 216 * 215, {0, 1, 0}, lightGrey, black}, /* floor */
		{{0, 4, 7, 3},		8, 1, 221 * 215, {1, 0, 0}, red, black}, /* wall */
		{{0, 1, 5, 4},		8, 1, 221 * 216, {0, 0, 1}, lightGrey, black}, /* wall */
		{{2, 6, 5, 1},		8, 1, 221 * 215, {-1, 0, 0}, green, black}, /* wall */
		{{2, 3, 7, 6},		8, 1, 221 * 216, {0, 0,-1}, lightGrey, black}, /* ADDED wall */
		{{8, 9, 10, 11},	2, 1, 40 * 45, {0, -1, 0}, black, white}, /* light */
		{{16, 19, 18, 17},	2, 1, 65 * 65, {0, 1, 0}, yellow, black}, /* box 1 */
		{{12, 13, 14, 15},	1, 1, 65 * 65, {0, -1, 0}, yellow, black},
		{{12, 15, 19, 16},	4, 1, 65 * 65, {-0.866, 0, -0.5}, yellow, black},
		{{12, 16, 17, 13},	4, 1, 65 * 65, {0.5, 0, -0.866}, yellow, black},
		{{14, 13, 17, 18},	4, 1, 65 * 65, {0.866, 0, 0.5}, yellow, black},
		{{14, 18, 19, 15},	4, 1, 65 * 65, {-0.5, 0, 0.866}, yellow, black},
		{{24, 27, 26, 25},	4, 1, 65 * 65, {0, 1, 0}, lightGrey, black}, /* box 2 */
		{{20, 21, 22, 23},	1, 1, 65 * 65, {0, -1, 0}, lightGrey, black},
		{{20, 23, 27, 24},	4, 1, 65 * 130, {-0.866, 0, -0.5}, lightGrey, black},
		{{20, 24, 25, 21},	4, 1, 65 * 130, {0.5, 0, -0.866}, lightGrey, black},
		{{22, 21, 25, 26},	4, 1, 65 * 130, {0.866, 0, 0.5}, lightGrey, black},
		{{22, 26, 27, 23},	4, 1, 65 * 130, {-0.5, 0, 0.866}, lightGrey, black},
	};

	Vector roomPoints[] = {
	{ 0, 0, 0 },
	{ 216, 0, 0 },
	{ 216, 0, 215 },
	{ 0, 0, 215 },
	{ 0, 221, 0 },
	{ 216, 221, 0 },
	{ 216, 221, 215 },
	{ 0, 221, 215 },

	{ 85.5, 220, 90 },
	{ 130.5, 220, 90 },
	{ 130.5, 220, 130 },
	{ 85.5, 220, 130 },

	{ 53.104, 0, 64.104 },
	{ 109.36, 0, 96.604 },
	{ 76.896, 0, 152.896 },
	{ 20.604, 0, 120.396 },
	{ 53.104, 65, 64.104 },
	{ 109.36, 65, 96.604 },
	{ 76.896, 65, 152.896 },
	{ 20.604, 65, 120.396 },

	{ 134.104, 0, 67.104 },
	{ 190.396, 0, 99.604 },
	{ 157.896, 0, 155.896 },
	{ 101.604, 0, 123.396 },
	{ 134.104, 130, 67.104 },
	{ 190.396, 130, 99.604 },
	{ 157.896, 130, 155.896 },
	{ 101.604, 130, 123.39 },
	};
}

Vector UVToXYZ(const Vector (&InQuad)[4], float InU, float InV)
{
	Vector Result;

	float InvU = 1.0f - InU;
	float InvV = 1.0f - InV;

	Result.x = InQuad[0].x * InvU * InvV + InQuad[1].x * InvU * InV + InQuad[2].x * InU * InV + InQuad[3].x * InU * InvV;
	Result.y = InQuad[0].y * InvU * InvV + InQuad[1].y * InvU * InV + InQuad[2].y * InU * InV + InQuad[3].y * InU * InvV;
	Result.z = InQuad[0].z * InvU * InvV + InQuad[1].z * InvU * InV + InQuad[2].z * InU * InV + InQuad[3].z * InU * InvV;

	return Result;
}

Radiosity::InputParams* InitInputParams()
{
	Radiosity::InputParams* Params = new Radiosity::InputParams();;

	int32 NumOfPatches = 0;
	for (int32 i = numberOfPolys - 1; i >= 0;--i)
	{
		NumOfPatches += Radiosity::roomPolys[i].PatchLevel * Radiosity::roomPolys[i].PatchLevel;
	}
	Params->Patches.resize(NumOfPatches);

	int32 NumOfElement = 0;
	for (int32 i = numberOfPolys - 1; i >= 0; --i)
	{
		const int32 ElementWidth = Radiosity::roomPolys[i].ElementLevel * Radiosity::roomPolys[i].PatchLevel;
		NumOfElement += ElementWidth * ElementWidth;
	}

	int32 NumOfVertices = 0;
	for (int32 i = numberOfPolys - 1; i >= 0; --i)
	{
		const int32 VerticesWidth = Radiosity::roomPolys[i].ElementLevel * Radiosity::roomPolys[i].PatchLevel + 1;
		NumOfVertices += VerticesWidth * VerticesWidth;
	}
	Params->AllVertices.resize(NumOfVertices);
	Params->AllColors.resize(NumOfVertices, Vector::ZeroVector);

	static int32 ID_Gen = 0;

	int32 VertexIndex = 0;
	int32 ElementIndex = 0;
	int32 PatchIndex = 0;
	int32 VertexOffset = 0;
	for (int32 i = 0; i < numberOfPolys; ++i)
	{
		const Radiosity::Quad& CurQuad = Radiosity::roomPolys[i];

		Vector Vertices[4];

		for (int32 k = 0; k < 4; ++k)
			Vertices[k] = Radiosity::roomPoints[CurQuad.Verts[k]];

		int32 NumOfCurQuadVertices = 0;

		// Element의 Vertex 계산
		{
			// 가로 세로 버택스 수
			int32 nu = CurQuad.PatchLevel * CurQuad.ElementLevel + 1;
			int32 nv = CurQuad.PatchLevel * CurQuad.ElementLevel + 1;

			// u, v step.
			double du = 1.0 / (nu - 1);
			double dv = 1.0 / (nv - 1);

			for (int32 k = 0; k < nu; ++k)
			{
				double u = k * du;
				for (int32 m = 0; m < nv; ++m)
				{
					double v = m * dv;
					Params->AllVertices[VertexIndex++] = UVToXYZ(Vertices, u, v);
					++NumOfCurQuadVertices;
				}
			}
		}

		int32 PrvPatchIndex = PatchIndex;

		std::vector<int32> CurrentGeneratedPatches;
		// Patch 계산
		{
			int32 nu = CurQuad.PatchLevel;
			int32 nv = CurQuad.PatchLevel;
			double du = 1.0 / nu;
			double dv = 1.0 / nv;

			for (int32 k = 0; k < nu; ++k)
			{
				double u = k * du + du / 2.0;
				for (int32 m = 0; m < nv; ++m)
				{
					double v = m * dv + dv / 2.0;
					Radiosity::Patch& CurPatch = Params->Patches[PatchIndex];
					CurPatch.QuadID = i;
					CurPatch.Center = UVToXYZ(Vertices, u, v);
					CurPatch.Normal = CurQuad.Normal;
					CurPatch.Reflectance = CurQuad.Reflectance;
					CurPatch.Emission = CurQuad.Emission;
					CurPatch.Area = CurQuad.Area / (nu * nv);
					CurPatch.RootElement = new Radiosity::Element();
					CurrentGeneratedPatches.push_back(PatchIndex);
					++PatchIndex;
				}
			}
		}

		// Element 계산
		//const int32 PatchCnt = CurQuad.PatchLevel * CurQuad.PatchLevel;
		//for(int32 PatchIndex = 0;PatchIndex < PatchCnt;++PatchIndex)
		{
			int32 nu = CurQuad.PatchLevel * CurQuad.ElementLevel;
			int32 nv = CurQuad.PatchLevel * CurQuad.ElementLevel;
			double du = 1.0 / nu;
			double dv = 1.0 / nv;

			int32 CurLevel = CurQuad.ElementLevel;
			//Params->Elements.resize(NumOfElement);

			//Params->RootElements.push_back(Radiosity::Element());
			//Radiosity::Element* RootElement = &Params->RootElements[Params->RootElements.size() - 1];

			//std::vector<Radiosity::Element*> CurElements;
			//CurElements.push_back(RootElement);

			while (CurLevel > 0)
			{
				const int32 CurAreaWidth = (nu / CurLevel);
				const int32 CurAreaHeight = (nv / CurLevel);
				const int32 CurrentArea = CurQuad.Area / (CurAreaWidth * CurAreaHeight);

				int cnt = 0;
				for (int32 k = 0; k < nu; k += CurLevel)
				{
					double u = k * du + du / 2.0;
					for (int32 m = 0; m < nv; m += CurLevel, ++cnt)
					{
						double v = m * dv + dv / 2.0;

						int curCnt = cnt;
						int Multi = 1;
						int x = 0;
						int y = 0;
						while (curCnt)
						{
							y += !!(curCnt & 0x1) * Multi;
							x += !!(curCnt & 0x2) * Multi;
							curCnt = curCnt >> 2;
							Multi *= 2;
						}
						x *= CurLevel;
						y *= CurLevel;

						float fi = y / (float)nv;
						float fj = x / (float)nu;
						int32 pi = (int32)(fi * CurQuad.PatchLevel);
						int32 pj = (int32)(fj * CurQuad.PatchLevel);
						const int32 CurrentPatchIndex = PrvPatchIndex + pi * CurQuad.PatchLevel + pj;
						JASSERT(Params->Patches.size() > CurrentPatchIndex);
						Radiosity::Patch* CurrentPatch = &Params->Patches[CurrentPatchIndex];

						Radiosity::Element* CurElement = CurrentPatch->RootElement->GetNextEmptyElement(CurrentPatch->RootElement);
						//if (CurLevel == Level)
						//{
						//	CurElement = CurElements[0];
						//}
						//else
						//{
						//	for (auto it = CurElements.begin(); it != CurElements.end(); ++it)
						//	{
						//		if ((*it)->ID == -1)
						//		{
						//			CurElement = *it;
						//			break;
						//		}
						//	}
						//}
						JASSERT(CurElement);

						//Radiosity::Element& CurElement = Params->Elements[ElementIndex++];
						CurElement->ID = ID_Gen++;
						CurElement->Normal = CurQuad.Normal;
						CurElement->Indices.resize(4);

						CurElement->Indices[0] = VertexOffset + (y * (nv + 1) + x);
						CurElement->Indices[1] = VertexOffset + (y * (nv + 1) + (x + CurLevel));
						CurElement->Indices[2] = VertexOffset + ((y + CurLevel) * (nv + 1) + x);
						CurElement->Indices[3] = VertexOffset + ((y + CurLevel) * (nv + 1) + (x + CurLevel));

						CurElement->Area = CurrentArea;

						CurElement->ParentPatch = CurrentPatch;
					}
				}
				CurLevel /= 2;

				if (CurLevel > 0)
				{
					//std::vector<Radiosity::Element*> TempElement;
					//TempElement.swap(CurElements);
					//for (auto it = TempElement.begin(); it != TempElement.end(); ++it)
					//{
					//	auto Elem = *it;
					//	for (int w = 0; w < 4; ++w)
					//	{
					//		Elem->ChildElement[w] = new Radiosity::Element();
					//		CurElements.push_back(Elem->ChildElement[w]);
					//	}
					//}
					for (int32 Index : CurrentGeneratedPatches)
						Params->Patches[Index].RootElement->GenerateEmptyChildElement();
				}
			}
		}

		VertexOffset += NumOfCurQuadVertices;
	}

	Params->TotalElementCount = ID_Gen;

	return Params;
}

void InitRadiosity(Radiosity::InputParams* InParams)
{
	// Hemicube Resolution이 짝수이도록 함.
	float Res = (int32)((InParams->HemicubeResolution / 2.0 + 0.5)) * 2;
	InParams->HemicubeCamera = jCamera::CreateCamera(Vector::ZeroVector, Vector::ZeroVector, Vector::ZeroVector
		, DegreeToRadian(90.0f), InParams->WorldSize * 0.001f, InParams->WorldSize, Res, Res, true);

	/* take advantage of the symmetry in the delta form-factors */
	InParams->TopFactors.resize(Res * Res / 4, 0.0);
	// 윗면을 위한 Delta form-factors
	// 1/4의 폼팩터만 필요함, 왜냐하면 4-way symmetry 라서(?)
	{
		int32 Index = 0;
		double halfRes = Res / 2.0;
		for (int32 i = 0; i < halfRes; ++i)
		{
			double di = i;
			double ySq = (halfRes - (di + 0.5)) / halfRes;
			ySq *= ySq;
			for (int32 k = 0; k < halfRes; ++k)
			{
				double dk = k;
				double xSq = (halfRes - (dk + 0.5)) / halfRes;
				xSq *= xSq;
				double xy1Sq = xSq + ySq + 1.0;
				xy1Sq *= xy1Sq;
				InParams->TopFactors[Index++] = 1.0 / (xy1Sq * PI * halfRes * halfRes);
			}
		}
	}

	InParams->SideFactors.resize(Res * Res / 4, 0.0);

	// 옆면을 위한 Delta form-factors
	// 1/4의 폼팩터만 필요함, 왜냐하면 4-way symmetry 라서(?)
	{
		int32 Index = 0;
		double halfRes = Res / 2.0;
		for (int32 i = 0; i < halfRes; ++i)
		{
			double di = i;
			double y = (halfRes - (di-0.5)) / halfRes;	// ((halfRes - 0.5) / HalfRes), ((halfRes - 1.5) / HalfRes), ... (1.5 / HalfRes), (0.5 / HalfRes)
			double ySq = y * y;
			for (int32 k = 0; k < halfRes; ++k)
			{
				double dk = k;
				double x = (halfRes - (dk + 0.5)) / halfRes;
				double xSq = x * x;
				double xy1Sq = xSq + ySq + 1.0;
				xy1Sq *= xy1Sq;
				InParams->SideFactors[Index++] = y / (xy1Sq * PI * halfRes * halfRes);
			}
		}
	}

	InParams->Formfactors.resize(InParams->TotalElementCount);

	// Initialize Raidosity
	for (int32 i = 0; i < InParams->Patches.size(); ++i)
		InParams->Patches[i].UnShotRadiosity = InParams->Patches[i].Emission;

	for (int32 i = 0; i < InParams->Patches.size(); ++i)
	{
		InParams->Patches[i].RootElement->IterateOnlyLeaf([](Radiosity::Element* InElement)
		{
			InElement->Radiosity = InElement->ParentPatch->Emission;
		});
	}

	// 총 에너지 계산
	InParams->TotalEnergy = 0.0;
	for (int32 i = 0; i < InParams->Patches.size(); ++i)
		InParams->TotalEnergy += InParams->Patches[i].Emission.DotProduct(Vector(InParams->Patches[i].Area));
}

int32 FindShootPatch(Radiosity::InputParams* InParams)
{
	int32 MaxEnergyPatchIndex = -1;
	double MaxEnergySum = 0.0;
	for (int32 i = 0; i < InParams->Patches.size(); ++i)
	{
		double energySum = 0.0;
		energySum += InParams->Patches[i].UnShotRadiosity.DotProduct(Vector(InParams->Patches[i].Area));

		if (energySum > MaxEnergySum)
		{
			MaxEnergyPatchIndex = i;
			MaxEnergySum = energySum;
		}
	}

	double error = MaxEnergySum / InParams->TotalEnergy;
	if (error < InParams->Threshold)
		return -1;		// converged;

	return MaxEnergyPatchIndex;
}

void SumFactors(Radiosity::InputParams* InParams, bool InIsTop)
{
#define kBackgroundItem -1

	int32 ResX = InParams->HemicubeCamera->Width;
	int32 ResY = InParams->HemicubeCamera->Height;
	int32 StartY = 0;
	int32 EndY = ResY;
	if (!InIsTop)
		StartY = ResY / 2;

	const std::vector<double>& CurrentFactors = (InIsTop ? InParams->TopFactors : InParams->SideFactors);

	int32 HalfResX = ResX / 2;
	for (int32 i = StartY; i < EndY; ++i)
	{
		int32 indexY;
		if (i < HalfResX)
			indexY = i;		// 0, 1, 2, 3 ... HalfResX-1
		else
			indexY = (HalfResX - 1 - (i % HalfResX));	// HalfResX-1, HalfResX-2, HalfResX-3 ... 2, 1, 0
		indexY *= HalfResX;

		register unsigned long int current_backItem = kBackgroundItem;

		for (int32 k = 0; k < ResX; ++k)
		{
			int32 CurrentID = InParams->CurrentBuffer[i * ResX + k];
			if (current_backItem != CurrentID)
			{
				int32 indexX;
				if (k < HalfResX)
					indexX = k;
				else
					indexX = (HalfResX - 1 - (k % HalfResX));
				JASSERT(InParams->Formfactors.size() > CurrentID);
				JASSERT(CurrentFactors.size() > (indexX + indexY));
				InParams->Formfactors[CurrentID] += CurrentFactors[indexX + indexY];
			}
		}
	}
}

void ComputeFormfactors(int32 InShootPatchIndex, Radiosity::InputParams* InParams)
{
	Radiosity::Patch& ShootPatch = InParams->Patches[InShootPatchIndex];

	// 슈팅 패치를 얻음
	Vector Center = ShootPatch.Center;
	Vector Normal = ShootPatch.Normal;
	//jPlane Plane(Normal, -(Normal.DotProduct(Center)));

	Radiosity::Patch::LookAtAndUp lookAtAndUp = ShootPatch.GenerateHemicubeLookAtAndUp();

	for (int32 i = 0; i < InParams->Formfactors.size(); ++i)
		InParams->Formfactors[i] = 0.0;

	// hemicube 를 Shooting Patch의 중심보다 살짝 위에 위치시킴.
	//InParams->HemicubeCamera->Pos = Center + (Normal * InParams->WorldSize * 0.00000001);

	static std::shared_ptr<jRenderTarget> RenderTarget = jRenderTargetPool::GetRenderTarget(
		{ ETextureType::TEXTURE_2D, ETextureFormat::R32F, ETextureFormat::R, EFormatType::FLOAT
		, EDepthBufferType::DEPTH24, InParams->HemicubeCamera->Width, InParams->HemicubeCamera->Height, 1 });

	char szTemp[1024];
	for (int32 face = 0; face < 5; ++face)
	{
		if (FixedFace != -1)
			face = FixedFace;

		sprintf_s(szTemp, sizeof(szTemp), "ComputeFormfactors Face : %d", face);
		SCOPE_DEBUG_EVENT(g_rhi, szTemp);
		
		Vector Offset = Vector::ZeroVector;// (lookAtAndUp.LookAt[face] - ShootPatch.Center).GetNormalize()* (sqrt(ShootPatch.Area) * 0.25f);
		InParams->HemicubeCamera->Target = lookAtAndUp.LookAt[face] + Offset;
		InParams->HemicubeCamera->Up = lookAtAndUp.Up[face] + Offset;
		InParams->HemicubeCamera->Pos = ShootPatch.Center + Offset;

		InParams->HemicubeCamera->UpdateCamera();
		InParams->HemicubeCamera->IsEnableCullMode = true;
		if (RenderTarget->Begin())
		{
			g_rhi->SetClearColor(-1.0f, -1.0f, -1.0f, 1.0f);
			g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });
			g_rhi->EnableCullFace(true);
			g_rhi->EnableDepthTest(true);
			
			jShader* shader = jShader::GetShader("RadiosityQuadID");
			g_rhi->SetShader(shader);
			SET_UNIFORM_BUFFER_STATIC(Vector, "CameraPos", InParams->HemicubeCamera->Pos, shader);
			SET_UNIFORM_BUFFER_STATIC(Matrix, "MVP", InParams->HemicubeCamera->GetViewProjectionMatrix(), shader);
			SET_UNIFORM_BUFFER_STATIC(Matrix, "M", Matrix(IdentityType), shader);

			for (int32 i = 0; i < InParams->Patches.size(); ++i)
			{
				InParams->Patches[i].RootElement->IterateOnlyLeaf([&](Radiosity::Element* InElement)
				{
					if (!InElement)
						return;

					static std::vector<Vector> Vertices(4, Vector::ZeroVector);

					if (InElement->ParentPatch == &ShootPatch)
						return;

					for (int32 k = 0; k < InElement->Indices.size(); ++k)
					{
						int32 Index = InElement->Indices[k];
						Vertices[k] = (InParams->AllVertices[Index]);
					}

					SET_UNIFORM_BUFFER_STATIC(int32, "ID", InElement->ID, shader);
					SET_UNIFORM_BUFFER_STATIC(Vector, "Normal", InElement->Normal, shader);

					SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[0]", Vertices[0], shader);
					SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[1]", Vertices[1], shader);
					SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[2]", Vertices[2], shader);
					SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[3]", Vertices[3], shader);

					g_rhi->DrawArrays(EPrimitiveType::POINTS, 0, 1);
				});
			}
			RenderTarget->End();
		}

		InParams->CurrentBuffer.resize(RenderTarget->Info.Width * RenderTarget->Info.Height);
		jTexture_OpenGL* Tex = (jTexture_OpenGL*)RenderTarget->GetTexture();
		glBindTexture(GL_TEXTURE_2D, Tex->TextureID);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_FLOAT, (GLvoid*)(&InParams->CurrentBuffer[0]));

		/* get formfactors */
		if (face == 0)
		{
			SumFactors(InParams, true);
		}
		else
		{
			SumFactors(InParams, false);
		}

		if (FixedFace != -1)
			break;
	}

	// reciprocal form-factor 계산
	for (int32 i = 0; i < InParams->Patches.size();++i)
	{
		InParams->Patches[i].RootElement->IterateOnlyLeaf([&](Radiosity::Element* InElement)
		{
			int32 index = InElement->ID;
			InParams->Formfactors[index] *= ShootPatch.Area / InElement->Area;

			/* This is a potential source of hemi-cube aliasing */
			/* To do this right, we need to subdivide the shooting patch
			and reshoot. For now we just clip it to unity */
			if (InParams->Formfactors[index] > 1.0)
				InParams->Formfactors[index] = 1.0;
		});
	}
}

void DistributeRadiosity(int32 InShootPatchIndex, Radiosity::InputParams* InParams)
{
	Radiosity::Patch& ShootPatch = InParams->Patches[InShootPatchIndex];
	/* distribute unshotRad to every element */
	for (int32 i = 0; i < InParams->Patches.size(); ++i)
	{
		InParams->Patches[i].RootElement->IterateOnlyLeaf([&](Radiosity::Element* InElement)
		{
			const int32 index = InElement->ID;

			JASSERT(InParams->Formfactors.size() > index);

			if (InParams->Formfactors[index] == 0.0)
				return;

#if WITH_REFLECTANCE
			Vector DeltaRadiosity = ShootPatch.UnShotRadiosity * (InParams->Formfactors[index] * InElement->ParentPatch->Reflectance);
#else
			Vector DeltaRadiosity = ShootPatch.UnShotRadiosity * (InParams->Formfactors[index]);
#endif
			if (IsOnlyUseFormFactor)
				DeltaRadiosity = Vector(InParams->Formfactors[index]);

			/* incremental element's radiosity and patch's unshot radiosity */
			double w = InElement->Area / InElement->ParentPatch->Area;
			InElement->Radiosity += DeltaRadiosity;
			InElement->ParentPatch->UnShotRadiosity += DeltaRadiosity * w;
		});
	}

	/* reset shooting patch's unshot radiosity */
	ShootPatch.UnShotRadiosity = Radiosity::black;
}

Vector GetAmbient(Radiosity::InputParams* InParams)
{
	Vector Ambient = Radiosity::black;
	Vector Sum = Radiosity::black;
	static Vector OverallInterreflectance = Radiosity::black;
	static int first = 1;
	static double AreaSum = 0.0f;
	if (first) {
		Vector AverageReflectance = Radiosity::black;
		Vector rSum;
		rSum = Radiosity::black;
		/* sum area and (area*reflectivity) */
		for (int32 i = 0; i < InParams->Patches.size(); ++i)
		{
			AreaSum += InParams->Patches[i].Area;
			rSum += InParams->Patches[i].Reflectance * InParams->Patches[i].Area;
		}
		AverageReflectance = rSum / AreaSum;
		OverallInterreflectance = 1.0f / (1.0f - AverageReflectance);
		first = 0;
	}

	/* sum (unshot radiosity * area) */
	for (int32 i = 0; i < InParams->Patches.size(); ++i)
	{
		Sum += InParams->Patches[i].UnShotRadiosity * (InParams->Patches[i].Area / AreaSum);
	}

	/* compute ambient */
	Ambient = OverallInterreflectance * Sum;

	return Ambient;
}

void DrawElement(Radiosity::InputParams* InParams, Radiosity::Element* InElement, const std::vector<int32>& SummedCount, jShader* shader, bool IsSelected = false)
{
	static Vector Vertices[4];
	static Vector4 Colors[4];
	if (InElement->HasUseChild())
	{
		for (int32 i = 0; i < 4; ++i)
		{
			DrawElement(InParams, InElement->ChildElement[i], SummedCount, shader, IsSelected);
		}
	}
	else
	{
		Vector Normal = InElement->Normal;
		for (int32 k = 0; k < InElement->Indices.size(); ++k)
		{
			int32 Index = InElement->Indices[k];
			Vertices[k] = (InParams->AllVertices[Index]);
			Colors[k] = Vector4(InParams->AllColors[Index] / (float)SummedCount[Index], 1.0);
		}

		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[0]", Vertices[0], shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[1]", Vertices[1], shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[2]", Vertices[2], shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[3]", Vertices[3], shader);

		if (SelectedShooterPatchAsDrawLine && IsSelected)
		{
			SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[0]", Vector4::OneVector, shader);
			SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[1]", Vector4::OneVector, shader);
			SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[2]", Vector4::OneVector, shader);
			SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[3]", Vector4::OneVector, shader);
		}
		else
		{
			SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[0]", Colors[0], shader);
			SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[1]", Colors[1], shader);
			SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[2]", Colors[2], shader);
			SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[3]", Colors[3], shader);
		}

		g_rhi->DrawArrays(EPrimitiveType::POINTS, 0, 1);
	}
};

void DisplayResults(Radiosity::InputParams* InParams, int32 InSelectedPatch = -1)
{
	SCOPE_PROFILE(DisplayResults);
	Vector Ambient = GetAmbient(InParams);

	InParams->DisplayCamera->UpdateCamera();

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });
	g_rhi->EnableCullFace(true);
	g_rhi->EnableDepthTest(true);
	if (WireFrame)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	static std::vector<Vector> Vertices(4, Vector::ZeroVector);
	static std::vector<Vector4> Colors(4, Vector4::ZeroVector);
	static std::vector<int32> SummedCount(InParams->AllVertices.size(), 0);
	
	{
		memset(&InParams->AllColors[0], 0, InParams->AllColors.size() * sizeof(Vector));
		memset(&SummedCount[0], 0, SummedCount.size() * sizeof(int32));

		for (int32 i = 0; i < InParams->Patches.size(); ++i)
		{
			InParams->Patches[i].RootElement->IterateOnlyLeaf([&](Radiosity::Element* InElement)
			{
				Vector Color;
#if WITH_REFLECTANCE
				if (InParams->AddAmbient)
					Color = (InElement->Radiosity + (Ambient * InElement->ParentPatch->Reflectance)) * InParams->IntensityScale;
				else
#endif
					Color = InElement->Radiosity * InParams->IntensityScale;

				//Color = InElement->Radiosity * InParams->IntensityScale;

				for (int32 k = 0; k < InElement->Indices.size(); ++k)
				{
					int32 Index = InElement->Indices[k];
					InParams->AllColors[Index] += Color;
					++SummedCount[Index];
				}
			});			
		}
	}

	jShader* shader = jShader::GetShader("SimpleQuad");
	g_rhi->SetShader(shader);

	SET_UNIFORM_BUFFER_STATIC(Matrix, "MVP", InParams->DisplayCamera->GetViewProjectionMatrix(), shader);

	for (int32 i = 0; i < InParams->Patches.size(); ++i)
	{
		const bool IsSelected = (InSelectedPatch == i);
		if (SelectedShooterPatchAsDrawLine)
		{
			if (IsSelected)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				g_rhi->EnableCullFace(false);
			}
			else
			{
				if (WireFrame)
					glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				else
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				g_rhi->EnableCullFace(true);
			}
		}
		DrawElement(InParams, InParams->Patches[i].RootElement, SummedCount, shader, IsSelected);
	}
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	//for (int32 kk = 0; kk < numberOfPolys; ++kk)
	//{
	//	int32 Cnt = 0;
	//	Vector RadiositySum = Vector::ZeroVector;

	//	for (int32 i = 0; i < InParams->Patches.size(); ++i)
	//	{
	//		if (InParams->Patches[i].QuadID != kk)
	//			continue;

	//		InParams->Patches[i].RootElement->IterateUsingElement([&](Radiosity::Element* InElement)
	//			{
	//				++Cnt;
	//				RadiositySum += InElement->Delta;
	//			});
	//	}
	//	float Average = (RadiositySum / Cnt).Length();

	//	for (int32 i = 0; i < InParams->Patches.size(); ++i)
	//	{
	//		if (InParams->Patches[i].QuadID != kk)
	//			continue;
	//		//RadiositySqrtSum /= Cnt;
	//		//RadiositySum = ((RadiositySum / Cnt) * (RadiositySum / Cnt));
	//		//float Variance = (RadiositySqrtSum - RadiositySum).Length();
	//		//if (Variance > 0.03f)
	//		{
	//			InParams->Patches[i].RootElement->IterateUsingElement([&](Radiosity::Element* InElement)
	//				{
	//					float CurElementDelta = InElement->Delta.Length();
	//					float temp = fabs(Average - CurElementDelta);
	//					if (temp > 0.0015)
	//						InElement->Subdivide();
	//				});
	//		}
	//	}
	//}
}

void jGame::Update(float deltaTime)
{
	SCOPE_DEBUG_EVENT(g_rhi, "Game::Update");

	UpdateAppSetting();

	MainCamera->IsEnableCullMode = true;
	MainCamera->UpdateCamera();

	const int32 numOfLights = MainCamera->GetNumOfLight();
	for (int32 i = 0; i < numOfLights; ++i)
	{
		auto light = MainCamera->GetLight(i);
		JASSERT(light);
		light->Update(deltaTime);
	}

	static Radiosity::InputParams* RadiosityParams = InitInputParams();
	static bool IsInitialized = false;
	if (!IsInitialized)
	{
		for (int32 i = 0; i < RadiosityParams->Patches.size(); ++i)
		{
			RadiosityParams->Patches[i].RootElement->Iterate([](Radiosity::Element* InElement)
			{
				InElement->Subdivide();
			});
		}

		InitRadiosity(RadiosityParams);
		RadiosityParams->DisplayCamera = MainCamera;

		IsInitialized = true;
	}

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });

	int32 a = 1;
	static int32 PrevShooter = -1;
	int32 FoundShootPatch = StartShooterPatch;
	static int32 TotalProcessingCounts = 0;
	if (a < 0)
	{
		FoundShootPatch = PrevShooter;
	}
	else
	{
		while (a-- > 0)
		{
			++TotalProcessingCounts;
			if (FoundShootPatch == -1)
				FoundShootPatch = FindShootPatch(RadiosityParams);
			if (FoundShootPatch != -1)
			{
				ComputeFormfactors(FoundShootPatch, RadiosityParams);
				DistributeRadiosity(FoundShootPatch, RadiosityParams);

				//for (int i = 0; i < RadiosityParams->Patches.size(); ++i)
				//{
				//	if (RadiosityParams->Patches[i].QuadID != 4)
				//		continue;

				//	ComputeFormfactors(i, RadiosityParams);
				//	DistributeRadiosity(i, RadiosityParams);
				//}
			}
			else
			{
				break;
			}
			PrevShooter = FoundShootPatch;
		}
	}

	// 하는 중인 것 
	// fixedface = 3 으로 설정해두고, 4, 4와 4, 1을 변경해가면서 문제가 왜 발생하는지 파악해보자.
	static auto oldstate = g_KeyState['z'];
	if (g_KeyState['z'] != oldstate && oldstate == true)
	{
		a = 1;
	}
	oldstate = g_KeyState['z'];

	DisplayResults(RadiosityParams, FoundShootPatch);

	if (FoundShootPatch != -1)
	{
		//for (int i = 0; i < RadiosityParams->Patches.size(); ++i)
		//{
		//	if (RadiosityParams->Patches[i].QuadID != 1)
		//		continue;
		//	FoundShootPatch = i;
		//}
		const auto& ShootPatch = RadiosityParams->Patches[FoundShootPatch];

		//Vector Center = ShootPatch.Center + ShootPatch.Normal * 10.0f;
		//Vector Offset = ShootPatch.Normal * 10.0f;
		Radiosity::Patch::LookAtAndUp lookAtAndUp = ShootPatch.GenerateHemicubeLookAtAndUp();

		static int32 face = (FixedFace == -1) ? 0 : FixedFace;
		//for (int32 face = 0; face < 5; ++face)
		{
			Vector Offset = Vector::ZeroVector;// (lookAtAndUp.LookAt[face] - ShootPatch.Center).GetNormalize()* (sqrt(ShootPatch.Area) * 0.25f);
			RadiosityParams->HemicubeCamera->Target = lookAtAndUp.LookAt[face] + Offset;
			RadiosityParams->HemicubeCamera->Up = lookAtAndUp.Up[face] + Offset;
			RadiosityParams->HemicubeCamera->Pos = ShootPatch.Center + Offset;

			//g_rhi->SetClearColor(-1.0f, -1.0f, -1.0f, 1.0f);
			//g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });
			g_rhi->EnableCullFace(true);
			g_rhi->EnableDepthTest(true);

			RadiosityParams->HemicubeCamera->UpdateCamera();
			auto CameraDebug = jPrimitiveUtil::CreateFrustumDebug(RadiosityParams->HemicubeCamera);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			CameraDebug->Update(0.0f);
			CameraDebug->Draw(RadiosityParams->DisplayCamera, jShader::GetShader("Simple"), {});
			delete CameraDebug;
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
	}

	

	if (FoundShootPatch == -1)
		TitleText = " Converged";
	else
		TitleText = " Progressing : " + std::to_string(TotalProcessingCounts);
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
