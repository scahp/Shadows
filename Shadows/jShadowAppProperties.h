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
	Vector DirecionalLightDirection = Vector(-0.56f, -0.83f, 0.01f).GetNormalize();
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

	Vector LeftWallColor = { 1.0f, 0.0f, 0.0f };
	float LeftWallAlpha = 0.5f;
	
	Vector BackWallColor = { 0.7f, 0.7f, 0.7f };
	float BackWallAlpha = 0.5f;
	
	Vector RightWallColor = { 0.0f, 0.0f, 1.0f };
	float RightWallAlpha = 0.5f;
	
	Vector FloorWallColor = { 0.0f, 1.0f, 0.0f };
	float FloorWallAlpha = 0.5f;

	Vector SphereColor = { 0.8f, 0.0f, 0.0f };
	float SphereAlpha = 1.0f;

	Vector CubeColor = { 0.7f, 0.7f, 0.0f };
	float CubeAlpha = 0.8f;

	Vector CapsuleColor = { 1.0f, 0.0f, 1.0f };
	float CapsuleAlpha = 0.4f;

	bool WeightedOITQuads = true;

	Vector BackgroundColor = { 135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f };
	bool BackgroundColorOnOff = true;

	bool SumColor = true;
	bool SumWeight = true;

	ESampleCameraPos SampleCameraPos = ESampleCameraPos::Quads;

	Vector SphereShellColor[4] = { {0.525f, 0.898f, 0.498f}, {0.949f, 0.796f, 0.38f}, {0.647f, 0.4f, 1.0f}, {1.0f, 0.698f, 0.85f} };
	float SphereShellAlpha[4] = { 0.5f, 0.5f, 0.5f, 0.5f };

	virtual void Setup(jAppSettingBase* appSetting) override;
	virtual void Teardown(jAppSettingBase* appSetting) override;

	void SwitchShadowType(jAppSettingBase* appSetting);
	void SwitchShadowMapType(jAppSettingBase* appSetting);

private:
	jShadowAppSettingProperties() {}

	static jShadowAppSettingProperties* _instance;
};

