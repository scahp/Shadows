#include <pch.h>
#include "jShadowAppProperties.h"

jShadowAppSettingProperties* jShadowAppSettingProperties::_instance = nullptr;

void jShadowAppSettingProperties::UpdateVisibleProperties(jAppSettingBase* appSetting)
{
	static auto PrevMappingType = MappingType;
	if (PrevMappingType == MappingType)
		return;

	PrevMappingType = MappingType;

	const bool IsVisibleBasicRelief = MappingType == EMappingType::Linear || MappingType == EMappingType::RelaxedCone;
	const bool IsVisibleDualDepthRelief = MappingType == EMappingType::DualDepth;
	const bool IsVisibleInterior = MappingType == EMappingType::Interior;
	const bool IsVisibleDualDepth_Interior = MappingType == EMappingType::DualDepth_Interior;

	appSetting->SetVisible("SingleDepth", IsVisibleBasicRelief);
	appSetting->SetVisible("DualDepth", IsVisibleDualDepthRelief || IsVisibleDualDepth_Interior);

	const bool IsVisibleSelectReliefTexture = IsVisibleDualDepthRelief && !IsVisibleDualDepth_Interior;
    appSetting->SetVisible("DualDepth_ReliefTexture", IsVisibleSelectReliefTexture);

	appSetting->SetVisible("AmbientColor0", IsVisibleDualDepth_Interior);
    appSetting->SetVisible("AmbientColor1", IsVisibleDualDepth_Interior);
    appSetting->SetVisible("AmbientColor2", IsVisibleDualDepth_Interior);
	appSetting->SetVisible("RoomCount", IsVisibleDualDepth_Interior);
}

void jShadowAppSettingProperties::Setup(jAppSettingBase* appSetting)
{
	appSetting->AddEnumVariable("MappingType", MappingType, "EMappingType", EMappingTypeString);
	appSetting->SetGroup("MappingType", "Type");

	appSetting->AddVariable("DepthBias", DepthBias);
	appSetting->SetGroup("DepthBias", "SingleDepth");

	appSetting->AddVariable("DepthScale", DepthScale);
	appSetting->SetMinMax("DepthScale", 0.0f, 2.0f);
	appSetting->SetStep("DepthScale", 0.001f);
	appSetting->SetGroup("DepthScale", "SingleDepth");

	appSetting->AddVariable("ShadowOn", ReliefShadowOn);
	appSetting->SetGroup("ShadowOn", "SingleDepth");

	appSetting->AddVariable("DualDepth_DepthBias", DualDepth_DepthBias);
	appSetting->SetLabel("DualDepth_DepthBias", "DepthBias");
	appSetting->SetGroup("DualDepth_DepthBias", "DualDepth");

	appSetting->AddVariable("DualDepth_DepthScale", DualDepth_DepthScale);
	appSetting->SetLabel("DualDepth_DepthScale", "DepthScale");
	appSetting->SetMinMax("DualDepth_DepthScale", 0.0f, 2.0f);
	appSetting->SetStep("DualDepth_DepthScale", 0.001f);
	appSetting->SetGroup("DualDepth_DepthScale", "DualDepth");

	appSetting->AddEnumVariable("DualDepth_ReliefTexture", ReliefTexture, "EDualDepthReliefTexture", EDualDepthReliefTextureString);
	appSetting->SetLabel("DualDepth_ReliefTexture", "ReliefTexture");
	appSetting->SetGroup("DualDepth_ReliefTexture", "DualDepth");

    appSetting->AddColorVariable("AmbientColor0", AmbientColors[0]);
    appSetting->AddColorVariable("AmbientColor1", AmbientColors[1]);
    appSetting->AddColorVariable("AmbientColor2", AmbientColors[2]);

	appSetting->AddVariable("RoomCount", RoomCount);
    appSetting->SetMinMax("RoomCount", 1, 12);

	//appSetting->AddVariable("ShadowOn", ShadowOn);

	//appSetting->AddEnumVariable("ShadowType", ShadowType, "EShadowType", EShadowTypeString);
	//appSetting->AddEnumVariable("ShadowMapType", ShadowMapType, "EShadowMapType", EShadowMapTypeString);
	//appSetting->AddVariable("UsePoissonSample", UsePoissonSample);
	//appSetting->AddVariable("DirectionalLightMap", ShowDirectionalLightMap);
	//appSetting->AddVariable("UseTonemap", UseTonemap);
	//appSetting->AddVariable("AdaptationRate", AdaptationRate);
	//appSetting->SetStep("AdaptationRate", 0.01f);
	//appSetting->SetMinMax("AdaptationRate", 0.0f, 4.0f);
	//appSetting->AddVariable("AutoExposureKeyValue", AutoExposureKeyValue);
	//appSetting->SetStep("AutoExposureKeyValue", 0.01f);
	//appSetting->SetMinMax("AutoExposureKeyValue", 0.0f, 1.0f);
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

	////////////////////////////////////////////////////////////////////////////
	//// LightInfo Group
	//appSetting->AddVariable("DirectionalLightOn", DirectionalLightOn);
	//appSetting->SetGroup("DirectionalLightOn", "LightInfo");
	//appSetting->SetLabel("DirectionalLightOn", "DirectionalLightOn");

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
	////////////////////////////////////////////////////////////////////////////
	//// Light Setting
	appSetting->AddDirectionVariable("DirecionalLight_Direction", DirecionalLightDirection);
	appSetting->SetGroup("DirecionalLight_Direction", "DirectionalLight");
	appSetting->SetLabel("DirecionalLight_Direction", "Direction");

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
	appSetting->AddVariable("Box", ShowBoundBox);
	appSetting->SetGroup("Box", "Bound");
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

	////////////////////////////////////////////////////////////////////////////
	//// Bloom Option
	//appSetting->AddVariable("BloomThreshold", BloomThreshold);
	//appSetting->SetStep("BloomThreshold", 0.01f);
	//appSetting->SetMinMax("BloomThreshold", 0.0f, 20.0f);
	//appSetting->SetGroup("BloomThreshold", "Bloom");
	//appSetting->AddVariable("BloomBlurSigma", BloomBlurSigma);
	//appSetting->SetStep("BloomBlurSigma", 0.01f);
	//appSetting->SetMinMax("BloomBlurSigma", 0.5f, 1.5f);
	//appSetting->SetGroup("BloomBlurSigma", "Bloom");
	//appSetting->AddVariable("BloomMagnitude", BloomMagnitude);
	//appSetting->SetStep("BloomMagnitude", 0.01f);
	//appSetting->SetMinMax("BloomMagnitude", 0.0f, 2.0f);
	//appSetting->SetGroup("BloomMagnitude", "Bloom");	
}

void jShadowAppSettingProperties::Teardown(jAppSettingBase* appSetting)
{
	appSetting->RemoveVariable("MappingType");
	appSetting->RemoveVariable("DepthBias");
	appSetting->RemoveVariable("DepthScale");
	appSetting->RemoveVariable("ShadowOn");

	appSetting->RemoveVariable("DualDepth_DepthBias");
	appSetting->RemoveVariable("DualDepth_DepthScale");
	appSetting->RemoveVariable("DualDepth_ReliefTexture");
	appSetting->RemoveVariable("ShadowOn");

	appSetting->RemoveVariable("AmbientColor0");
    appSetting->RemoveVariable("AmbientColor1");
    appSetting->RemoveVariable("AmbientColor2");

    appSetting->RemoveVariable("RoomCount");

	//appSetting->RemoveVariable("ShadowOn");
	//appSetting->RemoveVariable("EShadowType");
	//appSetting->RemoveVariable("DirectionalLightSilhouette");
	//appSetting->RemoveVariable("PointLightSilhouette");
	//appSetting->RemoveVariable("SpotLightSilhouette");
	//appSetting->RemoveVariable("IsGPUShadowVolume");	
	//appSetting->RemoveVariable("ShadowMapType");
	//appSetting->RemoveVariable("UsePoissonSample");
	//appSetting->RemoveVariable("DirectionalLightMap");
	//appSetting->RemoveVariable("UseTonemap");
	//appSetting->RemoveVariable("DirectionalLight_Info");
	//appSetting->RemoveVariable("PointLight_Info");
	//appSetting->RemoveVariable("SpotLight_Info");
	appSetting->RemoveVariable("DirecionalLight_Direction");
	//appSetting->RemoveVariable("PointLight_PositionX");
	//appSetting->RemoveVariable("PointLight_PositionY");
	//appSetting->RemoveVariable("PointLight_PositionZ");
	//appSetting->RemoveVariable("SpotLight_PositionX");
	//appSetting->RemoveVariable("SpotLight_PositionY");
	//appSetting->RemoveVariable("SpotLight_PositionZ");
	//appSetting->RemoveVariable("SpotLight_Direction");
	appSetting->RemoveVariable("Box");
	//appSetting->RemoveVariable("Sphere");
	//appSetting->RemoveVariable("DeepShadowAlpha");
	//appSetting->RemoveVariable("ExponentDeepShadowOn");
	//appSetting->RemoveVariable("CSMDebugOn");
	//appSetting->RemoveVariable("AdaptationRate");
	//appSetting->RemoveVariable("AutoExposureKeyValue");
	//appSetting->RemoveVariable("BloomThreshold");
	//appSetting->RemoveVariable("BloomBlurSigma");
	//appSetting->RemoveVariable("BloomMagnitude");	

	appSetting->RemoveVariable("DualDepthReliefTexture");
}

void jShadowAppSettingProperties::SwitchShadowType(jAppSettingBase* appSetting)
{
	//switch (ShadowType)
	//{
	//case EShadowType::ShadowVolume:
	//	appSetting->SetVisible("Silhouette", 1);
	//	appSetting->SetVisible("IsGPUShadowVolume", 1);
	//	appSetting->SetVisible("UsePoissonSample", 0);
	//	appSetting->SetVisible("DirectionalLightMap", 0);
	//	appSetting->SetVisible("ShadowMapType", 0);
	//	appSetting->SetVisible("CSMDebugOn", 0);
	//	break;
	//case EShadowType::ShadowMap:
	//	appSetting->SetVisible("Silhouette", 0);
	//	appSetting->SetVisible("IsGPUShadowVolume", 0);
	//	appSetting->SetVisible("UsePoissonSample", 1);
	//	appSetting->SetVisible("DirectionalLightMap", 1);
	//	appSetting->SetVisible("ShadowMapType", 1);
	//	appSetting->SetVisible("CSMDebugOn", (ShadowMapType == EShadowMapType::CSM_SSM));
	//	break;
	//}
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
