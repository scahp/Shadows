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

enum class EMappingType
{
	Linear = 0,
	RelaxedCone,
	DualDepth,
	Interior,
	DualDepth_Interior,
	MAX
};

static const char* EMappingTypeString[] = {
	"Relief_Linear",
	"Relief_RelaxedCone",
	"Relief_DualDepth",
	"Interior",
	"DualDepth_Interior",
};

// Resource from Matrix city sample of ue5
// https://docs.unrealengine.com/5.0/en-US/city-sample-project-unreal-engine-demonstration/
enum class EDualDepthReliefTexture
{
	office1x1x1e_CaptureFG = 0,
	office1x1x1c_CaptureFG,
	office1x1x1b_CaptureFG,
	MAX,
};

static const char* EDualDepthReliefTextureString[] = {
	"office1x1x1e_CaptureFG",
	"office1x1x1c_CaptureFG",
	"office1x1x1b_CaptureFG",
};

enum class EInteriorTexture
{
	CaptureCube_Tex_Office1x1x1b2 = 0,
	CaptureCube_Tex_Office1x1x1c3,
	CaptureCube_Tex_Office1x1x1d2,
	MAX
};

static const char* EInteriorTextureString[] = {
    "CaptureCube_Tex_Office1x1x1b2",
    "CaptureCube_Tex_Office1x1x1c3",
    "CaptureCube_Tex_Office1x1x1d2",
};

