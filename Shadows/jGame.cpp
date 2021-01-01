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
		Vector Reflectance = Vector::ZeroVector;
		Vector Emission = Vector::ZeroVector;
		Vector Center = Vector::ZeroVector;
		Vector Normal = Vector::ZeroVector;
		Vector UnShotRadiosity = Vector::ZeroVector;
		double Area = 0.0f;
	};

	struct Element
	{
		std::vector<int32> Indices;
		Vector Normal = Vector::ZeroVector;
		Vector Radiosity = Vector::ZeroVector;
		double Area = 0.0;
		Patch* ParentPatch = nullptr;
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
		std::vector<Element> Elements;
		std::vector<Vector> AllVertices;
		std::vector<Vector> AllColors;
		jCamera* DisplayCamera = nullptr;
		uint32 HemicubeResolution = 512;
		float WorldSize = 500.0f;
		float IntensityScale = 20.0f;
		int32 AddAmbient = 1;

		double TotalEnergy = 0.0;
		std::vector<double> Formfactors;
		jCamera* HemicubeCamera = nullptr;

		std::vector<double> TopFactors;
		std::vector<double> SideFactors;
		std::vector<float> CurrentBuffer;
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
	Quad roomPolys[numberOfPolys] = {
		{{4, 5, 6, 7},		8*2, 1, 216 * 215, {0, -1, 0}, lightGrey, black}, /* ceiling */
		{{0, 3, 2, 1},		8*3, 1, 216 * 215, {0, 1, 0}, lightGrey, black}, /* floor */
		{{0, 4, 7, 3},		8*2, 1, 221 * 215, {1, 0, 0}, red, black}, /* wall */
		{{0, 1, 5, 4},		8*2, 1, 221 * 216, {0, 0, 1}, lightGrey, black}, /* wall */
		{{2, 6, 5, 1},		8*2, 1, 221 * 215, {-1, 0, 0}, green, black}, /* wall */
		{{2, 3, 7, 6},		8*2, 1, 221 * 216, {0, 0,-1}, lightGrey, black}, /* ADDED wall */
		{{8, 9, 10, 11},	1, 1, 40 * 45, {0, -1, 0}, black, white}, /* light */
		{{16, 19, 18, 17},	5, 1, 65 * 65, {0, 1, 0}, yellow, black}, /* box 1 */
		{{12, 13, 14, 15},	1, 1, 65 * 65, {0, -1, 0}, yellow, black},
		{{12, 15, 19, 16},	5, 1, 65 * 65, {-0.866, 0, -0.5}, yellow, black},
		{{12, 16, 17, 13},	5, 1, 65 * 65, {0.5, 0, -0.866}, yellow, black},
		{{14, 13, 17, 18},	5, 1, 65 * 65, {0.866, 0, 0.5}, yellow, black},
		{{14, 18, 19, 15},	5, 1, 65 * 65, {-0.5, 0, 0.866}, yellow, black},
		{{24, 27, 26, 25},	5, 1, 65 * 65, {0, 1, 0}, lightGrey, black}, /* box 2 */
		{{20, 21, 22, 23},	1, 1, 65 * 65, {0, -1, 0}, lightGrey, black},
		{{20, 23, 27, 24},	6, 1, 65 * 130, {-0.866, 0, -0.5}, lightGrey, black},
		{{20, 24, 25, 21},	6, 1, 65 * 130, {0.5, 0, -0.866}, lightGrey, black},
		{{22, 21, 25, 26},	6, 1, 65 * 130, {0.866, 0, 0.5}, lightGrey, black},
		{{22, 26, 27, 23},	6, 1, 65 * 130, {-0.5, 0, 0.866}, lightGrey, black},
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
		const int32 ElementWidth = Radiosity::roomPolys[i].ElementLevel* Radiosity::roomPolys[i].PatchLevel;
		NumOfElement += ElementWidth * ElementWidth;
	}
	Params->Elements.resize(NumOfElement);

	int32 NumOfVertices = 0;
	for (int32 i = numberOfPolys - 1; i >= 0; --i)
	{
		const int32 VerticesWidth = Radiosity::roomPolys[i].ElementLevel * Radiosity::roomPolys[i].PatchLevel + 1;
		NumOfVertices += VerticesWidth * VerticesWidth;
	}
	Params->AllVertices.resize(NumOfVertices);
	Params->AllColors.resize(NumOfVertices, Vector::ZeroVector);

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

		// Element 계산
		{
			int32 nu = CurQuad.PatchLevel * CurQuad.ElementLevel;
			int32 nv = CurQuad.PatchLevel * CurQuad.ElementLevel;
			double du = 1.0 / nu;
			double dv = 1.0 / nv;

			for (int32 k = 0; k < nu; ++k)
			{
				double u = k * du + du / 2.0;
				for (int32 m = 0; m < nv; ++m)
				{
					double v = m * dv + dv / 2.0;

					Radiosity::Element& CurElement = Params->Elements[ElementIndex++];
					CurElement.Normal = CurQuad.Normal;
					CurElement.Indices.resize(4);
					
					//CurElement.Indices[0] = VertexOffset + k * (nv + 1) + m;
					//CurElement.Indices[1] = VertexOffset + (k + 1) * (nv + 1) + m;
					//CurElement.Indices[2] = VertexOffset + (k + 1) * (nv + 1) + (m + 1);
					//CurElement.Indices[3] = VertexOffset + k * (nv + 1) + (m + 1);

					CurElement.Indices[0] = VertexOffset + k * (nv + 1) + m;
					CurElement.Indices[1] = VertexOffset + k * (nv + 1) + (m + 1);
					CurElement.Indices[2] = VertexOffset + (k + 1) * (nv + 1) + m;
					CurElement.Indices[3] = VertexOffset + (k + 1) * (nv + 1) + (m + 1);

					CurElement.Area = CurQuad.Area / (nu * nv);

					float fi = k / (float)nu;
					float fj = m / (float)nv;
					int32 pi = (int32)(fi * CurQuad.PatchLevel);
					int32 pj = (int32)(fj * CurQuad.PatchLevel);
					CurElement.ParentPatch = &Params->Patches[PatchIndex + pi * CurQuad.PatchLevel + pj];
				}
			}
		}

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
					Radiosity::Patch& CurPatch = Params->Patches[PatchIndex++];
					CurPatch.Center = UVToXYZ(Vertices, u, v);
					CurPatch.Normal = CurQuad.Normal;
					CurPatch.Reflectance = CurQuad.Reflectance;
					CurPatch.Emission = CurQuad.Emission;
					CurPatch.Area = CurQuad.Area / (nu * nv);
				}
			}
		}

		VertexOffset += NumOfCurQuadVertices;
	}

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
			double di = i;// -halfRes * 0.5;
			double y = (halfRes - (di + 0.5)) / halfRes;
			//double y = (di + 0.5) / halfRes;
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

	InParams->Formfactors.resize(InParams->Elements.size());

	// Initialize Raidosity
	for (int32 i = InParams->Patches.size() - 1; i >= 0; --i)
		InParams->Patches[i].UnShotRadiosity = InParams->Patches[i].Emission;
	for (int32 i = InParams->Elements.size() - 1; i >= 0; --i)
		InParams->Elements[i].Radiosity = InParams->Elements[i].ParentPatch->Emission;

	// 총 에너지 계산
	InParams->TotalEnergy = 0.0;
	for (int32 i = InParams->Patches.size() - 1; i >= 0; --i)
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
			indexY = i;
		else
			indexY = (HalfResX - 1 - (i % HalfResX));
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
				InParams->Formfactors[CurrentID] += CurrentFactors[indexX + indexY];
			}
		}
	}
}

//void DrawQuadWithID(const std::vector<Vector>& InVertices, const Vector& InNormal, int32 InID
//	, jCamera* InCamera, jShader* InShader)
//{
//	JASSERT(InShader);
//
//	g_rhi->SetShader(InShader);
//	SET_UNIFORM_BUFFER_STATIC(Vector, "Normal", InNormal, InShader);
//	SET_UNIFORM_BUFFER_STATIC(Vector, "CameraPos", InCamera->Pos, InShader);
//	SET_UNIFORM_BUFFER_STATIC(int32, "ID", InID, InShader);
//
//	static jObject* QuadObject = nullptr;
//	static bool InitializedDrawQuad = false;
//	if (!InitializedDrawQuad)
//	{
//		// attribute 추가
//		auto vertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());
//		{
//			auto streamParam = new jStreamParam<float>();
//			streamParam->BufferType = EBufferType::STATIC;
//			streamParam->ElementTypeSize = sizeof(float);
//			streamParam->ElementType = EBufferElementType::FLOAT;
//			streamParam->Stride = sizeof(float) * 3;
//			streamParam->Name = "Pos";
//			streamParam->Data.resize(InVertices.size() * 4);
//			memcpy(&streamParam->Data[0], &InVertices[0], sizeof(Vector) * 4);
//			vertexStreamData->Params.push_back(streamParam);
//		}
//
//		vertexStreamData->PrimitiveType = EPrimitiveType::TRIANGLE_STRIP;
//		vertexStreamData->ElementCount = InVertices.size();
//
//		auto renderObject = new jRenderObject();
//		renderObject->CreateRenderObject(vertexStreamData, nullptr);
//		renderObject->Pos = Vector::ZeroVector;
//		renderObject->Scale = Vector::OneVector;
//
//		QuadObject = new jObject();
//		QuadObject->RenderObject = renderObject;
//
//		InitializedDrawQuad = true;
//	}
//	jStreamParam<float>* Stream = static_cast<jStreamParam<float>*>(QuadObject->RenderObject->VertexStream->Params[0]);
//	if (Stream)
//	{
//		memcpy(&Stream->Data[0], &InVertices[0], sizeof(Vector) * 4);
//		QuadObject->RenderObject->UpdateVertexStream(0);
//	}
//
//	static std::list<const jLight*> EmtpyLights;
//	QuadObject->Draw(InCamera, InShader, EmtpyLights);
//}

//void DrawQuad(const std::vector<Vector>& InVertices, const Vector& InNormal, const Vector4& InColor
//	, jCamera* InCamera)
//{
//	jShader* shader = jShader::GetShader("RadiosityDebug");
//	JASSERT(shader);
//
//	g_rhi->SetShader(shader);
//
//	SET_UNIFORM_BUFFER_STATIC(Vector, "Normal", InNormal, shader);
//	SET_UNIFORM_BUFFER_STATIC(Vector4, "ColorTest", InColor, shader);
//
//	static jObject* QuadObject = nullptr;
//	static bool InitializedDrawQuad = false;
//	if (!InitializedDrawQuad)
//	{
//		// attribute 추가
//		auto vertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());
//		{
//			auto streamParam = new jStreamParam<float>();
//			streamParam->BufferType = EBufferType::STATIC;
//			streamParam->ElementTypeSize = sizeof(float);
//			streamParam->ElementType = EBufferElementType::FLOAT;
//			streamParam->Stride = sizeof(float) * 3;
//			streamParam->Name = "Pos";
//			streamParam->Data.resize(InVertices.size() * 4);
//			memcpy(&streamParam->Data[0], &InVertices[0], sizeof(Vector) * 4);
//			vertexStreamData->Params.push_back(streamParam);
//		}
//
//		vertexStreamData->PrimitiveType = EPrimitiveType::TRIANGLE_STRIP;
//		vertexStreamData->ElementCount = InVertices.size();
//
//		auto renderObject = new jRenderObject();
//		renderObject->CreateRenderObject(vertexStreamData, nullptr);
//		renderObject->Pos = Vector::ZeroVector;
//		renderObject->Scale = Vector::OneVector;
//
//		QuadObject = new jObject();
//		QuadObject->RenderObject = renderObject;
//
//		InitializedDrawQuad = true;
//	}
//	jStreamParam<float>* Stream = static_cast<jStreamParam<float>*>(QuadObject->RenderObject->VertexStream->Params[0]);
//	if (Stream)
//	{
//		memcpy(&Stream->Data[0], &InVertices[0], sizeof(Vector) * 4);
//		QuadObject->RenderObject->UpdateVertexStream(0);
//	}
//
//	static std::list<const jLight*> EmtpyLights;
//	QuadObject->Draw(InCamera, shader, EmtpyLights);
//}

//void DrawQuad(const std::vector<Vector>& InVertices, const std::vector<Vector4>& InColors, const Vector& InNormal, jCamera* InCamera)
//{
//	jShader* shader = jShader::GetShader("RadiosityLinearColorDebug");
//	JASSERT(shader);
//
//	g_rhi->SetShader(shader);
//
//	SET_UNIFORM_BUFFER_STATIC(Vector, "Normal", InNormal, shader);
//
//	static jObject* QuadObject = nullptr;
//	static bool InitializedDrawQuad = false;
//	if (!InitializedDrawQuad)
//	{
//		// attribute 추가
//		auto vertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());
//		{
//			auto streamParam = new jStreamParam<float>();
//			streamParam->BufferType = EBufferType::STATIC;
//			streamParam->ElementTypeSize = sizeof(float);
//			streamParam->ElementType = EBufferElementType::FLOAT;
//			streamParam->Stride = sizeof(float) * 3;
//			streamParam->Name = "Pos";
//			streamParam->Data.resize(InVertices.size() * 4);
//			memcpy(&streamParam->Data[0], &InVertices[0], sizeof(Vector) * 4);
//			vertexStreamData->Params.push_back(streamParam);
//		}
//
//		{
//			auto streamParam = new jStreamParam<float>();
//			streamParam->BufferType = EBufferType::STATIC;
//			streamParam->ElementTypeSize = sizeof(float);
//			streamParam->ElementType = EBufferElementType::FLOAT;
//			streamParam->Stride = sizeof(float) * 4;
//			streamParam->Name = "Color";
//			streamParam->Data.resize(InVertices.size() * 4);
//			memcpy(&streamParam->Data[0], &InColors[0], sizeof(Vector4) * 4);
//			vertexStreamData->Params.push_back(streamParam);
//		}
//
//		vertexStreamData->PrimitiveType = EPrimitiveType::TRIANGLE_STRIP;
//		vertexStreamData->ElementCount = InVertices.size();
//
//		auto renderObject = new jRenderObject();
//		renderObject->CreateRenderObject(vertexStreamData, nullptr);
//		renderObject->Pos = Vector::ZeroVector;
//		renderObject->Scale = Vector::OneVector;
//
//		QuadObject = new jObject();
//		QuadObject->RenderObject = renderObject;
//
//		InitializedDrawQuad = true;
//	}
//	jStreamParam<float>* Stream = static_cast<jStreamParam<float>*>(QuadObject->RenderObject->VertexStream->Params[0]);
//	if (Stream)
//	{
//		memcpy(&Stream->Data[0], &InVertices[0], sizeof(Vector) * 4);
//		QuadObject->RenderObject->UpdateVertexStream(0);
//	}
//	jStreamParam<float>* Stream2 = static_cast<jStreamParam<float>*>(QuadObject->RenderObject->VertexStream->Params[1]);
//	if (Stream2)
//	{
//		memcpy(&Stream2->Data[0], &InColors[0], sizeof(Vector4) * 4);
//		QuadObject->RenderObject->UpdateVertexStream(1);
//	}
//
//	static std::list<const jLight*> EmtpyLights;
//	QuadObject->Draw(InCamera, shader, EmtpyLights);
//}

void ComputeFormfactors(int32 InShootPatchIndex, Radiosity::InputParams* InParams)
{
	Radiosity::Patch& ShootPatch = InParams->Patches[InShootPatchIndex];

	// 슈팅 패치를 얻음
	Vector Center = ShootPatch.Center;
	Vector Normal = ShootPatch.Normal;
	jPlane Plane(Normal, -(Normal.DotProduct(Center)));

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
			if (fabs(temp) > 0.9 )
				RandVec = Vector::FowardVector;
			TangentU = Normal.CrossProduct(RandVec).GetNormalize();

		} while (TangentU.Length() == 0);
	}

	// TangentV 를 계산
	Vector TangentV = Normal.CrossProduct(TangentU).GetNormalize();

	// Hemicube face를 계산한다.
	Vector LookAt[5];
	Vector Up[5];

	LookAt[0] = Center + Normal;
	Up[0] = Center + TangentU;

	LookAt[1] = Center + TangentU;
	Up[1] = Center + Normal;

	LookAt[2] = Center + TangentV;
	Up[2] = Center + Normal;

	LookAt[3] = Center - TangentU;
	Up[3] = Center + Normal;

	LookAt[4] = Center - TangentV;
	Up[4] = Center + Normal;

	for (int32 i = 0; i < InParams->Formfactors.size(); ++i)
		InParams->Formfactors[i] = 0.0;

	// hemicube 를 Shooting Patch의 중심보다 살짝 위에 위치시킴.
	InParams->HemicubeCamera->Pos = Center + (Normal * InParams->WorldSize * 0.00000001);

	static std::vector<Vector> Vertices(4, Vector::ZeroVector);
	//jShader* shader = jShader::GetShader("RadiosityElementID");
	//g_rhi->SetShader(shader);

	static std::shared_ptr<jRenderTarget> RenderTarget = jRenderTargetPool::GetRenderTarget(
		{ ETextureType::TEXTURE_2D, ETextureFormat::R32F, ETextureFormat::R, EFormatType::FLOAT
		, EDepthBufferType::DEPTH24, InParams->HemicubeCamera->Width, InParams->HemicubeCamera->Height, 1 });

	// 전체 그리기
	//for (int32 face = 0; face < 5; ++face)
		static int32 face = 1;
	//if (0)
	//{
	//	InParams->HemicubeCamera->Target = LookAt[face];
	//	InParams->HemicubeCamera->Up = Up[face];

	//	InParams->HemicubeCamera->UpdateCamera();
	//	auto CameraDebug = jPrimitiveUtil::CreateFrustumDebug(InParams->HemicubeCamera);
	//	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//	shader = jShader::GetShader("Simple");
	//	CameraDebug->Update(0.0f);
	//	CameraDebug->Draw(InParams->DisplayCamera, jShader::GetShader("Simple"), {});
	//	delete CameraDebug;
	//	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//	//return;
	//}

	char szTemp[1024];
	for (int32 face = 0; face < 5; ++face)
	{
		sprintf_s(szTemp, sizeof(szTemp), "ComputeFormfactors Face : %d", face);
		SCOPE_DEBUG_EVENT(g_rhi, szTemp);
		InParams->HemicubeCamera->Target = LookAt[face];
		InParams->HemicubeCamera->Up = Up[face];

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

			for (int32 i = 0; i < InParams->Elements.size(); ++i)
			{
				if (InParams->Elements[i].ParentPatch == &ShootPatch)
					continue;

				for (int32 k = 0; k < InParams->Elements[i].Indices.size(); ++k)
				{
					int32 Index = InParams->Elements[i].Indices[k];
					Vertices[k] = (InParams->AllVertices[Index]);
				}
				//DrawQuadWithID(Vertices, InParams->Elements[i].Normal, i, InParams->HemicubeCamera, shader);
				//float Temp = (float)i / InParams->Elements.size();
				//DrawQuad(Vertices, InParams->Elements[i].Normal, Vector4(Vector(Temp), 1.0), InParams->HemicubeCamera);

				SET_UNIFORM_BUFFER_STATIC(int32, "ID", i, shader);
				SET_UNIFORM_BUFFER_STATIC(Vector, "Normal", InParams->Elements[i].Normal, shader);

				SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[0]", Vertices[0], shader);
				SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[1]", Vertices[1], shader);
				SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[2]", Vertices[2], shader);
				SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[3]", Vertices[3], shader);

				g_rhi->DrawArrays(EPrimitiveType::POINTS, 0, 1);
			}
			RenderTarget->End();
		}
		//return;

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
	}

	// reciprocal form-factor 계산
	for (int32 i = 0; i < InParams->Elements.size();++i)
	{
		InParams->Formfactors[i] *= ShootPatch.Area / InParams->Elements[i].Area;

		/* This is a potential source of hemi-cube aliasing */
		/* To do this right, we need to subdivide the shooting patch
		and reshoot. For now we just clip it to unity */
		if (InParams->Formfactors[i] > 1.0)
			InParams->Formfactors[i] = 1.0;
	}
}

void DistributeRadiosity(int32 InShootPatchIndex, Radiosity::InputParams* InParams)
{
	Radiosity::Patch& ShootPatch = InParams->Patches[InShootPatchIndex];

	/* distribute unshotRad to every element */
	for (int32 i = 0; i < InParams->Elements.size(); ++i)
	{
		if (InParams->Formfactors[i] == 0.0)
			continue;

		Vector DeltaRadiosity = ShootPatch.UnShotRadiosity * (InParams->Formfactors[i] * InParams->Elements[i].ParentPatch->Reflectance);
		//Vector DeltaRadiosity = ShootPatch.UnShotRadiosity * InParams->Elements[i].ParentPatch->Reflectance;

		/* incremental element's radiosity and patch's unshot radiosity */
		double w = InParams->Elements[i].Area / InParams->Elements[i].ParentPatch->Area;
		InParams->Elements[i].Radiosity += DeltaRadiosity;
		InParams->Elements[i].ParentPatch->UnShotRadiosity += DeltaRadiosity * w;
	}

	/* reset shooting patch's unshot radiosity */
	ShootPatch.UnShotRadiosity = Radiosity::black;
}

Vector GetAmbient(Radiosity::InputParams* InParams)
{
	Vector Ambient = Radiosity::black;
	Vector Sum = Radiosity::black;
	static Vector BaseSum = Radiosity::black;
	static int first = 1;
	if (first) {
		double areaSum = 0.0;
		Vector rSum;
		rSum = Radiosity::black;
		/* sum area and (area*reflectivity) */
		for (int32 i = 0; i < InParams->Patches.size(); ++i)
		{
			areaSum += InParams->Patches[i].Area;
			rSum += InParams->Patches[i].Reflectance * InParams->Patches[i].Area;
		}
		BaseSum = areaSum - rSum;
		first = 0;
	}

	/* sum (unshot radiosity * area) */
	for (int32 i = 0; i < InParams->Patches.size(); ++i)
	{
		Sum += InParams->Patches[i].UnShotRadiosity * InParams->Patches[i].Area;
	}

	/* compute ambient */
	Ambient = Sum / BaseSum;

	return Ambient;
}

void DisplayResults(Radiosity::InputParams* InParams, int32 InSelectedPatch = -1, bool InIsUpdated = false)
{
	SCOPE_PROFILE(DisplayResults);
	Vector Ambient = GetAmbient(InParams);

	InParams->DisplayCamera->UpdateCamera();

	//g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });
	g_rhi->EnableCullFace(true);
	g_rhi->EnableDepthTest(true);
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	static std::vector<Vector> Vertices(4, Vector::ZeroVector);
	static std::vector<Vector4> Colors(4, Vector4::ZeroVector);
	static std::vector<int32> SummedCount(InParams->AllVertices.size(), 0);

	//if (InIsUpdated)
	{
		//InParams->AllColors.resize(InParams->AllVertices.size(), Vector::ZeroVector);
		//SummedCount.resize(InParams->AllVertices.size(), 0);
		memset(&InParams->AllColors[0], 0, InParams->AllColors.size() * sizeof(Vector));
		memset(&SummedCount[0], 0, SummedCount.size() * sizeof(int32));

		for (int32 i = 0; i < InParams->Elements.size(); ++i)
		{
			Vector Color;
			if (InParams->AddAmbient)
				Color = (InParams->Elements[i].Radiosity + (Ambient * InParams->Elements[i].ParentPatch->Reflectance)) * InParams->IntensityScale;
			else
				Color = InParams->Elements[i].Radiosity * InParams->IntensityScale;

			for (int32 k = 0; k < InParams->Elements[i].Indices.size(); ++k)
			{
				int32 Index = InParams->Elements[i].Indices[k];
				InParams->AllColors[Index] += Color;
				++SummedCount[Index];
			}
		}
	}

	jShader* shader = jShader::GetShader("SimpleQuad");
	g_rhi->SetShader(shader);

	SET_UNIFORM_BUFFER_STATIC(Matrix, "MVP", InParams->DisplayCamera->GetViewProjectionMatrix(), shader);

	for (int32 i = 0; i < InParams->Elements.size(); ++i)
	{
		if ((InSelectedPatch != -1) && (&InParams->Patches[InSelectedPatch] != InParams->Elements[i].ParentPatch))
			continue;
			
		Vector Normal = InParams->Elements[i].Normal;
		for (int32 k = 0; k < InParams->Elements[i].Indices.size(); ++k)
		{
			int32 Index = InParams->Elements[i].Indices[k];
			Vertices[k] = (InParams->AllVertices[Index]);
			Colors[k] = Vector4(InParams->AllColors[Index] / (float)SummedCount[Index], 1.0);
		}

		float Temp = (float)i / InParams->Elements.size();
		//DrawQuad(Vertices, InParams->Elements[i].Normal, Vector4(Vector(Temp), 1.0), InParams->DisplayCamera);
		//DrawQuad(Vertices, InParams->Elements[i].Normal, MaxColors[i], InParams->DisplayCamera);
		//DrawQuad(Vertices, InParams->Elements[i].Normal, Vector4(InParams->Elements[i].ParentPatch->Reflectance, 1.0)
		//	, InParams->DisplayCamera, shader);
		
		//DrawQuad(Vertices, Colors, Normal, InParams->DisplayCamera);

		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[0]", Vertices[0], shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[1]", Vertices[1], shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[2]", Vertices[2], shader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[3]", Vertices[3], shader);

		SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[0]", Colors[0], shader);
		SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[1]", Colors[1], shader);
		SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[2]", Colors[2], shader);
		SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[3]", Colors[3], shader);

		g_rhi->DrawArrays(EPrimitiveType::POINTS, 0, 1);
	}
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
		InitRadiosity(RadiosityParams);
		RadiosityParams->DisplayCamera = MainCamera;

		IsInitialized = true;
	}

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });

	int32 a = 1;
	bool Updated = false;
	int32 FoundShootPatch = 23;
	while (a--)
	{
		FoundShootPatch = FindShootPatch(RadiosityParams);
		if (FoundShootPatch != -1)
		//if (1)
		{
			auto& tempPatch = RadiosityParams->Patches[FoundShootPatch];

			ComputeFormfactors(FoundShootPatch, RadiosityParams);
			//if (!Updated)
				DistributeRadiosity(FoundShootPatch, RadiosityParams);
			Updated = true;
			//break;
		}
		else
		{
			break;
		}
	}
	DisplayResults(RadiosityParams, -1, Updated);
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
