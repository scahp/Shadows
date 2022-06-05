#pragma once
#include "jShadowTypes.h"
#include "jPostProcess.h"

class jRHI;
extern jRHI* g_rhi;

class jDirectionalLight;
class jLight;
class jCamera;
struct jShader;
class jPointLight;
class jSpotLight;
class jRenderer;
struct jRenderTarget;
struct IShaderStorageBufferObject;
struct IAtomicCounterBuffer;
struct jTexture;
struct jRenderTarget;
class jObject;
class jPipelineSet;
class jCascadeDirectionalLight;
class jUIQuadPrimitive;

class jGame
{
public:
	jGame();
	~jGame();

	void ProcessInput();
	void Setup();

	enum class ESpawnedType
	{
		None = 0,
		Hair,
		TestPrimitive,
		CubePrimitive
	};

	void SpawnObjects(ESpawnedType spawnType);

	void RemoveSpawnedObjects();
	void SpawnHairObjects();
	void SpawnTestPrimitives();
	void SapwnCubePrimitives();

	void SpawnGraphTestFunc();	// Test

	ESpawnedType SpawnedType = ESpawnedType::None;

	void Update(float deltaTime);

	void UpdateAppSetting();

	void OnMouseButton();
	void OnMouseMove(int32 xOffset, int32 yOffset);
	void Teardown();

	jDirectionalLight* DirectionalLight = nullptr;
	jDirectionalLight* NormalDirectionalLight = nullptr;
	jCascadeDirectionalLight* CascadeDirectionalLight = nullptr;
	jPointLight* PointLight = nullptr;
	jSpotLight* SpotLight = nullptr;
	jLight* AmbientLight = nullptr;
	jCamera* MainCamera = nullptr;

	jObject* Sphere = nullptr;

	jRenderer* Renderer = nullptr;

	std::map<EShadowType, jRenderer*> ShadowRendererMap;
	EShadowType ShadowType = EShadowType::MAX;

	jRenderer* DeferredRenderer = nullptr;
	jRenderer* ForwardRenderer = nullptr;

	jPipelineSet* ShadowVolumePipelineSet = nullptr;
	std::map<EShadowMapType, jPipelineSet*> ShadowPipelineSetMap;
	std::map<EShadowMapType, jPipelineSet*> ShadowPoissonSamplePipelineSetMap;

	EShadowMapType CurrentShadowMapType = EShadowMapType::SSM;
	bool UsePoissonSample = false;
	EShadowType CurrentShadowType = EShadowType::MAX;

	jObject* DirectionalLightInfo = nullptr;
	jObject* PointLightInfo = nullptr;
	jObject* SpotLightInfo = nullptr;
	jUIQuadPrimitive* DirectionalLightShadowMapUIDebug = nullptr;

	std::vector<jObject*> SpawnedObjects;
};

