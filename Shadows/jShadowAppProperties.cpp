#include <pch.h>
#include "jShadowAppProperties.h"

jShadowAppSettingProperties* jShadowAppSettingProperties::_instance = nullptr;

void jShadowAppSettingProperties::Setup(jAppSettingBase* appSetting)
{
	appSetting->AddVariable("ShadowOn", ShadowOn);

	//appSetting->AddEnumVariable("ShadowType", ShadowType, "EShadowType", EShadowTypeString);
	//appSetting->AddEnumVariable("ShadowMapType", ShadowMapType, "EShadowMapType", EShadowMapTypeString);
	//appSetting->AddVariable("UsePoissonSample", UsePoissonSample);
	//appSetting->AddVariable("DirectionalLightMap", ShowDirectionalLightMap);
	appSetting->AddVariable("UseTonemap", UseTonemap);
	appSetting->AddVariable("AdaptationRate", AdaptationRate);
	appSetting->SetStep("AdaptationRate", 0.01f);
	appSetting->SetMinMax("AdaptationRate", 0.0f, 4.0f);
	appSetting->AddVariable("AutoExposureKeyValue", AutoExposureKeyValue);
	appSetting->SetStep("AutoExposureKeyValue", 0.01f);
	appSetting->SetMinMax("AutoExposureKeyValue", 0.0f, 1.0f);
	//appSetting->AddVariable("IsGPUShadowVolume", IsGPUShadowVolume);

	////////////////////////////////////////////////////////////////////////////
	//// Silhouette Group
	//appSetting->AddVariable("DirectionalLight_Silhouette", ShowSilhouette_DirectionalLight);
	//appSetting->SetGroup("DirectionalLight_Silhouette", "Silhouette");
	//appSetting->SetLabel("DirectionalLight_Silhouette", "DirectionalLight");

	//appSetting->AddVariable("PointLight_Silhouette", ShowSilhouette_PointLight);
	//appSetting->SetGroup("PointLight_Silhouette", "Silhouette");
	//appSetting->SetLabel("PointLight_Silhouette", "PointLight");

	//appSetting->AddVariable("SpotLight_Silhouette", ShowSilhouette_SpotLight);
	//appSetting->SetGroup("SpotLight_Silhouette", "Silhouette");
	//appSetting->SetLabel("SpotLight_Silhouette", "SpotLight");

	//////////////////////////////////////////////////////////////////////////
	// LightInfo Group
	appSetting->AddVariable("DirectionalLightOn", DirectionalLightOn);
	appSetting->SetGroup("DirectionalLightOn", "LightInfo");
	appSetting->SetLabel("DirectionalLightOn", "DirectionalLightOn");

	//appSetting->AddVariable("PointLightOn", PointLightOn);
	//appSetting->SetGroup("PointLightOn", "LightInfo");
	//appSetting->SetLabel("PointLightOn", "PointLightOn");

	//appSetting->AddVariable("SpotLightOn", SpotLightOn);
	//appSetting->SetGroup("SpotLightOn", "LightInfo");
	//appSetting->SetLabel("SpotLightOn", "SpotLightOn");

	//appSetting->AddVariable("DirectionalLight_Info", ShowDirectionalLightInfo);
	//appSetting->SetGroup("DirectionalLight_Info", "LightInfo");
	//appSetting->SetLabel("DirectionalLight_Info", "DirectionalLightInfo");

	//appSetting->AddVariable("PointLight_Info", ShowPointLightInfo);
	//appSetting->SetGroup("PointLight_Info", "LightInfo");
	//appSetting->SetLabel("PointLight_Info", "PointLightInfo");

	//appSetting->AddVariable("SpotLight_Info", ShowSpotLightInfo);
	//appSetting->SetGroup("SpotLight_Info", "LightInfo");
	//appSetting->SetLabel("SpotLight_Info", "SpotLightInfo");
	//////////////////////////////////////////////////////////////////////////
	// Light Setting
	appSetting->AddDirectionVariable("DirecionalLight_Direction", DirecionalLightDirection);
	appSetting->SetGroup("DirecionalLight_Direction", "DirectionalLight");
	appSetting->SetLabel("DirecionalLight_Direction", "Direction");

	// Skin
	appSetting->AddVariable("RoughnessScale", RoughnessScale);
	appSetting->SetStep("RoughnessScale", 0.003f);
	appSetting->SetMinMax("RoughnessScale", 0.2f, 1.0f);
	appSetting->SetGroup("DirecionalLight_Direction", "RoughnessScale");

	appSetting->AddVariable("SpecularScale", SpecularScale);
	appSetting->SetStep("SpecularScale", 0.01f);
	appSetting->SetMinMax("SpecularScale", 0.0f, 3.0);
	appSetting->SetGroup("DirecionalLight_Direction", "SpecularScale");

	appSetting->AddVariable("GaussianStepScale", GaussianStepScale);
	appSetting->SetStep("GaussianStepScale", 0.1f);
	appSetting->SetMinMax("GaussianStepScale", 1.0f, 20.0f);
	appSetting->SetGroup("Skin", "GaussianStepScale");

	appSetting->AddVariable("PreScatterWeight", PreScatterWeight);
	appSetting->SetStep("PreScatterWeight", 0.003f);
	appSetting->SetMinMax("PreScatterWeight", 0.0f, 1.0f);
	appSetting->SetGroup("Skin", "PreScatterWeight");

	appSetting->AddVariable("EnableTSM", EnableTSM);
	appSetting->SetGroup("Skin", "EnableTSM");

	appSetting->AddVariable("EnergyConversion", EnergyConversion);
	appSetting->SetGroup("Skin", "EnergyConversion");

	appSetting->AddVariable("ShowIrradianceMap", ShowIrradianceMap);
	appSetting->SetGroup("Debug", "ShowIrradianceMap");
	appSetting->AddVariable("ShowStretchMap", ShowStretchMap);
	appSetting->SetGroup("Debug", "ShowStretchMap");
	appSetting->AddVariable("TSMMap", ShowTSMMap);
	appSetting->SetGroup("Debug", "TSMMap");
	appSetting->AddVariable("PdhBRDFBaked", ShowPdhBRDFBaked);
	appSetting->SetGroup("Debug", "PdhBRDFBaked");
	appSetting->AddVariable("VisualizeRangeSeam", VisualizeRangeSeam);
	appSetting->SetGroup("Debug", "VisualizeRangeSeam");
	appSetting->AddVariable("ShowShadowMap", ShowShadowMap);
	appSetting->SetGroup("Debug", "ShowShadowMap");
	appSetting->AddEnumVariable("SkinShading", SkinShading, "ESkinShading", ESkinShadingString);
	appSetting->SetGroup("Debug", "SkinShading");
	appSetting->AddVariable("FastBloomAndTonemap", FastBloomAndTonemap);
	appSetting->SetGroup("Debug", "FastBloomAndTonemap");

	//appSetting->AddVariable("PointLight_PositionX", PointLightPosition.x);
	//appSetting->SetGroup("PointLight_PositionX", "PointLight");
	//appSetting->SetLabel("PointLight_PositionX", "X");
	//appSetting->AddVariable("PointLight_PositionY", PointLightPosition.y);
	//appSetting->SetGroup("PointLight_PositionY", "PointLight");
	//appSetting->SetLabel("PointLight_PositionY", "Y");
	//appSetting->AddVariable("PointLight_PositionZ", PointLightPosition.z);
	//appSetting->SetGroup("PointLight_PositionZ", "PointLight");
	//appSetting->SetLabel("PointLight_PositionZ", "Z");

	//appSetting->AddVariable("SpotLight_PositionX", SpotLightPosition.x);
	//appSetting->SetGroup("SpotLight_PositionX", "SpotLightPos");
	//appSetting->SetLabel("SpotLight_PositionX", "X");
	//appSetting->AddVariable("SpotLight_PositionY", SpotLightPosition.y);
	//appSetting->SetGroup("SpotLight_PositionY", "SpotLightPos");
	//appSetting->SetLabel("SpotLight_PositionY", "Y");
	//appSetting->AddVariable("SpotLight_PositionZ", SpotLightPosition.z);
	//appSetting->SetGroup("SpotLight_PositionZ", "SpotLightPos");
	//appSetting->SetLabel("SpotLight_PositionZ", "Z");

	//appSetting->AddDirectionVariable("SpotLight_Direction", SpotLightDirection);
	//appSetting->SetGroup("SpotLight_Direction", "SpotLight");
	//appSetting->SetLabel("SpotLight_Direction", "Direction");

	////////////////////////////////////////////////////////////////////////////
	//// Box Group
	//appSetting->AddVariable("Box", ShowBoundBox);
	//appSetting->SetGroup("Box", "Bound");
	//appSetting->AddVariable("Sphere", ShowBoundSphere);
	//appSetting->SetGroup("Sphere", "Bound");

	////////////////////////////////////////////////////////////////////////////
	//// Deep Shadow Option
	//appSetting->AddVariable("DeepShadowAlpha", DeepShadowAlpha);
	//appSetting->SetGroup("DeepShadowAlpha", "DeepShadow");
	//appSetting->SetStep("DeepShadowAlpha", 0.01f);
	//appSetting->SetMinMax("DeepShadowAlpha", 0.05f, 1.0f);
	//
	//appSetting->AddVariable("ExponentDeepShadowOn", ExponentDeepShadowOn);
	//appSetting->SetGroup("ExponentDeepShadowOn", "DeepShadow");

	////////////////////////////////////////////////////////////////////////////
	//// Cascade Shadow Option
	//appSetting->AddVariable("CSMDebugOn", CSMDebugOn);
	//appSetting->SetGroup("CSMDebugOn", "CascadeShadow");

	//////////////////////////////////////////////////////////////////////////
	// Bloom Option
	appSetting->AddVariable("BloomThreshold", BloomThreshold);
	appSetting->SetStep("BloomThreshold", 0.01f);
	appSetting->SetMinMax("BloomThreshold", 0.0f, 20.0f);
	appSetting->SetGroup("BloomThreshold", "Bloom");
	appSetting->AddVariable("BloomBlurSigma", BloomBlurSigma);
	appSetting->SetStep("BloomBlurSigma", 0.01f);
	appSetting->SetMinMax("BloomBlurSigma", 0.5f, 1.5f);
	appSetting->SetGroup("BloomBlurSigma", "Bloom");
	appSetting->AddVariable("BloomMagnitude", BloomMagnitude);
	appSetting->SetStep("BloomMagnitude", 0.01f);
	appSetting->SetMinMax("BloomMagnitude", 0.0f, 2.0f);
	appSetting->SetGroup("BloomMagnitude", "Bloom");	

	appSetting->AddVariable("CosmeticLayer1_KR", CosmeticLayerData[0].KR);
	appSetting->SetLabel("CosmeticLayer1_KR", "Absorption per unit(K)");
	appSetting->SetGroup("CosmeticLayer1_KR", "CosmeticLayer1");
	appSetting->SetStep("CosmeticLayer1_KR", 0.01f);
	appSetting->SetMinMax("CosmeticLayer1_KR", 0.0f, 1.0f);
	appSetting->AddVariable("CosmeticLayer1_KG", CosmeticLayerData[0].KG);
	appSetting->SetLabel("CosmeticLayer1_KG", "Absorption per unit(K)");
	appSetting->SetGroup("CosmeticLayer1_KG", "CosmeticLayer1");
	appSetting->SetStep("CosmeticLayer1_KG", 0.01f);
	appSetting->SetMinMax("CosmeticLayer1_KG", 0.0f, 1.0f);
	appSetting->AddVariable("CosmeticLayer1_KB", CosmeticLayerData[0].KB);
	appSetting->SetLabel("CosmeticLayer1_KB", "Absorption per unit(K)");
	appSetting->SetGroup("CosmeticLayer1_KB", "CosmeticLayer1");
	appSetting->SetStep("CosmeticLayer1_KB", 0.01f);
	appSetting->SetMinMax("CosmeticLayer1_KB", 0.0f, 1.0f);

	appSetting->AddVariable("CosmeticLayer1_SR", CosmeticLayerData[0].SR);
	appSetting->SetLabel("CosmeticLayer1_SR", "Scaterring per unit(S)");
	appSetting->SetGroup("CosmeticLayer1_SR", "CosmeticLayer1");
	appSetting->SetStep("CosmeticLayer1_SR", 0.01f);
	appSetting->SetMinMax("CosmeticLayer1_SR", 0.0f, 1.0f);
	appSetting->AddVariable("CosmeticLayer1_SG", CosmeticLayerData[0].SG);
	appSetting->SetLabel("CosmeticLayer1_SG", "Scaterring per unit(S)");
	appSetting->SetGroup("CosmeticLayer1_SG", "CosmeticLayer1");
	appSetting->SetStep("CosmeticLayer1_SG", 0.01f);
	appSetting->SetMinMax("CosmeticLayer1_SG", 0.0f, 1.0f);
	appSetting->AddVariable("CosmeticLayer1_SB", CosmeticLayerData[0].SB);
	appSetting->SetLabel("CosmeticLayer1_SB", "Scaterring per unit(S)");
	appSetting->SetGroup("CosmeticLayer1_SB", "CosmeticLayer1");
	appSetting->SetStep("CosmeticLayer1_SB", 0.01f);
	appSetting->SetMinMax("CosmeticLayer1_SB", 0.0f, 1.0f);

	appSetting->AddVariable("CosmeticLayer1_X", CosmeticLayerData[0].X);
	appSetting->SetLabel("CosmeticLayer1_X", "Thickness of a layer");
	appSetting->SetGroup("CosmeticLayer1_X", "CosmeticLayer1");
	appSetting->SetStep("CosmeticLayer1_X", 0.001f);
	appSetting->SetMinMax("CosmeticLayer1_X", 0.0f, 1.0f);

	appSetting->AddVariable("CosmeticLayer2_KR", CosmeticLayerData[1].KR);
	appSetting->SetLabel("CosmeticLayer2_KR", "Absorption per unit(K)");
	appSetting->SetGroup("CosmeticLayer2_KR", "CosmeticLayer2");
	appSetting->SetStep("CosmeticLayer2_KR", 0.001f);
	appSetting->SetMinMax("CosmeticLayer2_KR", 0.0f, 1.0f);
	appSetting->AddVariable("CosmeticLayer2_KG", CosmeticLayerData[1].KG);
	appSetting->SetLabel("CosmeticLayer2_KG", "Absorption per unit(K)");
	appSetting->SetGroup("CosmeticLayer2_KG", "CosmeticLayer2");
	appSetting->SetStep("CosmeticLayer2_KG", 0.001f);
	appSetting->SetMinMax("CosmeticLayer2_KG", 0.0f, 1.0f);
	appSetting->AddVariable("CosmeticLayer2_KB", CosmeticLayerData[1].KB);
	appSetting->SetLabel("CosmeticLayer2_KB", "Absorption per unit(K)");
	appSetting->SetGroup("CosmeticLayer2_KB", "CosmeticLayer2");
	appSetting->SetStep("CosmeticLayer2_KB", 0.001f);
	appSetting->SetMinMax("CosmeticLayer2_KB", 0.0f, 1.0f);

	appSetting->AddVariable("CosmeticLayer2_SR", CosmeticLayerData[1].SR);
	appSetting->SetLabel("CosmeticLayer2_SR", "Scaterring per unit(S)");
	appSetting->SetGroup("CosmeticLayer2_SR", "CosmeticLayer2");
	appSetting->SetStep("CosmeticLayer2_SR", 0.001f);
	appSetting->SetMinMax("CosmeticLayer2_SR", 0.0f, 1.0f);
	appSetting->AddVariable("CosmeticLayer2_SG", CosmeticLayerData[1].SG);
	appSetting->SetLabel("CosmeticLayer2_SG", "Scaterring per unit(S)");
	appSetting->SetGroup("CosmeticLayer2_SG", "CosmeticLayer2");
	appSetting->SetStep("CosmeticLayer2_SG", 0.001f);
	appSetting->SetMinMax("CosmeticLayer2_SG", 0.0f, 1.0f);
	appSetting->AddVariable("CosmeticLayer2_SB", CosmeticLayerData[1].SB);
	appSetting->SetLabel("CosmeticLayer2_SB", "Scaterring per unit(S)");
	appSetting->SetGroup("CosmeticLayer2_SB", "CosmeticLayer2");
	appSetting->SetStep("CosmeticLayer2_SB", 0.001f);
	appSetting->SetMinMax("CosmeticLayer2_SB", 0.0f, 1.0f);

	appSetting->AddVariable("CosmeticLayer2_X", CosmeticLayerData[1].X);
	appSetting->SetLabel("CosmeticLayer2_X", "Thickness of a layer");
	appSetting->SetGroup("CosmeticLayer2_X", "CosmeticLayer2");
	appSetting->SetStep("CosmeticLayer2_X", 0.001f);
	appSetting->SetMinMax("CosmeticLayer2_X", 0.0f, 1.0f);

}

void jShadowAppSettingProperties::Teardown(jAppSettingBase* appSetting)
{
	//appSetting->RemoveVariable("EShadowType");
	//appSetting->RemoveVariable("DirectionalLightSilhouette");
	//appSetting->RemoveVariable("PointLightSilhouette");
	//appSetting->RemoveVariable("SpotLightSilhouette");
	//appSetting->RemoveVariable("IsGPUShadowVolume");	
	//appSetting->RemoveVariable("ShadowMapType");
	//appSetting->RemoveVariable("UsePoissonSample");
	appSetting->RemoveVariable("DirectionalLightMap");
	appSetting->RemoveVariable("UseTonemap");
	appSetting->RemoveVariable("DirectionalLight_Info");
	appSetting->RemoveVariable("PointLight_Info");
	appSetting->RemoveVariable("SpotLight_Info");
	appSetting->RemoveVariable("DirecionalLight_Direction");
	//appSetting->RemoveVariable("PointLight_PositionX");
	//appSetting->RemoveVariable("PointLight_PositionY");
	//appSetting->RemoveVariable("PointLight_PositionZ");
	//appSetting->RemoveVariable("SpotLight_PositionX");
	//appSetting->RemoveVariable("SpotLight_PositionY");
	//appSetting->RemoveVariable("SpotLight_PositionZ");
	//appSetting->RemoveVariable("SpotLight_Direction");
	//appSetting->RemoveVariable("Box");
	//appSetting->RemoveVariable("Sphere");
	//appSetting->RemoveVariable("DeepShadowAlpha");
	//appSetting->RemoveVariable("ExponentDeepShadowOn");
	//appSetting->RemoveVariable("CSMDebugOn");
	appSetting->RemoveVariable("AdaptationRate");
	appSetting->RemoveVariable("AutoExposureKeyValue");
	appSetting->RemoveVariable("BloomThreshold");
	appSetting->RemoveVariable("BloomBlurSigma");
	appSetting->RemoveVariable("BloomMagnitude");

	appSetting->RemoveVariable("RoughnessScale");
	appSetting->RemoveVariable("SpecularScale");
	appSetting->RemoveVariable("PreScatterWeight");
	appSetting->RemoveVariable("EnableTSM");
	appSetting->RemoveVariable("VisualizeRangeSeam");
	appSetting->RemoveVariable("ShowIrradianceMap");
	appSetting->RemoveVariable("ShowStretchMap");
	appSetting->RemoveVariable("ShowTSMMap");
	appSetting->RemoveVariable("ShowPdhBRDFBaked");
	appSetting->RemoveVariable("ShowShadowMap");
	appSetting->RemoveVariable("EnergyConversion");
	appSetting->RemoveVariable("SkinShading");
	appSetting->RemoveVariable("GaussianStepScale");

	appSetting->RemoveVariable("CosmeticLayer1_SR");
	appSetting->RemoveVariable("CosmeticLayer1_SG");
	appSetting->RemoveVariable("CosmeticLayer1_SB");
	appSetting->RemoveVariable("CosmeticLayer1_KR");
	appSetting->RemoveVariable("CosmeticLayer1_KG");
	appSetting->RemoveVariable("CosmeticLayer1_KB");
	appSetting->RemoveVariable("CosmeticLayer1_X");
	appSetting->RemoveVariable("CosmeticLayer2_SR");
	appSetting->RemoveVariable("CosmeticLayer2_SG");
	appSetting->RemoveVariable("CosmeticLayer2_SB");
	appSetting->RemoveVariable("CosmeticLayer2_KR");
	appSetting->RemoveVariable("CosmeticLayer2_KG");
	appSetting->RemoveVariable("CosmeticLayer2_KB");
	appSetting->RemoveVariable("CosmeticLayer2_X");
}

void jShadowAppSettingProperties::SwitchShadowType(jAppSettingBase* appSetting)
{
	switch (ShadowType)
	{
	case EShadowType::ShadowVolume:
		appSetting->SetVisible("Silhouette", 1);
		appSetting->SetVisible("IsGPUShadowVolume", 1);
		appSetting->SetVisible("UsePoissonSample", 0);
		appSetting->SetVisible("DirectionalLightMap", 0);
		appSetting->SetVisible("ShadowMapType", 0);
		appSetting->SetVisible("CSMDebugOn", 0);
		break;
	case EShadowType::ShadowMap:
		appSetting->SetVisible("Silhouette", 0);
		appSetting->SetVisible("IsGPUShadowVolume", 0);
		appSetting->SetVisible("UsePoissonSample", 1);
		appSetting->SetVisible("DirectionalLightMap", 1);
		appSetting->SetVisible("ShadowMapType", 1);
		appSetting->SetVisible("CSMDebugOn", (ShadowMapType == EShadowMapType::CSM_SSM));
		break;
	}
}

void jShadowAppSettingProperties::SwitchShadowMapType(jAppSettingBase* appSetting)
{
	appSetting->SetVisible("CSMDebugOn", (ShadowMapType == EShadowMapType::CSM_SSM));

	switch (ShadowMapType)
	{
	case EShadowMapType::PCF:
	case EShadowMapType::PCSS:
		appSetting->SetVisible("UsePoissonSample", 1);
		break;
	default:
		appSetting->SetVisible("UsePoissonSample", 0);
		break;
	}
}
