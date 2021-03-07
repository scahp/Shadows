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

enum class EDeferredRendererPass
{
	DepthPrepass = 0,
	ShowdowMap,
	GBuffer,
	SSAO,
	LightingPass,
	Tonemap,
	SSR,
	AA,
	MAX
};

static const char* EDeferredRendererPassString[] = {
	"DepthPrepass",
	"ShowdowMap",
	"GBuffer",
	"SSAO",
	"LightingPass",
	"Tonemap",
	"SSR",
	"AA"
};

enum class EDeferredRenderPassDebugRT
{
	None = 0,
	DepthPrepass,
	ShowdowMap,
	GBuffer_Color,
	GBuffer_Normal,
	GBuffer_Pos,
	SSAO,
	LightingPass,
	Tonemap,
	SSR,
	AA,
	MAX
};

static const char* EDeferredRenderPassDebugRTString[] = {
	"None",
	"DepthPrepass",
	"ShowdowMap",
	"GBuffer_Color",
	"GBuffer_Normal",
	"GBuffer_Pos",
	"SSAO",
	"LightingPass",
	"Tonemap",
	"SSR",
	"AA",
	"MAX"
};

enum class ESSRType
{
	SSR = 0,
	PPR,
	MAX
};

static const char* ESSRTypeString[] = {
	"SSR",
	"PPR",
	"MAX"
};
