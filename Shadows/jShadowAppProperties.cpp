#include <pch.h>
#include "jShadowAppProperties.h"

jShadowAppSettingProperties* jShadowAppSettingProperties::_instance = nullptr;

void jShadowAppSettingProperties::Setup(jAppSettingBase* appSetting)
{
	appSetting->AddDirectionVariable("DirecionalLight_Direction", DirecionalLightDirection);
	appSetting->SetGroup("DirecionalLight_Direction", "DirectionalLight");
	appSetting->SetLabel("DirecionalLight_Direction", "Direction");

	appSetting->AddVariable("Rayleigh", Kr);
	appSetting->SetMinMax("Rayleigh", 0.0f, 1.0f);
	appSetting->SetStep("Rayleigh", 0.0001f);
	appSetting->SetGroup("Rayleigh", "Scattering Constant");

	appSetting->AddVariable("Mie", Km);
	appSetting->SetMinMax("Mie", 0.0f, 1.0f);
	appSetting->SetStep("Mie", 0.0001f);
	appSetting->SetGroup("Mie", "Scattering Constant");

	appSetting->AddVariable("Brightness", ESun);
	appSetting->SetGroup("Brightness", "Sun");
	appSetting->SetMinMax("Brightness", 0.0f, 200.0f);
	appSetting->SetStep("Brightness", 0.1f);

	appSetting->AddVariable("Mie Asymmetry", g);
	appSetting->SetMinMax("Mie Asymmetry", -0.999f, -0.75f);
	appSetting->SetStep("Mie Asymmetry", 0.001f);

	appSetting->AddVariable("InnerRadius", InnerRadius);
	appSetting->SetMinMax("InnerRadius", 1.0f, 100.0f);
	appSetting->SetStep("InnerRadius", 0.01f);

	appSetting->AddVariable("OuterRadius", OuterRadius);
	appSetting->SetMinMax("OuterRadius", 1.25f, 100.25f);
	appSetting->SetStep("OuterRadius", 0.01f);

	appSetting->SetMinMax("Mie Asymmetry", -0.999f, -0.75f);
	appSetting->SetStep("Mie Asymmetry", 0.001f);

	appSetting->AddVariable("AverageScaleDepth", AverageScaleDepth);
	appSetting->SetMinMax("AverageScaleDepth", 0.0f, 1.0f);
	appSetting->SetStep("AverageScaleDepth", 0.01f);

	appSetting->AddVariable("Exposure", Exposure);
	appSetting->SetMinMax("Exposure", 0.1f, 10.0f);
	appSetting->SetStep("Exposure", 0.01f);

	appSetting->AddVariable("Use HDR", UseHDR);
	appSetting->AddVariable("AutoMoveSun", AutoMoveSun);

	appSetting->AddColorVariable("ScatterColor", ScatterColor);
}

void jShadowAppSettingProperties::Teardown(jAppSettingBase* appSetting)
{
	appSetting->RemoveVariable("DirecionalLight_Direction");
	appSetting->RemoveVariable("Rayleigh");
	appSetting->RemoveVariable("Mie");
	appSetting->RemoveVariable("Brightness");
	appSetting->RemoveVariable("Mie Asymmetry");
	appSetting->RemoveVariable("InnerRadius");
	appSetting->RemoveVariable("OuterRadius");
	appSetting->RemoveVariable("RayleighScaleDepth");
	appSetting->RemoveVariable("MieScaleDepth");
	appSetting->RemoveVariable("Exposure");
	appSetting->RemoveVariable("Use HDR");
	appSetting->RemoveVariable("AutoMoveSun");
	appSetting->RemoveVariable("ScatterColor");
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