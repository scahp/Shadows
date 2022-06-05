#pragma once

enum class EShadowType
{
	ShadowVolume = 0,
	ShadowMap,
	MAX,
};

static const char* EShadowTypeString[] = {
	"ShadowVolume",
	"ShadowMap",
};

enum class EShadowVolumeSilhouette
{
	DirectionalLight = 0,
	PointLight,
	SpotLight,
	MAX,
};

static const char* EShadowVolumeSilhouetteString[] = {
	"DirectionalLight",
	"PointLight",
	"SpotLight",
};


enum class EShadowMapType
{
	SSM = 0,
	PCF,
	PCSS,
	VSM,
	ESM,
	EVSM,
	DeepShadowMap_DirectionalLight,
	CSM_SSM,
	MAX
};

static const char* EShadowMapTypeString[] = {
	"SSM",
	"PCF",
	"PCSS",
	"VSM",
	"ESM",
	"EVSM",
	"DeepShadowMap_DirectionalLight",
	"CSM_SSM",
};

enum class EReliefTracingType
{
	Linear = 0,
	RelaxedCone,
	MAX
};

static const char* EReliefTracingTypeString[] = {
	"Linear",
	"RelaxedCone",
};

// Resource from Matrix city sample of ue5
// https://docs.unrealengine.com/5.0/en-US/city-sample-project-unreal-engine-demonstration/
enum class EDualDepthReliefTexture
{
	supermarket4x4x2a_CaptureFG = 0,
	supermarket4x4x2a_CaptureBG,
	supermarket3x3x3b_CaptureFG,
	supermarket3x3x3b_CaptureBG,
	supermarket3x3x1a_CaptureFG,
	supermarket2x2x1a_CaptureFG,
	supermarket1x1x1c_CaptureFG,
	supermarket1x1x1b_CaptureFG,
	supermarket1x1x1a_CaptureFG,
	restaurant2x2x2a_CaptureFG,
	restaurant2x2x1a_CaptureFG,
	restaurant1x1x1c_CaptureFG,
	restaurant1x1x1c_CaptureBG,
	restaurant1x1x1b_CaptureFG,
	restaurant1x1x1a_CaptureFG,
	office3x3x1b_CaptureFG,
	office3x3x1a_CaptureFG,
	office2x2x1f_CaptureFG,
	office2x2x1c_CaptureFG,
	office2x2x1b_CaptureFG,
	office2x2x1a_CaptureFG,
	office1x1x1e_CaptureFG,
	office1x1x1d_CaptureFG,
	office1x1x1c_CaptureFG,
	office1x1x1b_CaptureFG,
	lobby4x4x2a_CaptureFG,
	lobby3x3x3c_CaptureFG,
	lobby3x3x3b_CaptureFG,
	lobby3x3x3b_CaptureBG,
	lobby3x3x1a_CaptureFG,
	lobby2x2x1c_CaptureFG,
	lobby2x2x1b_CaptureFG,
	lobby2x2x1a_CaptureFG,
	livingRoom2x2x1b_CaptureFG,
	livingRoom2x2x1a_CaptureFG,
	livingRoom1x1x1b_CaptureFG,
	livingRoom1x1x1b_CaptureBG,
	livingRoom1x1x1a_CaptureFG,
	black1x1x1_Capture,
	bedroom3x3x1a_CaptureFG,
	bedroom1x1x1c_CaptureFG,
	bedroom1x1x1b_CaptureFG,
	bedroom1x1x1a_CaptureFG,
	MAX,
};

static const char* EDualDepthReliefTextureString[] = {
	"supermarket4x4x2a_CaptureFG",
	"supermarket4x4x2a_CaptureBG",
	"supermarket3x3x3b_CaptureFG",
	"supermarket3x3x3b_CaptureBG",
	"supermarket3x3x1a_CaptureFG",
	"supermarket2x2x1a_CaptureFG",
	"supermarket1x1x1c_CaptureFG",
	"supermarket1x1x1b_CaptureFG",
	"supermarket1x1x1a_CaptureFG",
	"restaurant2x2x2a_CaptureFG",
	"restaurant2x2x1a_CaptureFG",
	"restaurant1x1x1c_CaptureFG",
	"restaurant1x1x1c_CaptureBG",
	"restaurant1x1x1b_CaptureFG",
	"restaurant1x1x1a_CaptureFG",
	"office3x3x1b_CaptureFG",
	"office3x3x1a_CaptureFG",
	"office2x2x1f_CaptureFG",
	"office2x2x1c_CaptureFG",
	"office2x2x1b_CaptureFG",
	"office2x2x1a_CaptureFG",
	"office1x1x1e_CaptureFG",
	"office1x1x1d_CaptureFG",
	"office1x1x1c_CaptureFG",
	"office1x1x1b_CaptureFG",
	"lobby4x4x2a_CaptureFG",
	"lobby3x3x3c_CaptureFG",
	"lobby3x3x3b_CaptureFG",
	"lobby3x3x3b_CaptureBG",
	"lobby3x3x1a_CaptureFG",
	"lobby2x2x1c_CaptureFG",
	"lobby2x2x1b_CaptureFG",
	"lobby2x2x1a_CaptureFG",
	"livingRoom2x2x1b_CaptureFG",
	"livingRoom2x2x1a_CaptureFG",
	"livingRoom1x1x1b_CaptureFG",
	"livingRoom1x1x1b_CaptureBG",
	"livingRoom1x1x1a_CaptureFG",
	"black1x1x1_Capture",
	"bedroom3x3x1a_CaptureFG",
	"bedroom1x1x1c_CaptureFG",
	"bedroom1x1x1b_CaptureFG",
	"bedroom1x1x1a_CaptureFG",
};
