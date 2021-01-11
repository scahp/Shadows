#include "pch.h"
#include "Radiosity.h"
#include "jCamera.h"
#include "jRenderTargetPool.h"
#include "jPrimitiveUtil.h"

bool IsOnlyUseFormFactor = false;
int32 FixedFace = -1;
int32 StartShooterPatch = -1;
bool SelectedShooterPatchAsDrawLine = false;
bool IsWireFrame = 0;
bool IsWireFrameWhite = 0;
bool IsFullSubdivide = 0;
int32 IsAddAmbient = 1;
bool IsDrawFormfactorsCamera = false;

namespace Radiosity
{
	//////////////////////////////////////////////////////////////////////////
	/// Patch
	Patch::~Patch()
	{
		JASSERT(sizeof(Element) > 0);
		delete RootElement;
	}

	Patch::LookAtAndUp Patch::GenerateHemicubeLookAtAndUp(Vector InCenter) const
	{
		Vector UpVector = Vector::UpVector;
		if (fabs(Normal.DotProduct(UpVector)) > 0.9)
			UpVector = Vector::FowardVector;

		// TangentU 를 계산
		Vector TangentU = Normal.CrossProduct(UpVector).GetNormalize();

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

	//////////////////////////////////////////////////////////////////////////
	/// Element
	Element::~Element()
	{
		for (int32 i = 0; i < 4; ++i) delete ChildElement[i];
	}

	Element* Element::GetNextEmptyElement(Element* InElement)
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

	void Element::SetPositionUniforms(const Vector(&InPos)[4], const jShader* InShader)
	{
		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[0]", InPos[0], InShader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[1]", InPos[1], InShader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[2]", InPos[2], InShader);
		SET_UNIFORM_BUFFER_STATIC(Vector, "Pos[3]", InPos[3], InShader);
	}

	void Element::SetColorUniforms(const Vector4(&InColor)[4], const jShader* InShader)
	{
		SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[0]", InColor[0], InShader);
		SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[1]", InColor[1], InShader);
		SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[2]", InColor[2], InShader);
		SET_UNIFORM_BUFFER_STATIC(Vector4, "Color[3]", InColor[3], InShader);
	}

	bool Element::IsUsingChild() const
	{
		for (int32 i = 0; i < 4; ++i)
		{
			if (UsingChild[i])
				return true;
		}
		return false;
	}

	bool Element::Subdivide()
	{
		bool IsSubDivided = false;
		for (int32 i = 0; i < 4; ++i)
		{
			if (ChildElement[i])
			{
				IsSubDivided = true;
				UsingChild[i] = true;

				ChildElement[i]->Radiosity = Radiosity;
			}
		}
		if (IsSubDivided)
			Radiosity = Vector::ZeroVector;
		return IsSubDivided;
	}

	void Element::GenerateEmptyChildElement()
	{
		for (int32 i = 0; i < 4; ++i)
		{
			if (ChildElement[i])
			{
				ChildElement[i]->GenerateEmptyChildElement();
			}
			else
			{
				ChildElement[i] = new Element();
				ChildElement[i]->Area = Area / 4.0f;
				ChildElement[i]->Parentlement = this;
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	/// InputParams
	InputParams::~InputParams()
	{
		delete DisplayCamera;
		delete HemicubeCamera;
	}

	Vector InputParams::UVToXYZ(const Vector(&InQuad)[4], float InU, float InV)
	{
		Vector Result;

		float InvU = 1.0f - InU;
		float InvV = 1.0f - InV;

		Result.x = InQuad[0].x * InvU * InvV + InQuad[1].x * InvU * InV + InQuad[2].x * InU * InV + InQuad[3].x * InU * InvV;
		Result.y = InQuad[0].y * InvU * InvV + InQuad[1].y * InvU * InV + InQuad[2].y * InU * InV + InQuad[3].y * InU * InvV;
		Result.z = InQuad[0].z * InvU * InvV + InQuad[1].z * InvU * InV + InQuad[2].z * InU * InV + InQuad[3].z * InU * InvV;

		return Result;
	}

	InputParams* InputParams::InitInputParams()
	{
		InputParams* Params = new InputParams();;

		// Generate Patch Array
		int32 NumOfPatches = 0;
		for (int32 i = 0; i < numberOfPolys; ++i)
			NumOfPatches += roomPolys[i].PatchLevel * roomPolys[i].PatchLevel;
		Params->Patches.resize(NumOfPatches);

		// Generate Vertices & Color Array 
		int32 NumOfVertices = 0;
		for (int32 i = 0; i < numberOfPolys; ++i)
		{
			const int32 VerticesWidth = roomPolys[i].ElementLevel * roomPolys[i].PatchLevel + 1;
			NumOfVertices += VerticesWidth * VerticesWidth;
		}
		Params->AllVertices.resize(NumOfVertices);
		Params->AllColors.resize(NumOfVertices, Vector::ZeroVector);

		static int32 ID_Gen = 0;

		int32 VertexIndex = 0;
		int32 PatchIndex = 0;
		int32 VertexOffset = 0;
		Vector Vertices[4];
		for (int32 i = 0; i < numberOfPolys; ++i)
		{
			const Quad& CurQuad = roomPolys[i];

			for (int32 k = 0; k < 4; ++k)
				Vertices[k] = roomPoints[CurQuad.Verts[k]];

			int32 NumOfCurQuadVertices = 0;

			// Element의 Vertex 계산
			{
				// 가로 세로 버택스 수
				const int32 nu = CurQuad.PatchLevel * CurQuad.ElementLevel + 1;
				const int32 nv = CurQuad.PatchLevel * CurQuad.ElementLevel + 1;

				// u, v step.
				const float du = 1.0f / (nu - 1);
				const float dv = 1.0f / (nv - 1);

				for (int32 k = 0; k < nu; ++k)
				{
					float u = k * du;
					for (int32 m = 0; m < nv; ++m)
					{
						float v = m * dv;
						Params->AllVertices[VertexIndex++] = UVToXYZ(Vertices, u, v);
						++NumOfCurQuadVertices;
					}
				}
			}

			int32 PrvPatchIndex = PatchIndex;

			std::vector<int32> CurrentGeneratedPatches;
			// Patch 계산
			{
				const int32 nu = CurQuad.PatchLevel;
				const int32 nv = CurQuad.PatchLevel;
				const float du = 1.0f / nu;
				const float dv = 1.0f / nv;

				for (int32 k = 0; k < nu; ++k)
				{
					float u = k * du + du / 2.0f;
					for (int32 m = 0; m < nv; ++m)
					{
						float v = m * dv + dv / 2.0f;
						Patch& CurPatch = Params->Patches[PatchIndex];
						CurPatch.QuadID = i;
						CurPatch.Center = UVToXYZ(Vertices, u, v);
						CurPatch.Normal = CurQuad.Normal;
						CurPatch.Reflectance = CurQuad.Reflectance;
						CurPatch.Emission = CurQuad.Emission;
						CurPatch.Area = static_cast<float>(CurQuad.Area) / (nu * nv);

						// Element의 나머지 설정은 Element 계산에서 진행함.
						CurPatch.RootElement = new Element();
						CurPatch.RootElement->Area = CurPatch.Area;			// Root Element의 크기 설정

						CurrentGeneratedPatches.push_back(PatchIndex);
						++PatchIndex;
					}
				}
			}

			// Element 계산
			{
				const int32 nu = CurQuad.PatchLevel * CurQuad.ElementLevel;
				const int32 nv = CurQuad.PatchLevel * CurQuad.ElementLevel;
				const float du = 1.0f / nu;
				const float dv = 1.0f / nv;

				int32 CurLevel = CurQuad.ElementLevel;

				while (CurLevel > 0)
				{
					int cnt = 0;
					for (int32 k = 0; k < nu; k += CurLevel)
					{
						float u = k * du + du / 2.0f;
						for (int32 m = 0; m < nv; m += CurLevel, ++cnt)
						{
							float v = m * dv + dv / 2.0f;

							// GeoHash algorithm https://en.wikipedia.org/wiki/Geohash
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
							//////////////////////////////////////////////////////////////////////////
							x *= CurLevel;
							y *= CurLevel;

							const float fi = y / (float)nv;
							const float fj = x / (float)nu;
							const int32 pi = (int32)(fi * CurQuad.PatchLevel);
							const int32 pj = (int32)(fj * CurQuad.PatchLevel);
							const int32 CurrentPatchIndex = PrvPatchIndex + pi * CurQuad.PatchLevel + pj;
							
							JASSERT(Params->Patches.size() > CurrentPatchIndex);
							Patch* CurrentPatch = &Params->Patches[CurrentPatchIndex];

							Element* CurElement = CurrentPatch->RootElement->GetNextEmptyElement(CurrentPatch->RootElement);
							JASSERT(CurElement);
							CurElement->ID = ID_Gen++;
							CurElement->Normal = CurQuad.Normal;
							CurElement->Indices[0] = VertexOffset + (y * (nv + 1) + x);
							CurElement->Indices[1] = VertexOffset + (y * (nv + 1) + (x + CurLevel));
							CurElement->Indices[2] = VertexOffset + ((y + CurLevel) * (nv + 1) + x);
							CurElement->Indices[3] = VertexOffset + ((y + CurLevel) * (nv + 1) + (x + CurLevel));
							CurElement->ParentPatch = CurrentPatch;
						}
					}
					CurLevel /= 2;

					if (CurLevel > 0)
					{
						for (int32 Index : CurrentGeneratedPatches)
							Params->Patches[Index].RootElement->GenerateEmptyChildElement();
					}
				}
			}

			VertexOffset += NumOfCurQuadVertices;
		}

		Params->TotalElementCount = ID_Gen;
		Params->AllElementsIndexedByID.resize(Params->TotalElementCount, nullptr);
		Params->ElementIterate([&](Element* InElement) {
			JASSERT(InElement);
			JASSERT(!Params->AllElementsIndexedByID[InElement->ID]);
			Params->AllElementsIndexedByID[InElement->ID] = InElement;			
		});

		return Params;
	}

	void InputParams::InitRadiosity(InputParams* InParams)
	{
		// Hemicube Resolution이 짝수이도록 함.
		const int32 Res = (int32)((InParams->HemicubeResolution / 2.0f + 0.5f)) * 2;
		InParams->HemicubeCamera = jCamera::CreateCamera(Vector::ZeroVector, Vector::ZeroVector, Vector::ZeroVector
			, DegreeToRadian(90.0f), 0.1f, InParams->WorldSize, static_cast<float>(Res), static_cast<float>(Res), true);

		/* take advantage of the symmetry in the delta form-factors */
		InParams->TopFactors.resize(Res * Res / 4, 0.0);
		InParams->SideFactors.resize(Res * Res / 4, 0.0);

		// Top and side Delta form-factors
		// 1/4 formfactors needed since we will use this like 4-way symmetry.
		{
			int32 Index = 0;
			const float halfRes = Res / 2.0f;
			for (int32 i = 0; i < halfRes; ++i)
			{
				const float y = (halfRes - (i - 0.5f)) / halfRes;	// ((halfRes - 0.5) / HalfRes), ((halfRes - 1.5) / HalfRes), ... (1.5 / HalfRes), (0.5 / HalfRes)
				const float ySq = y * y;
				for (int32 k = 0; k < halfRes; ++k)
				{
					const float x = (halfRes - (k + 0.5f)) / halfRes;
					const float xSq = x * x;
					float xy1Sq = xSq + ySq + 1.0f;
					xy1Sq *= xy1Sq;

					InParams->TopFactors[Index] = 1.0f / (xy1Sq * PI * halfRes * halfRes);
					InParams->SideFactors[Index] = y / (xy1Sq * PI * halfRes * halfRes);
					Index++;
				}
			}
		}

		// Initialize Raidosity
		for (int32 i = 0; i < InParams->Patches.size(); ++i)
			InParams->Patches[i].UnShootRadiosity = InParams->Patches[i].Emission;

		InParams->ElementIterateOnlyLeaf([](Element* InElement) {
			InElement->Radiosity = InElement->ParentPatch->Emission;
			});

		// 총 에너지 계산
		InParams->TotalEnergy = 0.0;
		for (int32 i = 0; i < InParams->Patches.size(); ++i)
			InParams->TotalEnergy += InParams->Patches[i].Emission.DotProduct(Vector(InParams->Patches[i].Area));
	}

	//////////////////////////////////////////////////////////////////////////
	// Radiosity Functions
	int32 FindShootPatch(InputParams* InParams)
	{
		int32 MaxEnergyPatchIndex = -1;
		float MaxEnergySum = 0.0f;
		for (int32 i = 0; i < InParams->Patches.size(); ++i)
		{
			float energySum = 0.0f;
			energySum += InParams->Patches[i].UnShootRadiosity.DotProduct(Vector(InParams->Patches[i].Area));

			if (energySum > MaxEnergySum)
			{
				MaxEnergyPatchIndex = i;
				MaxEnergySum = energySum;
			}
		}

		const float error = MaxEnergySum / InParams->TotalEnergy;
		if (error < InParams->Threshold)
			return -1;		// converged;

		return MaxEnergyPatchIndex;
	}

	bool SumFactors(InputParams* InParams, bool InIsTop, Vector InUnShoot)
	{
#define kBackgroundItem -1

		const int32 ResX = InParams->HemicubeCamera->Width;
		const int32 ResY = InParams->HemicubeCamera->Height;
		const int32 StartY = InIsTop ? 0 : (ResY / 2);
		const int32 EndY = ResY;

		const auto& CurrentFactors = (InIsTop ? InParams->TopFactors : InParams->SideFactors);

		std::set<int32> SubDivideElementSet;

		const int32 HalfResX = ResX / 2;
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
				const int32 CurrentID = static_cast<int32>(InParams->CurrentBuffer[i * ResX + k].x);
				if (current_backItem != CurrentID)
				{
					int32 indexX;
					if (k < HalfResX)
						indexX = k;
					else
						indexX = (HalfResX - 1 - (k % HalfResX));

					const int32 ElementID = indexX + indexY;
					JASSERT(CurrentFactors.size() > (ElementID));

					Element* CurElement = InParams->GetElementByID(CurrentID);
					JASSERT(CurElement);
					CurElement->Formfactor.AddTempFormfactor(CurrentFactors[ElementID]);

					if (!IsFullSubdivide)
					{
						// Get Average Depth like PCF.
						float AverageZ = 0.0f;
						float CntTemp = 0.0f;
						for (int32 ii = i - 1; ii <= i + 1; ++ii)
						{
							for (int32 kk = k - 1; kk <= k + 1; ++kk)
							{
								if (ii < 0 || ii > EndY - 1)
									continue;
								if (kk < 0 || kk > ResX - 1)
									continue;

								int32 CurY = Clamp(ii, StartY, EndY - 1);
								int32 CurX = Clamp(kk, 0, ResX - 1);
								if (InParams->CurrentBuffer[CurY * ResX + CurX].x != current_backItem)
								{
									float CurZ = InParams->CurrentBuffer[CurY * ResX + CurX].y;
									AverageZ += CurZ;
									++CntTemp;
								}
							}
						}
						AverageZ /= CntTemp;
						float z = InParams->CurrentBuffer[i * ResX + k].y;
						if (z > AverageZ + 0.5f)
							SubDivideElementSet.insert(CurrentID);
					}
				}
			}
		}

		bool Subdivided = false;

		if (!IsFullSubdivide)
		{
			// Subdivide if I need it.
			const float ThresholdEnergy = white.DotProduct(Vector::OneVector) * 0.5f;
			const float UnShootEnergy = InUnShoot.DotProduct(Vector::OneVector);
			if (UnShootEnergy > ThresholdEnergy)
			{
				InParams->ElementIterateOnlyLeaf([&](Element* InElement) {
					if (SubDivideElementSet.count(InElement->ID) <= 0)
						return;

					if (InElement->Subdivide())
						Subdivided = true;
					});
			}
		}

		return Subdivided;
	}

	void ComputeFormfactors(int32 InShootPatchIndex, InputParams* InParams)
	{
		// 슈팅 패치를 얻음
		Patch& ShootPatch = InParams->Patches[InShootPatchIndex];
		Patch::LookAtAndUp lookAtAndUp = ShootPatch.GenerateHemicubeLookAtAndUp();

		// hemicube 를 Shooting Patch의 중심보다 살짝 위에 위치시킴.
		InParams->HemicubeCamera->Pos = ShootPatch.Center + (ShootPatch.Normal * InParams->WorldSize * 0.00000001f);

		static std::shared_ptr<jRenderTarget> RenderTarget = jRenderTargetPool::GetRenderTarget(
			{ ETextureType::TEXTURE_2D, ETextureFormat::RG32F, ETextureFormat::RG, EFormatType::FLOAT
			, EDepthBufferType::DEPTH24, InParams->HemicubeCamera->Width, InParams->HemicubeCamera->Height, 1 });

		char szTemp[1024];
		int32 ProcessCount = 0;
		for (int32 face = 0; face < 5; ++face)
		{
			if (FixedFace != -1)
				face = FixedFace;

			sprintf_s(szTemp, sizeof(szTemp), "ComputeFormfactors Face : %d", face);
			SCOPE_DEBUG_EVENT(g_rhi, szTemp);

			InParams->HemicubeCamera->Target = lookAtAndUp.LookAt[face];
			InParams->HemicubeCamera->Up = lookAtAndUp.Up[face];

			InParams->HemicubeCamera->UpdateCamera();
			InParams->HemicubeCamera->IsEnableCullMode = true;

			int32 RemainSubDivideCnt = 10;
			while (1)
			{
				if (RenderTarget->Begin())
				{
					g_rhi->SetClearColor(-1.0f, -1.0f, -1.0f, 1.0f);
					g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });
					g_rhi->EnableCullFace(true);
					g_rhi->EnableDepthTest(true);
					g_rhi->EnableWireframe(false);

					jShader* shader = jShader::GetShader("RadiosityQuadID");
					g_rhi->SetShader(shader);
					SET_UNIFORM_BUFFER_STATIC(Vector, "CameraPos", InParams->HemicubeCamera->Pos, shader);
					SET_UNIFORM_BUFFER_STATIC(Matrix, "MVP", InParams->HemicubeCamera->GetViewProjectionMatrix(), shader);
					SET_UNIFORM_BUFFER_STATIC(Matrix, "M", Matrix(IdentityType), shader);

					InParams->ElementIterateOnlyLeaf([&](Element* InElement) {
						JASSERT(InElement);

						InElement->Formfactor.ClearTempFormfactor();		// Clear Current Face Formfactors
						if (ProcessCount == 0)
							InElement->Formfactor.ClearSumOfFormfactor();		// Clear SumOfFormfactor once at first

						static Vector Vertices[4];

						if (InElement->ParentPatch == &ShootPatch)
							return;

						for (int32 k = 0; k < 4; ++k)
						{
							const int32 Index = InElement->Indices[k];
							Vertices[k] = (InParams->AllVertices[Index]);
						}

						SET_UNIFORM_BUFFER_STATIC(int32, "ID", InElement->ID, shader);
						SET_UNIFORM_BUFFER_STATIC(Vector, "Normal", InElement->Normal, shader);

						Element::SetPositionUniforms(Vertices, shader);

						g_rhi->DrawArrays(EPrimitiveType::POINTS, 0, 1);
						});
					RenderTarget->End();
				}
				glFinish();

				InParams->CurrentBuffer.resize(RenderTarget->Info.Width * RenderTarget->Info.Height);
				jTexture_OpenGL* Tex = (jTexture_OpenGL*)RenderTarget->GetTexture();
				glBindTexture(GL_TEXTURE_2D, Tex->TextureID);
				glGetTexImage(GL_TEXTURE_2D, 0, GL_RG, GL_FLOAT, (GLvoid*)(&InParams->CurrentBuffer[0]));

				// Cache formfactor to elements
				const bool IsTopFace = (face == 0);
				const bool Subdivided = SumFactors(InParams, IsTopFace, ShootPatch.UnShootRadiosity);

				if (!Subdivided)
					break;
			}

			InParams->ElementIterateOnlyLeaf([&](Element* InElement) {
				const float RateOfArea = ShootPatch.Area / InElement->Area;
				InElement->Formfactor.AccumulateFormfactor(RateOfArea);
				});

			if (FixedFace != -1)
				break;

			++ProcessCount;
		}
	}

	void DistributeRadiosity(int32 InShootPatchIndex, InputParams* InParams)
	{
		Patch& ShootPatch = InParams->Patches[InShootPatchIndex];
		/* distribute unshotRad to every element */
		InParams->ElementIterateOnlyLeaf([&](Element* InElement) {
			const float CurrentFormFactor = InElement->Formfactor.SumOfFormFactors;

#if WITH_REFLECTANCE
			Vector DeltaRadiosity = ShootPatch.UnShootRadiosity * (CurrentFormFactor * InElement->ParentPatch->Reflectance);
#else
			Vector DeltaRadiosity = ShootPatch.UnShootRadiosity * (CurrentFormFactor);
#endif
			if (IsOnlyUseFormFactor)
				DeltaRadiosity = Vector(static_cast<float>(CurrentFormFactor));

			/* incremental element's radiosity and patch's unshot radiosity */
			const float w = InElement->Area / InElement->ParentPatch->Area;
			InElement->Radiosity += DeltaRadiosity;
			InElement->ParentPatch->UnShootRadiosity += DeltaRadiosity * w;
			});

		/* reset shooting patch's unshot radiosity */
		ShootPatch.UnShootRadiosity = black;
	}

	Vector GetAmbient(InputParams* InParams)
	{
		Vector Ambient = black;
		Vector Sum = black;
		static Vector OverallInterreflectance = black;
		static int first = 1;
		static float AreaSum = 0.0f;
		if (first) {
			Vector AverageReflectance = black;
			Vector rSum;
			rSum = black;
			for (int32 i = 0; i < InParams->Patches.size(); ++i)
			{
				AreaSum += InParams->Patches[i].Area;
				rSum += InParams->Patches[i].Reflectance * InParams->Patches[i].Area;
			}
			AverageReflectance = rSum / AreaSum;
			OverallInterreflectance = 1.0f / (1.0f - AverageReflectance); // infinite geometric series for interreflection
			first = 0;
		}

		for (int32 i = 0; i < InParams->Patches.size(); ++i)
			Sum += InParams->Patches[i].UnShootRadiosity * static_cast<float>((InParams->Patches[i].Area / AreaSum));

		Ambient = OverallInterreflectance * Sum;

		return Ambient;
	}

	void DisplayResults(InputParams* InParams, int32 InSelectedPatch /*= -1*/)
	{
		SCOPE_PROFILE(DisplayResults);
		Vector Ambient = GetAmbient(InParams);

		InParams->DisplayCamera->UpdateCamera();

		g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });
		g_rhi->EnableCullFace(true);
		g_rhi->EnableDepthTest(true);
		g_rhi->EnableWireframe(IsWireFrame);

		static std::vector<int32> SummedCount(InParams->AllVertices.size(), 0);

		memset(&InParams->AllColors[0], 0, InParams->AllColors.size() * sizeof(Vector));
		memset(&SummedCount[0], 0, SummedCount.size() * sizeof(int32));

		// 1. Prepare all colros of vertices.
		InParams->ElementIterateOnlyLeaf([&](Element* InElement) {
			Vector Color;
#if WITH_REFLECTANCE
			if (InParams->AddAmbient)
				Color = (InElement->Radiosity + (Ambient * InElement->ParentPatch->Reflectance)) * InParams->IntensityScale;
			else
#endif
				Color = InElement->Radiosity * InParams->IntensityScale;

			InElement->Iterate([&](Element* InAllChildElement) {
				for (int32 k = 0; k < 4; ++k)
				{
					int32 Index = InAllChildElement->Indices[k];
					InParams->AllColors[Index] += Color;
					++SummedCount[Index];
				}
				});
			});

		jShader* shader = jShader::GetShader("SimpleQuad");
		g_rhi->SetShader(shader);

		SET_UNIFORM_BUFFER_STATIC(Matrix, "MVP", InParams->DisplayCamera->GetViewProjectionMatrix(), shader);

		// 2. Draw all elements
		InParams->ElementIterateOnlyLeaf([&](Element* InElement) {
			const bool IsSelected = (InSelectedPatch == InElement->ParentPatch->QuadID);
			const bool IsUseWhiteColor = (SelectedShooterPatchAsDrawLine && IsSelected) || (IsWireFrameWhite && IsWireFrame);
			if (SelectedShooterPatchAsDrawLine)
			{
				if (IsSelected)
				{
					g_rhi->EnableWireframe(true);
					g_rhi->EnableCullFace(false);
				}
				else
				{
					g_rhi->EnableWireframe(IsWireFrame);
					g_rhi->EnableCullFace(true);
				}
			}

			static Vector Vertices[4];
			static Vector4 Colors[4];

			Vector Normal = InElement->Normal;
			for (int32 k = 0; k < 4; ++k)
			{
				int32 Index = InElement->Indices[k];
				Vertices[k] = (InParams->AllVertices[Index]);
				if (IsUseWhiteColor)
					Colors[k] = Vector4::OneVector;
				else
					Colors[k] = Vector4(InParams->AllColors[Index] / (float)SummedCount[Index], 1.0);
			}

			Element::SetPositionUniforms(Vertices, shader);
			Element::SetColorUniforms(Colors, shader);

			g_rhi->DrawArrays(EPrimitiveType::POINTS, 0, 1);
			});

		g_rhi->EnableWireframe(false);

		DrawDebugFormfactorCamera(InParams, InSelectedPatch);	// Draw Debug Camera
	}

	void DrawDebugFormfactorCamera(InputParams* InParams, int32 InSelectedPatch)
	{
		if (InSelectedPatch == -1)
			return;

		if (!IsDrawFormfactorsCamera)
			return;

		const auto& ShootPatch = InParams->Patches[InSelectedPatch];
		Patch::LookAtAndUp lookAtAndUp = ShootPatch.GenerateHemicubeLookAtAndUp();

		static int32 face = (FixedFace == -1) ? 0 : FixedFace;

		// hemicube 를 Shooting Patch의 중심보다 살짝 위에 위치시킴.
		InParams->HemicubeCamera->Pos = ShootPatch.Center + (ShootPatch.Normal * InParams->WorldSize * 0.00000001f);

		InParams->HemicubeCamera->Target = lookAtAndUp.LookAt[face];
		InParams->HemicubeCamera->Up = lookAtAndUp.Up[face];

		// Draw shooter frustum without clear
		//g_rhi->SetClearColor(-1.0f, -1.0f, -1.0f, 1.0f);
		//g_rhi->SetClear({ ERenderBufferType::COLOR | ERenderBufferType::DEPTH });

		g_rhi->EnableCullFace(true);
		g_rhi->EnableDepthTest(true);

		InParams->HemicubeCamera->UpdateCamera();
		auto CameraDebug = jPrimitiveUtil::CreateFrustumDebug(InParams->HemicubeCamera);
		g_rhi->EnableWireframe(true);
		CameraDebug->Update(0.0f);
		CameraDebug->Draw(InParams->DisplayCamera, jShader::GetShader("Simple"), {});
		delete CameraDebug;
		g_rhi->EnableWireframe(IsWireFrame);
	}

}