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
	const Vector mainCameraPos(310.392578, 297.393494, -720.757629);
	//const Vector mainCameraTarget(171.96f, 166.02f, -180.05f);
	//const Vector mainCameraPos(165.0f, 125.0f, -136.0f);
	//const Vector mainCameraPos(300.0f, 100.0f, 300.0f);
	const Vector mainCameraTarget(310.389954, 297.393829, -719.757996);
	MainCamera = jCamera::CreateCamera(mainCameraPos, mainCameraTarget, mainCameraPos + Vector(0.0, 1.0, 0.0), DegreeToRadian(45.0f), 10.0f, 5000.0f, SCR_WIDTH, SCR_HEIGHT, true);
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

	//for (auto iter : jObject::GetStaticObject())
	//	iter->Update(deltaTime);

	//for (auto& iter : jObject::GetBoundBoxObject())
	//	iter->Update(deltaTime);

	//for (auto& iter : jObject::GetBoundSphereObject())
	//	iter->Update(deltaTime);

	//for (auto& iter : jObject::GetDebugObject())
	//	iter->Update(deltaTime);

	//jObject::FlushDirtyState();

	//Renderer->Render(MainCamera);

	constexpr int32 ElementRenderTargetSize = 256;

	struct MaterialProperties
	{
		Vector Diffuse;
		Vector Emission;
		Vector Reflectance;
	};

	//	didn't end up using this
	struct SubElement
	{
		SubElement(const Vector& r, float a) : radiosity(r), area(a) { }
		Vector radiosity;
		float area;
	};

	struct Element
	{
		Vector Vertex[4];
		MaterialProperties MatProp;
		Vector Normal;
		float area = 0.0f;
		jObject* Object = nullptr;

		std::vector<SubElement> Subdivision;
		std::vector<Vector> RadianceToApply;

		//	what will break if this becomes doubles?
		std::vector<float> coords;

		void Init()
		{
			Vector i = Vertex[0] - Vertex[1];
			Vector j = Vertex[3] - Vertex[0];
			float area1 = i.CrossProduct(Vertex[0] - Vertex[1]).Length();
			Normal = i.CrossProduct(j).GetNormalize();
			float area2 = (Vertex[3] - Vertex[1]).CrossProduct(j).Length();
			area = (area1 + area2) / 2.0f;
			Subdivide(5);
		}

		void Subdivide(unsigned int Divisions)
		{
			Vector A = Vertex[0];
			Vector B = Vertex[1];
			Vector C = Vertex[2];
			Vector D = Vertex[3];

			Vector i = B - A;
			Vector j = D - A;

			//	first, make the VBO
			coords.clear();
			RadianceToApply.clear();
			for (unsigned int yc = 0; yc <= Divisions; ++yc)
			{
				float py = static_cast<float>(yc) / static_cast<float>(Divisions);
				for (unsigned int xc = 0; xc <= Divisions; ++xc)
				{
					float px = static_cast<float>(xc) / static_cast<float>(Divisions);
					Vector Coord = A + (px * i) + (py * j);

					if (yc == Divisions)
					{
						if (xc == Divisions)
							Coord = C;
						else if (xc == 0)
							Coord = D;
					}
					if (xc == Divisions)
					{
						if (yc == Divisions)
							Coord = C;
						else if (yc == 0)
							Coord = B;
					}

					coords.push_back(Coord.v[0]);
					coords.push_back(Coord.v[1]);
					coords.push_back(Coord.v[2]);

					RadianceToApply.push_back(Vector(0.0f, 0.0f, 0.0f));
				}
			}

			//	build the index buffer
			Subdivision.clear();
			std::vector<uint32> indices;
			for (uint32 yc = 0; yc < Divisions; ++yc)
			{
				for (uint32 xc = 0; xc < Divisions; ++xc)
				{
					uint32 offset = yc * (Divisions + 1);
					indices.push_back(xc + offset);
					indices.push_back(xc + 1 + offset);
					indices.push_back(xc + Divisions + 1 + offset);
					indices.push_back(xc + 1 + Divisions + 1 + offset);

					Vector curA(coords[3 * (xc + offset) + 0],
						coords[3 * (xc + offset) + 1],
						coords[3 * (xc + offset) + 2]);

					Vector curB(coords[3 * (xc + offset + 1) + 0],
						coords[3 * (xc + offset + 1) + 1],
						coords[3 * (xc + offset + 1) + 2]);

					Vector curC(coords[3 * (xc + 1 + Divisions + offset) + 0],
						coords[3 * (xc + 1 + Divisions + offset) + 1],
						coords[3 * (xc + 1 + Divisions + offset) + 2]);

					Vector curD(coords[3 * (xc + 1 + Divisions + 1 + offset) + 0],
						coords[3 * (xc + 1 + Divisions + 1 + offset) + 1],
						coords[3 * (xc + 1 + Divisions + 1 + offset) + 2]);

					Vector i = curA - curB;
					Vector j = curD - curA;
					float area1 = i.CrossProduct(curA - curC).Length();
					float area2 = (curD - curC).CrossProduct(j).Length();
					area = (area1 + area2) / 2.0f;

					Subdivision.push_back(SubElement(MatProp.Emission, area));
				}
			}

			//unsigned int len = coords.size() / 3;
			//for (unsigned int ui = 0; ui < len; ++ui)
			//{
			//	//	add radiance for last step
			//	float percent = ui / static_cast<float>(len);
			//	percent = 0.0f;
			//	coords.push_back(MatProp.Emission.v[0]);
			//	coords.push_back(MatProp.Emission.v[0]);
			//	coords.push_back(MatProp.Emission.v[0]);
			//}

			////	add space for total radiance
			//for (unsigned int ui = 0; ui < len; ++ui)
			//{
			//	float percent = ui / static_cast<float>(len);
			//	percent = 0.0f;
			//	coords.push_back(percent);
			//	coords.push_back(percent);
			//	coords.push_back(percent);
			//}

			int32 elementCount = static_cast<int32>(coords.size() / 3);

			auto vertexStreamData = std::shared_ptr<jVertexStreamData>(new jVertexStreamData());
			{
				auto streamParam = new jStreamParam<float>();
				streamParam->BufferType = EBufferType::STATIC;
				streamParam->ElementTypeSize = sizeof(float);
				streamParam->ElementType = EBufferElementType::FLOAT;
				streamParam->Stride = sizeof(float) * 3;
				streamParam->Name = "Pos";
				streamParam->Data.resize(elementCount * 3);
				memcpy(&streamParam->Data[0], &coords[0], elementCount * sizeof(Vector));
				vertexStreamData->Params.push_back(streamParam);

				vertexStreamData->PrimitiveType = EPrimitiveType::TRIANGLE_STRIP;
				vertexStreamData->ElementCount = elementCount;
			}

			{
				auto streamParam = new jStreamParam<float>();
				streamParam->BufferType = EBufferType::STATIC;
				streamParam->ElementType = EBufferElementType::FLOAT;
				streamParam->ElementTypeSize = sizeof(float);
				streamParam->Stride = sizeof(float) * 4;
				streamParam->Name = "Color";
				streamParam->Data.resize(elementCount * 4);
				Vector4* DataPtr = (Vector4*)&streamParam->Data[0];
				for (int32 i = 0; i < streamParam->Data.size() / 4; ++i)
				{
					*DataPtr = Vector4(MatProp.Diffuse, 1.0f);
					++DataPtr;
				}
				vertexStreamData->Params.push_back(streamParam);
			}

			auto indexStreamData = std::shared_ptr<jIndexStreamData>(new jIndexStreamData());
			indexStreamData->ElementCount = static_cast<int32>(indices.size());
			{
				auto streamParam = new jStreamParam<uint32>();
				streamParam->BufferType = EBufferType::STATIC;
				streamParam->ElementType = EBufferElementType::INT;
				streamParam->ElementTypeSize = sizeof(uint32);
				streamParam->Stride = sizeof(uint32) * 3;
				streamParam->Name = "Index";
				streamParam->Data.resize(indices.size());
				memcpy(&streamParam->Data[0], &indices[0], indices.size() * sizeof(uint32));
				indexStreamData->Param = streamParam;
			}

			if (Object)
				delete Object;
			Object = new jObject();
			Object->RenderObject = new jRenderObject();
			Object->RenderObject->CreateRenderObject(vertexStreamData, indexStreamData);
		}

		enum EDirection : int32
		{
			Up = 0,
			Down,
			Left,
			Right,
			Forward,
			Max
		};

		jCamera GetCamera(uint32 subIndex, EDirection dir, bool drawAxes = false)
		{
			jCamera camera;
			camera.Pos = Vector(coords[3 * subIndex], coords[3 * subIndex + 1], coords[3 * subIndex + 2]);

			Vector up(0.0f, 1.0f, 0.0f);

			if (fabs(Normal.y) >= fabs(Normal.x) && fabs(Normal.y) >= fabs(Normal.z))
				std::swap(up.y, up.z);

			Vector right = Normal.CrossProduct(up).GetNormalize();
			up = right.CrossProduct(Normal).GetNormalize();
			//right *= -1.0f;

			Matrix Rot;
			switch (dir)
			{
			case 1:	//	up
				Rot = Matrix::MakeRotate(right, -DegreeToRadian(90.0f));
				break;
			case 2:	//	down
				Rot = Matrix::MakeRotate(right, DegreeToRadian(90.0f));
				break;
			case 3: //	left
				Rot = Matrix::MakeRotate(up, DegreeToRadian(90.0f));
				break;
			case 4:	//	right
				Rot = Matrix::MakeRotate(up, -DegreeToRadian(90.0f));
				break;
			default:	//	straight ahead
				Rot.SetIdentity();
				break;
			}

			Vector RotatedNormal = Normal;
			RotatedNormal = Rot.Transform(RotatedNormal).GetNormalize();

			Vector RotatedUp = up;
			RotatedUp = Rot.Transform(RotatedUp).GetNormalize();

			camera.Target = camera.Pos + RotatedNormal;
			camera.Up = camera.Pos + RotatedUp;

			camera.IsPerspectiveProjection = true;
			camera.Width = ElementRenderTargetSize;
			camera.Height = ElementRenderTargetSize;
			camera.FOVRad = DegreeToRadian(90.0f);
			camera.Far = 1000.0f;
			camera.Near = 0.1f;

			//// 그림그리는 축을 그림
			//if (drawAxes)
			//{
			//	Vec3 pos(c.Position[0], c.Position[1], c.Position[2]);
			//	Vec3 normal2 = normal * 50.0f;
			//	Vec3 right2(c.Right[0] * 50.0f, c.Right[1] * 50.0f, c.Right[2] * 50.0f);
			//	Vec3 up2(c.Up[0] * 50.0f, c.Up[1] * 50.0f, c.Up[2] * 50.0f);

			//	glColor3f(0.0f, 0.0f, 0.0f);

			//	float emission1[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
			//	cgSetParameter3fv(Element::myCgVertexParam_Emissive, &emission1[0]);
			//	cgUpdateProgramParameters(Element::myCgVertexProgram);
			//	checkForCgError("emission1 update");

			//	glBegin(GL_LINES);
			//	glVertex3dv(&pos[0]);
			//	glVertex3d(pos[0] + normal2[0], pos[1] + normal2[1], pos[2] + normal2[2]);
			//	glEnd();

			//	float emission2[4] = { 1.0f, 0.0f, 0.0f, 0.0f };
			//	cgSetParameter3fv(Element::myCgVertexParam_Emissive, &emission2[0]);
			//	cgUpdateProgramParameters(Element::myCgVertexProgram);
			//	checkForCgError("emission2 update");

			//	glBegin(GL_LINES);
			//	glVertex3dv(&pos[0]);
			//	glVertex3d(pos[0] + right2[0], pos[1] + right2[1], pos[2] + right2[2]);
			//	glEnd();

			//	float emission3[4] = { 0.0f, 1.0f, 0.0f, 0.0f };
			//	cgSetParameter3fv(Element::myCgVertexParam_Emissive, &emission3[0]);
			//	cgUpdateProgramParameters(Element::myCgVertexProgram);
			//	checkForCgError("emission3 update");

			//	glBegin(GL_LINES);
			//	glVertex3dv(&pos[0]);
			//	glVertex3d(pos[0] + up2[0], pos[1] + up2[1], pos[2] + up2[2]);
			//	glEnd();
			//}

			return camera;
		}
	};

	static std::vector<Element> SceneElements;
	static bool IsLoadedScene = false;
	if (!IsLoadedScene)
	{
		IsLoadedScene = true;

		// 1. LoadScene Data
		auto GetVectorFromString = [](const char* InString)
		{
			Vector Result = Vector::ZeroVector;
			if (InString)
			{
				const char* delim = ",";
				char* next_token;
				char* token = strtok_s((char*)InString, delim, &next_token);
				int k = 0;
				while (token)
				{
					Result.v[k] = static_cast<float>(atof(token));
					token = strtok_s(NULL, delim, &next_token);
					++k;

					if (k >= 3)
						break;
				}
			}
			return Result;
		};

		tinyxml2::XMLDocument doc;
		doc.LoadFile("model/CornellBox.xml");
		tinyxml2::XMLElement* CornellBox = doc.FirstChildElement("CornellBox");
		JASSERT(CornellBox);
		for (tinyxml2::XMLElement* Elem = CornellBox->FirstChildElement("Element"); Elem; Elem = Elem->NextSiblingElement())
		{
			Element NewElement;
			for (int32 i = 0; i < 4; ++i)
			{
				char szTemp[128] = { 0, };
				sprintf_s(szTemp, sizeof(szTemp), "Vertex%d", i);
				tinyxml2::XMLElement* VertexElement = Elem->FirstChildElement(szTemp);
				NewElement.Vertex[i] = GetVectorFromString(VertexElement->GetText());
			}

			tinyxml2::XMLElement* MaterialProp = Elem->FirstChildElement("MaterialProperties");
			JASSERT(MaterialProp);
			if (MaterialProp)
			{
				tinyxml2::XMLElement* DiffuseColorElem = MaterialProp->FirstChildElement("DiffuseColor");
				JASSERT(DiffuseColorElem);
				NewElement.MatProp.Diffuse = GetVectorFromString(DiffuseColorElem->GetText());

				tinyxml2::XMLElement* EmissionColorElem = MaterialProp->FirstChildElement("Emission");
				JASSERT(EmissionColorElem);
				NewElement.MatProp.Emission = GetVectorFromString(EmissionColorElem->GetText());

				tinyxml2::XMLElement* ReflectanceColorElem = MaterialProp->FirstChildElement("Reflectance");
				JASSERT(ReflectanceColorElem);
				NewElement.MatProp.Reflectance = GetVectorFromString(ReflectanceColorElem->GetText());
			}

			NewElement.Init();
			SceneElements.push_back(NewElement);
		}

		//// 2. Create Vertex
		//for (auto Elem : SceneElements)
		//{
		//	Elem.Subdivide(2);
		//}
	}

	g_rhi->SetClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });
	g_rhi->EnableCullFace(false);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	jShader* shader = nullptr;
	shader = jShader::GetShader("Simple");

	// 선택한 Elem 만 그리기
	g_rhi->SetViewport(SCR_WIDTH * 0.5f, 0.0f, SCR_WIDTH * 0.5f, SCR_HEIGHT * 0.5f);

	static int32 SelectSubIndex = 15;
	{
		static bool PrevKeyStateUp = (g_KeyState['z'] || g_KeyState['Z']);
		bool NewKeyStateUp = (g_KeyState['z'] || g_KeyState['Z']);
		if (PrevKeyStateUp != NewKeyStateUp)
		{
			PrevKeyStateUp = NewKeyStateUp;
			if (!NewKeyStateUp)
				SelectSubIndex++;
		}
		static bool PrevKeyStateDown = (g_KeyState['x'] || g_KeyState['X']);
		bool NewKeyStateDown = (g_KeyState['x'] || g_KeyState['X']);
		if (PrevKeyStateDown != NewKeyStateDown)
		{
			PrevKeyStateDown = NewKeyStateDown;
			if (!NewKeyStateDown)
				SelectSubIndex--;
		}
	}

	static int32 ElemIndex = 3;
	{
		static bool PrevKeyStateUp = (g_KeyState['c'] || g_KeyState['C']);
		bool NewKeyStateUp = (g_KeyState['c'] || g_KeyState['C']);
		if (PrevKeyStateUp != NewKeyStateUp)
		{
			PrevKeyStateUp = NewKeyStateUp;
			if (!NewKeyStateUp)
				ElemIndex++;
		}
		static bool PrevKeyStateDown = (g_KeyState['v'] || g_KeyState['V']);
		bool NewKeyStateDown = (g_KeyState['v'] || g_KeyState['V']);
		if (PrevKeyStateDown != NewKeyStateDown)
		{
			PrevKeyStateDown = NewKeyStateDown;
			if (!NewKeyStateDown)
				ElemIndex--;
		}
	}
	ElemIndex = Clamp<int32>(ElemIndex, 0, SceneElements.size() - 1);

	auto Elem = SceneElements[ElemIndex];

	int32 maxIndex = Elem.coords.size() / 3;
	SelectSubIndex = Clamp(SelectSubIndex, 0, maxIndex - 1);

	Elem.Object->Update(deltaTime);
	Elem.Object->Draw(MainCamera, shader, {});

	// 선택한 Elem의 카메라에서 그리기
	static int32 DirectionIndex = Element::Up;
	{
		static bool PrevKeyStateUp = (g_KeyState['b'] || g_KeyState['B']);
		bool NewKeyStateUp = (g_KeyState['b'] || g_KeyState['B']);
		if (PrevKeyStateUp != NewKeyStateUp)
		{
			PrevKeyStateUp = NewKeyStateUp;
			if (!NewKeyStateUp)
				DirectionIndex++;
		}
		static bool PrevKeyStateDown = (g_KeyState['n'] || g_KeyState['N']);
		bool NewKeyStateDown = (g_KeyState['n'] || g_KeyState['N']);
		if (PrevKeyStateDown != NewKeyStateDown)
		{
			PrevKeyStateDown = NewKeyStateDown;
			if (!NewKeyStateDown)
				DirectionIndex--;
		}
	}
	DirectionIndex = Clamp(DirectionIndex, 0, (int32)Element::Max - 1);

	g_rhi->SetViewport(SCR_WIDTH * 0.5f, SCR_HEIGHT * 0.5f, SCR_WIDTH * 0.5f, SCR_HEIGHT * 0.5f);
	jCamera cam = Elem.GetCamera(SelectSubIndex, (Element::EDirection)DirectionIndex);
	cam.UpdateCamera();
	for (auto Elem : SceneElements)
	{
		Elem.Object->Update(deltaTime);
		Elem.Object->Draw(&cam, shader, {});
	}

	auto CameraDebug = jPrimitiveUtil::CreateFrustumDebug(&cam);
	
	// 전체 그리기
	g_rhi->SetViewport(0.0f, 0.0f, SCR_WIDTH * 0.5f, SCR_HEIGHT * 0.5f);

	MainCamera->UpdateCamera();
	for (auto Elem : SceneElements)
	{
		Elem.Object->Update(deltaTime);
		Elem.Object->Draw(MainCamera, shader, {});
	}
	CameraDebug->Update(deltaTime);
	CameraDebug->Draw(MainCamera, shader, {});

	delete CameraDebug;
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
