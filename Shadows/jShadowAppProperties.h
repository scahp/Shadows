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

	Vector MockCameraPos = Vector(0.0f, 100.0f, -400.0f);
	Vector MockCameraTarget = Vector(0.0f, 100.0f, 0.0f);
	float MockCameraFov = DegreeToRadian(45.0f);
	float MockCameraNear = 200.0f;
	float MockCameraFar = 800.0f;

	Vector CubePos = Vector(50.0f, 100.0f, 0.0f);
	Vector CubeScale = Vector(50.0f, 50.0f, 50.0f);

	Vector NDCCameraPos = Vector(0.0f, 0.0f, -2.5f);
	Vector NDCCameraTarget = Vector(0.0f, 0.0f, 0.0f);

	bool PossessMockCamera = false;
	bool PossessMockCameraOnlyShadow = false;

	//Vector OffsetPP;// = Vector(-500.0f, 150.0f, 50.0f);
	Vector OffsetPP = Vector::ZeroVector;
	Vector ScalePP = Vector(60.0f);

	bool CameraFit = true;
	float ConstBias = 1.0f;
	float SlopBias = 0.0f;

	float MainCameraNear = 10.0f;
	float MainCameraFar = 2000.0f;
	float VirtualSliderBack = 50.0f;

	bool ShadowOn = true;
	EShadowType ShadowType = EShadowType::ShadowMap;
	bool ShowSilhouette_DirectionalLight = false;
	bool ShowSilhouette_PointLight = false;
	bool ShowSilhouette_SpotLight = false;
	bool IsGPUShadowVolume = true;
	EShadowMapType ShadowMapType = EShadowMapType::SSM;
	bool UsePoissonSample = true;
	bool ShowDirectionalLightMap = true;
	bool UseTonemap = true;
	float AutoExposureKeyValue = 0.5f;
	bool DirectionalLightOn = true;
	bool PointLightOn = false;
	bool SpotLightOn = false;
	bool ShowDirectionalLightInfo = true;
	bool ShowPointLightInfo = false;
	bool ShowSpotLightInfo = false;
	bool ShowBoundBox = false;
	bool ShowBoundSphere = false;
	//Vector DirecionalLightDirection = Vector(0.0f, -100.0f, -100.0f).GetNormalize();
	Vector DirecionalLightDirection = Vector(0.0f, -1.0f, 0.0f);
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

