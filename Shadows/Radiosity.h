#pragma once

#define WITH_REFLECTANCE 1
extern bool IsOnlyUseFormFactor;
extern int32 FixedFace;
extern int32 StartShooterPatch;
extern bool IsFullSubdivide;

class jCamera;

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
		Vector UnShootRadiosity = Vector::ZeroVector;
		float Area = 0.0f;
		struct Element* RootElement = nullptr;

		~Patch();

		struct LookAtAndUp
		{
			Vector LookAt[5];
			Vector Up[5];
		};

		LookAtAndUp GenerateHemicubeLookAtAndUp() const
		{
			return GenerateHemicubeLookAtAndUp(Center);
		}

		LookAtAndUp GenerateHemicubeLookAtAndUp(Vector InCenter) const;
	};

	struct ElementFormFactor
	{
		float SumOfFormFactors = 0.0f;		// Save all faces formfactors
		float TempFormFactor = 0.0f;		// Save a face formfactors for temporary

		void AddTempFormfactor(float InFormFactor) { TempFormFactor += InFormFactor; }
		void ClearTempFormfactor() { TempFormFactor = 0.0f; }
		void ClearSumOfFormfactor() { SumOfFormFactors = 0.0f; }
		void AccumulateFormfactor(float InRateOfArea)
		{
			SumOfFormFactors += TempFormFactor * InRateOfArea;
			TempFormFactor = 0.0;
		}
	};

	struct Element
	{
		int32 ID = -1;
		int32 Indices[4];
		Vector Normal = Vector::ZeroVector;
		Vector Radiosity = Vector::ZeroVector;
		float Area = 0.0f;
		Patch* ParentPatch = nullptr;
		Element* Parentlement = nullptr;
		bool UsingChild[4] = { false, false, false, false };
		Element* ChildElement[4] = { nullptr, nullptr, nullptr, nullptr };
		ElementFormFactor Formfactor;

		~Element();

		static Element* GetNextEmptyElement(Element* InElement);	// Patch Next Empty Element - This is only use for subdividing of elements.
		static void SetPositionUniforms(const Vector(&InPos)[4], const jShader* InShader);
		static void SetColorUniforms(const Vector4(&InColor)[4], const jShader* InShader);

		bool IsUsingChild() const;
		bool Subdivide();						// Subdivide quadtree
		void GenerateEmptyChildElement();		// Create sub quadtree

		// Iterate only leaf element (include this)
		template <typename T>
		void IterateOnlyLeaf(T func)
		{
			if (IsUsingChild())
			{
				for (int32 i = 0; i < 4; ++i)
				{
					if (UsingChild[i] && ChildElement[i])
						ChildElement[i]->IterateOnlyLeaf(func);
				}
				return;
			}
			func(this);
		}

		// Iterate all element (include this)
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
	};

	struct InputParams
	{
		~InputParams();

		double Threshold = 0.0001;
		std::vector<Patch> Patches;
		std::vector<Element*> AllElementsIndexedByID;
		std::vector<Vector> AllVertices;
		std::vector<Vector> AllColors;
		jCamera* DisplayCamera = nullptr;
		uint32 HemicubeResolution = 512;
		float WorldSize = 500.0f;
		float IntensityScale = 20.0f;
		float TotalEnergy = 0.0f;
		jCamera* HemicubeCamera = nullptr;

		std::vector<float> TopFactors;
		std::vector<float> SideFactors;
		std::vector<Vector2> CurrentBuffer;

		int32 TotalElementCount = 0;

		static Vector UVToXYZ(const Vector(&InQuad)[4], float InU, float InV);
		static InputParams* InitInputParams();
		static void InitRadiosity(InputParams* InParams);

		Element* GetElementByID(int32 InElementID) const
		{
			JASSERT(AllElementsIndexedByID.size() > InElementID);
			return AllElementsIndexedByID[InElementID];
		}

		template <typename T>
		void ElementIterateOnlyLeaf(const T&& InFunc)
		{
			for (int32 i = 0; i < Patches.size(); ++i)
				Patches[i].RootElement->IterateOnlyLeaf(InFunc);
		}

		template <typename T>
		void ElementIterate(const T&& InFunc)
		{
			for (int32 i = 0; i < Patches.size(); ++i)
				Patches[i].RootElement->Iterate(InFunc);
		}
	};

	static Vector red = { 0.80f, 0.10f, 0.075f };
	static Vector yellow = { 0.9f, 0.8f, 0.1f };
	static Vector blue = { 0.075f, 0.10f, 0.35f };
	static Vector green = { 0.075f, 0.8f, 0.1f };
	static Vector white = { 1.0f, 1.0f, 1.0f };
	static Vector lightGrey = { 0.9f, 0.9f, 0.9f };
	static Vector black = { 0.0f, 0.0f, 0.0f };

#define numberOfPolys 	19

	// Patch and Element Count should be 2^n. because It use quadtree for subdivide.
	// ** ElementCnt is max subdivision for quad, so It will not subdivide if don't need it. **
	static int32 AdaptiveSubdivisionCnt = 1;		// 1 is not using adaptive subdivision
	////////////////////////////////////////////////////////////////////////////////////////////////
	//  Quad Indices,  PatchCnt, ElementCnt,  Area,         Normal,              Color,    Emission
	////////////////////////////////////////////////////////////////////////////////////////////////
	static Quad roomPolys[numberOfPolys] = {
		{{4, 5, 6, 7},		16,		AdaptiveSubdivisionCnt,		216 * 215,	{0.0f, -1.0f, 0.0f},	lightGrey,	black}, /* ceiling */
		{{0, 3, 2, 1},		16,		AdaptiveSubdivisionCnt,		216 * 215,	{0.0f, 1.0f, 0.0f},		lightGrey,	black}, /* floor */
		{{0, 4, 7, 3},		16,		AdaptiveSubdivisionCnt,		221 * 215,	{1.0f, 0.0f, 0.0f},		red,		black}, /* wall */
		{{0, 1, 5, 4},		16,		AdaptiveSubdivisionCnt,		221 * 216,	{0.0f, 0.0f, 1.0f},		lightGrey,	black}, /* wall */
		{{2, 6, 5, 1},		16,		AdaptiveSubdivisionCnt,		221 * 215,	{-1.0f, 0.0f, 0.0f},	green,		black}, /* wall */
		{{2, 3, 7, 6},		16,		AdaptiveSubdivisionCnt,		221 * 216,	{0.0f, 0.0f,-1.0f},		lightGrey,	black}, /* ADDED wall */
		{{8, 9, 10, 11},	1,		1,							40 * 45,	{0.0f, -1.0f, 0.0f},	black,		white}, /* light */
		{{16, 19, 18, 17},	2,		1,							65 * 65,	{0.0f, 1.0f, 0.0f},		yellow,		black}, /* box 1 */
		{{12, 13, 14, 15},	2,		1,							65 * 65,	{0.0f, -1.0f, 0.0f},	yellow,		black},
		{{12, 15, 19, 16},	2,		1,							65 * 65,	{-0.866f, 0.0f, -0.5f},	yellow,		black},
		{{12, 16, 17, 13},	2,		1,							65 * 65,	{0.5f, 0.0f, -0.866f},	yellow,		black},
		{{14, 13, 17, 18},	2,		1,							65 * 65,	{0.866f, 0.0f, 0.5f},	yellow,		black},
		{{14, 18, 19, 15},	2,		1,							65 * 65,	{-0.5f, 0.0f, 0.866f},	yellow,		black},
		{{24, 27, 26, 25},	4,		1,							65 * 65,	{0.0f, 1.0f, 0.0f},		lightGrey,	black}, /* box 2 */
		{{20, 21, 22, 23},	4,		1,							65 * 65,	{0.0f, -1.0f, 0.0f},	lightGrey,	black},
		{{20, 23, 27, 24},	4,		1,							65 * 130,	{-0.866f, 0.0f, -0.5f},	lightGrey,	black},
		{{20, 24, 25, 21},	4,		1,							65 * 130,	{0.5f, 0.0f, -0.866f},	lightGrey,	black},
		{{22, 21, 25, 26},	4,		1,							65 * 130,	{0.866f, 0.0f, 0.5f},	lightGrey,	black},
		{{22, 26, 27, 23},	4,		1,							65 * 130,	{-0.5f, 0.0f, 0.866f},	lightGrey,	black},
	};

	static Vector roomPoints[] = {
	{ 0.0f, 0.0f, 0.0f },
	{ 216.0f, 0.0f, 0.0f },
	{ 216.0f, 0.0f, 215.0f },
	{ 0.0f, 0.0f, 215.0f },
	{ 0.0f, 221.0f, 0.0f },
	{ 216.0f, 221.0f, 0.0f },
	{ 216.0f, 221.0f, 215.0f },
	{ 0.0f, 221.0f, 215.0f },

	{ 85.5f, 220.0f, 90.0f },
	{ 130.5f, 220.0f, 90.0f },
	{ 130.5f, 220.0f, 130.0f },
	{ 85.5f, 220.0f, 130.0f },

	{ 53.104f, 0.0f, 64.104f },
	{ 109.36f, 0.0f, 96.604f },
	{ 76.896f, 0.0f, 152.896f },
	{ 20.604f, 0.0f, 120.396f },
	{ 53.104f, 65.0f, 64.104f },
	{ 109.36f, 65.0f, 96.604f },
	{ 76.896f, 65.0f, 152.896f },
	{ 20.604f, 65.0f, 120.396f },

	{ 134.104f, 0.0f, 67.104f },
	{ 190.396f, 0.0f, 99.604f },
	{ 157.896f, 0.0f, 155.896f },
	{ 101.604f, 0.0f, 123.396f },
	{ 134.104f, 130.0f, 67.104f },
	{ 190.396f, 130.0f, 99.604f },
	{ 157.896f, 130.0f, 155.896f },
	{ 101.604f, 130.0f, 123.39f },
	};

	//////////////////////////////////////////////////////////////////////////
	// Radiosity Functions
	int32 FindShootPatch(InputParams* InParams);
	bool SumFactors(InputParams* InParams, bool InIsTop, Vector InUnShoot);
	void ComputeFormfactors(int32 InShootPatchIndex, InputParams* InParams);
	void DistributeRadiosity(int32 InShootPatchIndex, InputParams* InParams);
	Vector GetAmbient(InputParams* InParams);
	void DisplayResults(InputParams* InParams, int32 InSelectedPatch = -1);
	void DrawDebugFormfactorCamera(InputParams* InParams, int32 InSelectedPatch);
}