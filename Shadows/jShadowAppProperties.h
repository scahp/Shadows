#pragma once
#include "jAppSettings.h"

struct CosmeticLayer
{
	FORCEINLINE Vector K() const { return Vector(KR, KG, KB); }
	FORCEINLINE Vector S() const { return Vector(SR, SG, SB); }

	void AddVariable(jAppSettingBase* appSetting, const std::string& name, const std::string& group);
	void RemoveVariable(jAppSettingBase* appSetting, const std::string& name) const;
	void SetVisibleVariable(jAppSettingBase* appSetting, const std::string& name, bool visible) const;

	float KR, KG, KB;
	float SR, SG, SB;
	float X;
};

enum class ECosmeticLayer : int32
{
	Quinacridone_Rose = 0,
	Indian_Red,
	Cadimium_Yellow,
	Hookers_Green,
	Cerulean_Blue,
	Burnt_Umber,
	Cadmium_Red,
	Brilliant_Orange,
	Hansa_Yellow,
	Phthalo_Green,
	French_Untramarine,
	Interference_Lilac,
	MAX
};

static const char* ECosmeticLayerString[] =
{
	"Quinacridone_Rose",
	"Indian_Red",
	"Cadimium_Yellow",
	"Hookers_Green",
	"Cerulean_Blue",
	"Burnt_Umber",
	"Cadmium_Red",
	"Brilliant_Orange",
	"Hansa_Yellow",
	"Phthalo_Green",
	"French_Untramarine",
	"Interference_Lilac",
};

// http://davis.wpi.edu/~matt/courses/watercolor/rendering.html
static const CosmeticLayer CosmeticLayerArray[static_cast<int32>(ECosmeticLayer::MAX)] =
{
	{ 0.22f, 1.47f, 0.57f, 0.05f, 0.003f, 0.03f, 1.0f },
	{ 0.46f, 1.07f, 1.5f, 1.28f, 0.38f, 0.21f, 1.0f },
	{ 0.1f, 0.36f, 3.45f, 0.97f, 0.65f, 0.007f, 1.0f },
	{ 1.62f, 0.61f, 1.64f, 0.01f, 0.012f, 0.003f, 1.0f },
	{ 1.52f, 0.32f, 0.25f, 0.06f, 0.26f, 0.4f, 1.0f },
	{ 0.74f, 1.54f, 2.1f, 0.09f, 0.09f, 0.004f, 1.0f },
	{ 0.14f, 1.08f, 1.68f, 0.77f, 0.015f, 0.018f, 1.0f },
	{ 0.13f, 0.81f, 3.45f, 0.005f, 0.009f, 0.007f, 1.0f },
	{ 0.06f, 0.21f, 1.78f, 0.5f, 0.88f, 0.009f, 1.0f },
	{ 1.55f, 0.47f, 0.63f, 0.01f, 0.05f, 0.035f, 1.0f },
	{ 0.86f, 0.86f, 0.06f, 0.005f, 0.005f, 0.09f, 1.0f },
	{ 0.08f, 0.11f, 0.07f, 1.25f, 0.42f, 1.43f, 1.0f },
};

//////////////////////////////////////////////////////////////////////////
class jShadowAppSettingProperties : public jAppSettingProperties
{
public:
	static jShadowAppSettingProperties& GetInstance()
	{
		if (!_instance)
			_instance = new jShadowAppSettingProperties();
		return *_instance;
	}

	bool ShadowOn = true;
	EShadowType ShadowType = EShadowType::ShadowMap;
	bool ShowSilhouette_DirectionalLight = false;
	bool ShowSilhouette_PointLight = false;
	bool ShowSilhouette_SpotLight = false;
	bool IsGPUShadowVolume = true;
	EShadowMapType ShadowMapType = EShadowMapType::SSM;
	bool UsePoissonSample = true;
	bool ShowDirectionalLightMap = false;
	bool UseTonemap = true;
	float AutoExposureKeyValue = 0.5f;
	bool DirectionalLightOn = true;
	bool PointLightOn = true;
	bool SpotLightOn = true;
	bool ShowDirectionalLightInfo = true;
	bool ShowPointLightInfo = false;
	bool ShowSpotLightInfo = false;
	bool ShowBoundBox = false;
	bool ShowBoundSphere = false;
	Vector DirecionalLightDirection = Vector(0.61f, -0.26f, -0.75).GetNormalize();
	Vector PointLightPosition = Vector(10.0f, 100.0f, 10.0f);
	Vector SpotLightPosition = Vector(0.0f, 60.0f, 5.0f);
	Vector SpotLightDirection = Vector(-1.0f, -1.0f, -0.4f).GetNormalize();
	float DeepShadowAlpha = 0.3f;
	bool ExponentDeepShadowOn = false;
	bool CSMDebugOn = false;
	float AdaptationRate = 1.0f;
	float BloomThreshold = 3.0f;
	float BloomBlurSigma = 0.8f;
	float BloomMagnitude = 0.75f;

	// Skin
	ESkinShading SkinShading = ESkinShading::Final;
	float RoughnessScale = 1.0;
	float SpecularScale = 1.0;
	float PreScatterWeight = 0.5;
	bool EnableTSM = true;
	bool EnergyConversion = true;
	bool VisualizeRangeSeam = false;
	bool ShowIrradianceMap = false;
	bool ShowStretchMap = false;
	bool ShowTSMMap = false;
	bool ShowPdhBRDFBaked = false;
	bool ShowShadowMap = false;
	float GaussianStepScale = 1.0f;
	bool FastBloomAndTonemap = true;

	// Cosmetic
	bool IsCustomCosmeticLayer = false;
	bool ShowCosmeticMap = true;

	CosmeticLayer CustomCosmeticLayer0 = CosmeticLayerArray[static_cast<int32>(ECosmeticLayer::Brilliant_Orange)];
	CosmeticLayer CustomCosmeticLayer1 = CosmeticLayerArray[static_cast<int32>(ECosmeticLayer::Hookers_Green)];
	CosmeticLayer CustomCosmeticLayer2 = CosmeticLayerArray[static_cast<int32>(ECosmeticLayer::Cadmium_Red)];

	ECosmeticLayer CosmeticLayer0 = ECosmeticLayer::Brilliant_Orange;
	ECosmeticLayer CosmeticLayer1 = ECosmeticLayer::Hookers_Green;
	ECosmeticLayer CosmeticLayer2 = ECosmeticLayer::Cadmium_Red;

	float CosmeticLayer0_Thickness = 1.0f;
	float CosmeticLayer1_Thickness = 1.0f;
	float CosmeticLayer2_Thickness = 1.0f;

	virtual void Setup(jAppSettingBase* appSetting) override;
	virtual void Teardown(jAppSettingBase* appSetting) override;

	void SwitchShadowType(jAppSettingBase* appSetting) const;
	void SwitchShadowMapType(jAppSettingBase* appSetting) const;
	void SwitchCosmeticLayer(jAppSettingBase* appSetting) const;

private:
	jShadowAppSettingProperties() {}

	static jShadowAppSettingProperties* _instance;
};

