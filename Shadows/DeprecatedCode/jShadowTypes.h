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