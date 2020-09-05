#pragma once
#include "jAppSettings.h"

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

	struct CosmeticLayer
	{
		FORCEINLINE Vector K() const { return Vector(KR, KG, KB); }
		FORCEINLINE Vector S() const { return Vector(SR, SG, SB); }

		float KR, KG, KB;
		float SR, SG, SB;
		float X;
	};

	CosmeticLayer CosmeticLayerData[2] = 
	{ 
		{ 
			0.22f, 1.47f, 0.57f,	// K
			0.05f, 0.003f, 0.03f,	// S
			0.0f					// X
		},		// Quinacridone Rose (http://davis.wpi.edu/~matt/courses/watercolor/rendering.html)
		{ 
			0.86f, 0.86f, 0.06f,	// K
			0.005f, 0.005f, 0.09f,	// S
			1.0f					// X
		},	// French Ultramarine
	};

	virtual void Setup(jAppSettingBase* appSetting) override;
	virtual void Teardown(jAppSettingBase* appSetting) override;

	void SwitchShadowType(jAppSettingBase* appSetting);
	void SwitchShadowMapType(jAppSettingBase* appSetting);

private:
	jShadowAppSettingProperties() {}

	static jShadowAppSettingProperties* _instance;
};

