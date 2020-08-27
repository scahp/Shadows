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

	//////////////////////////////////////////////////////////////////////////
	// Atmospheric Properties
	float Kr = 0.0025f;		// Rayleigh scattering constant
	float Km = 0.001f;		// Mie scattering constant
	float ESun = 20.0f;		// Sun brightness constant
	float g = -0.95f;			// The Mie phase asymmetry factor
	float Exposure = 2.0f;
	float InnerRadius = 10.0f;
	float OuterRadius = 10.25f;

	float m_Kr4PI = Kr * 4.0f * PI;
	float m_Km4PI = Km * 4.0f * PI;
	float AverageScaleDepth = 0.25f;

	float Wavelength[3] =
	{
		0.650f,		// 650 nm for red
		0.570f,		// 570 nm for green
		0.475f		// 475 nm for blue
	};

	float Wavelength4[3] =
	{
		powf(Wavelength[0], 4.0f),
		powf(Wavelength[1], 4.0f),
		powf(Wavelength[2], 4.0f)
	};
	bool UseHDR = true;
	bool AutoMoveSun = false;
	Vector ScatterColor = Vector(1.0f, 1.0f, 1.0f);
	//////////////////////////////////////////////////////////////////////////

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
	Vector DirecionalLightDirection = Vector(0.34f, -0.06f, 0.94f).GetNormalize();
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

	virtual void Setup(jAppSettingBase* appSetting) override;
	virtual void Teardown(jAppSettingBase* appSetting) override;

	void SwitchShadowType(jAppSettingBase* appSetting);
	void SwitchShadowMapType(jAppSettingBase* appSetting);

private:
	jShadowAppSettingProperties() {}

	static jShadowAppSettingProperties* _instance;
};