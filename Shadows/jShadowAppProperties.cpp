#include <pch.h>
#include "jShadowAppProperties.h"

jShadowAppSettingProperties* jShadowAppSettingProperties::_instance = nullptr;

//////////////////////////////////////////////////////////////////////////
// CosmeticLayer
void CosmeticLayer::AddVariable(jAppSettingBase* appSetting, const std::string& name, const std::string& group)
{
	const std::string& Kr = (name + "_KR");
	const std::string& Kg = (name + "_KG");
	const std::string& Kb = (name + "_KB");

	appSetting->AddVariable(Kr.c_str(), KR);
	appSetting->SetLabel(Kr.c_str(), "Absorption per unit(KR)");
	appSetting->SetGroup(Kr.c_str(), group.c_str());
	appSetting->SetStep(Kr.c_str(), 0.01f);
	appSetting->SetMinMax(Kr.c_str(), 0.0f, 50.0f);

	appSetting->AddVariable(Kg.c_str(), KG);
	appSetting->SetLabel(Kg.c_str(), "Absorption per unit(KG)");
	appSetting->SetGroup(Kg.c_str(), group.c_str());
	appSetting->SetStep(Kg.c_str(), 0.01f);
	appSetting->SetMinMax(Kg.c_str(), 0.0f, 50.0f);

	appSetting->AddVariable(Kb.c_str(), KB);
	appSetting->SetLabel(Kb.c_str(), "Absorption per unit(KB)");
	appSetting->SetGroup(Kb.c_str(), group.c_str());
	appSetting->SetStep(Kb.c_str(), 0.01f);
	appSetting->SetMinMax(Kb.c_str(), 0.0f, 50.0f);

	const std::string& Sr = (name + "_SR");
	const std::string& Sg = (name + "_SG");
	const std::string& Sb = (name + "_SB");

	appSetting->AddVariable(Sr.c_str(), SR);
	appSetting->SetLabel(Sr.c_str(), "Scaterring per unit(SR)");
	appSetting->SetGroup(Sr.c_str(), group.c_str());
	appSetting->SetStep(Sr.c_str(), 0.01f);
	appSetting->SetMinMax(Sr.c_str(), 0.0f, 50.0f);

	appSetting->AddVariable(Sg.c_str(), SG);
	appSetting->SetLabel(Sg.c_str(), "Scaterring per unit(SG)");
	appSetting->SetGroup(Sg.c_str(), group.c_str());
	appSetting->SetStep(Sg.c_str(), 0.01f);
	appSetting->SetMinMax(Sg.c_str(), 0.0f, 50.0f);

	appSetting->AddVariable(Sb.c_str(), SB);
	appSetting->SetLabel(Sb.c_str(), "Scaterring per unit(SB)");
	appSetting->SetGroup(Sb.c_str(), group.c_str());
	appSetting->SetStep(Sb.c_str(), 0.01f);
	appSetting->SetMinMax(Sb.c_str(), 0.0f, 50.0f);

	const std::string& Thickness = (name + "_X");

	appSetting->AddVariable(Thickness.c_str(), X);
	appSetting->SetLabel(Thickness.c_str(), "Thickness of a layer");
	appSetting->SetGroup(Thickness.c_str(), group.c_str());
	appSetting->SetStep(Thickness.c_str(), 0.001f);
	appSetting->SetMinMax(Thickness.c_str(), 0.0f, 1.0f);
}

void CosmeticLayer::RemoveVariable(jAppSettingBase* appSetting, const std::string& name) const
{
	const std::string& Kr = (name + "_KR");
	const std::string& Kg = (name + "_KG");
	const std::string& Kb = (name + "_KB");

	appSetting->RemoveVariable(Kr.c_str());
	appSetting->RemoveVariable(Kg.c_str());
	appSetting->RemoveVariable(Kb.c_str());

	const std::string& Sr = (name + "_SR");
	const std::string& Sg = (name + "_SG");
	const std::string& Sb = (name + "_SB");

	appSetting->RemoveVariable(Sr.c_str());
	appSetting->RemoveVariable(Sg.c_str());
	appSetting->RemoveVariable(Sb.c_str());

	const std::string& Thickness = (name + "_X");

	appSetting->RemoveVariable(Thickness.c_str());
}

void CosmeticLayer::SetVisibleVariable(jAppSettingBase* appSetting, const std::string& name, bool visible) const
{
	const std::string& Kr = (name + "_KR");
	const std::string& Kg = (name + "_KG");
	const std::string& Kb = (name + "_KB");

	appSetting->SetVisible(Kr.c_str(), visible);
	appSetting->SetVisible(Kg.c_str(), visible);
	appSetting->SetVisible(Kb.c_str(), visible);

	const std::string& Sr = (name + "_SR");
	const std::string& Sg = (name + "_SG");
	const std::string& Sb = (name + "_SB");

	appSetting->SetVisible(Sr.c_str(), visible);
	appSetting->SetVisible(Sg.c_str(), visible);
	appSetting->SetVisible(Sb.c_str(), visible);

	const std::string& Thickness = (name + "_X");

	appSetting->SetVisible(Thickness.c_str(), visible);
}
//////////////////////////////////////////////////////////////////////////

// jShadowAppSettingProperties
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

	appSetting->AddVariable("ShowCosmeticMap", ShowCosmeticMap);
	appSetting->AddVariable("IsCustomCosmetic?", IsCustomCosmeticLayer);

	appSetting->AddEnumVariable("ECosmeticLayer0", CosmeticLayer0, "ECosmeticLayer", ECosmeticLayerString);
	appSetting->AddVariable("CosmeticLayer0_Thickness", CosmeticLayer0_Thickness);
	appSetting->SetStep("CosmeticLayer0_Thickness", 0.001f);
	appSetting->SetMinMax("CosmeticLayer0_Thickness", 0.0f, 1.0f);

	appSetting->AddEnumVariable("ECosmeticLayer1", CosmeticLayer1, "ECosmeticLayer", ECosmeticLayerString);
	appSetting->AddVariable("CosmeticLayer1_Thickness", CosmeticLayer1_Thickness);
	appSetting->SetStep("CosmeticLayer1_Thickness", 0.001f);
	appSetting->SetMinMax("CosmeticLayer1_Thickness", 0.0f, 1.0f);

	appSetting->AddEnumVariable("ECosmeticLayer2", CosmeticLayer2, "ECosmeticLayer", ECosmeticLayerString);
	appSetting->AddVariable("CosmeticLayer2_Thickness", CosmeticLayer2_Thickness);
	appSetting->SetStep("CosmeticLayer2_Thickness", 0.001f);
	appSetting->SetMinMax("CosmeticLayer2_Thickness", 0.0f, 1.0f);

	CustomCosmeticLayer0.AddVariable(appSetting, "Layer0", "CustomCosmetic0");
	CustomCosmeticLayer1.AddVariable(appSetting, "Layer1", "CustomCosmetic1");
	CustomCosmeticLayer2.AddVariable(appSetting, "Layer2", "CustomCosmetic2");
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

	appSetting->RemoveVariable("IsCustomCosmetic?");

	appSetting->RemoveVariable("ECosmeticLayer0");
	appSetting->RemoveVariable("CosmeticLayer0_Thickness");
	appSetting->RemoveVariable("ECosmeticLayer1");
	appSetting->RemoveVariable("CosmeticLayer1_Thickness");
	appSetting->RemoveVariable("ECosmeticLayer2");
	appSetting->RemoveVariable("CosmeticLayer2_Thickness");

	CustomCosmeticLayer0.RemoveVariable(appSetting, "Layer0");
	CustomCosmeticLayer1.RemoveVariable(appSetting, "Layer1");
	CustomCosmeticLayer2.RemoveVariable(appSetting, "Layer2");

}

void jShadowAppSettingProperties::SwitchShadowType(jAppSettingBase* appSetting) const
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

void jShadowAppSettingProperties::SwitchShadowMapType(jAppSettingBase* appSetting) const
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

void jShadowAppSettingProperties::SwitchCosmeticLayer(jAppSettingBase* appSetting) const
{
	CustomCosmeticLayer0.SetVisibleVariable(appSetting, "Layer0", IsCustomCosmeticLayer);
	CustomCosmeticLayer1.SetVisibleVariable(appSetting, "Layer1", IsCustomCosmeticLayer);
	CustomCosmeticLayer2.SetVisibleVariable(appSetting, "Layer2", IsCustomCosmeticLayer);

	appSetting->SetVisible("ECosmeticLayer0", !IsCustomCosmeticLayer);
	appSetting->SetVisible("CosmeticLayer0_Thickness", !IsCustomCosmeticLayer);
	appSetting->SetVisible("ECosmeticLayer1", !IsCustomCosmeticLayer);
	appSetting->SetVisible("CosmeticLayer1_Thickness", !IsCustomCosmeticLayer);
	appSetting->SetVisible("ECosmeticLayer2", !IsCustomCosmeticLayer);
	appSetting->SetVisible("CosmeticLayer2_Thickness", !IsCustomCosmeticLayer);
}

