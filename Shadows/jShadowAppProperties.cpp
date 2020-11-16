#include <pch.h>
#include "jShadowAppProperties.h"

jShadowAppSettingProperties* jShadowAppSettingProperties::_instance = nullptr;

void jShadowAppSettingProperties::Setup(jAppSettingBase* appSetting)
{
	appSetting->AddVariable("ShowWaveTexture", ShowWaveTexture);
	appSetting->AddVariable("ShowCubeEnvMap", ShowCubeEnvMap);
	appSetting->AddVariable("ShowWaterGround", ShowWaterGround);
	appSetting->AddVariable("ShowWireFrame", ShowWireFrame);
	appSetting->AddVariable("PauseGeoWave", PauseGeoWave);
	appSetting->AddVariable("PauseTexWave", PauseTexWave);

	appSetting->AddVariable("GeoWaveIndex", GeoWaveIndex);
	appSetting->SetMinMax("GeoWaveIndex", -1, 3);

	//appSetting->AddVariable("TexWaveIndex", TexWaveIndex);
	//appSetting->SetMinMax("TexWaveIndex", -1, 15);

	appSetting->AddVariable("WaterLevel", WaterLevel);
	appSetting->SetStep("WaterLevel", 0.05f);
	appSetting->SetGroup("WaterLevel", "GeoWave");

	appSetting->AddVariable("GeoWaveHeight", GeoWaveHeight);
	appSetting->SetStep("GeoWaveHeight", 0.001f);
	appSetting->SetMinMax("GeoWaveHeight", 0.0f, 1.0f);
	appSetting->SetGroup("GeoWaveHeight", "GeoWave");

	appSetting->AddVariable("GeoWaveChoppiness", GeoWaveChoppiness);
	appSetting->SetStep("GeoWaveChoppiness", 0.1f);
	appSetting->SetMinMax("GeoWaveChoppiness", 0.0f, 15.0f);
	appSetting->SetGroup("GeoWaveChoppiness", "GeoWave");

	appSetting->AddVariable("GeoWaveAngleDeviation", GeoWaveAngleDeviation);
	appSetting->SetStep("GeoWaveAngleDeviation", 1.0f);
	appSetting->SetMinMax("GeoWaveAngleDeviation", 0.0f, 180.0f);
	appSetting->SetGroup("GeoWaveAngleDeviation", "GeoWave");

	appSetting->AddVariable("EnvMapRadius", EnvMapRadius);
	appSetting->SetStep("EnvMapRadius", 1.0f);
	appSetting->SetMinMax("EnvMapRadius", 100.f, 10000.f);
	appSetting->SetGroup("EnvMapRadius", "GeoWave");

	appSetting->AddVariable("EnvMapHeight", EnvMapHeight);
	appSetting->SetStep("EnvMapHeight", 1.0f);
	appSetting->SetMinMax("EnvMapHeight", -100.f, 100.f);
	appSetting->SetGroup("EnvMapHeight", "GeoWave");

	appSetting->AddVariable("GeoWaveReInit", GeoWaveReInit);
	appSetting->SetGroup("GeoWaveReInit", "GeoWave");

	appSetting->AddVariable("TexWaveHeight", TexWaveHeight);
	appSetting->SetStep("TexWaveHeight", 0.001f);
	appSetting->SetMinMax("TexWaveHeight", 0.0f, 0.5f);
	appSetting->SetGroup("TexWaveHeight", "TexWave");

	appSetting->AddVariable("TexWaveScaling", TexWaveScaling);
	appSetting->SetStep("TexWaveScaling", 1.0f);
	appSetting->SetMinMax("TexWaveScaling", 5.0f, 50.0f);
	appSetting->SetGroup("TexWaveScaling", "TexWave");

	appSetting->AddVariable("TexNoise", TexNoise);
	appSetting->SetStep("TexNoise", 0.01f);
	appSetting->SetMinMax("TexNoise", 0.0f, 1.0f);
	appSetting->SetGroup("TexNoise", "TexWave");

	appSetting->AddVariable("TexWaveAngleDeviation", TexWaveAngleDeviation);
	appSetting->SetStep("TexWaveAngleDeviation", 1.0f);
	appSetting->SetMinMax("TexWaveAngleDeviation", 0.0f, 180.0f);
	appSetting->SetGroup("TexWaveAngleDeviation", "TexWave");

	appSetting->AddVariable("TexWaveReInit", TexWaveReInit);
	appSetting->SetGroup("TexWaveReInit", "TexWave");
}

void jShadowAppSettingProperties::Teardown(jAppSettingBase* appSetting)
{
	appSetting->RemoveVariable("ShowWaveTexture");
	appSetting->RemoveVariable("ShowCubeEnvMap");
	appSetting->RemoveVariable("ShowWaterGround");	
	appSetting->RemoveVariable("PauseGeoWave");
	appSetting->RemoveVariable("PauseTexWave");
	appSetting->RemoveVariable("GeoWaveIndex");
	//appSetting->RemoveVariable("TexWaveIndex");	
	appSetting->RemoveVariable("ShowWireFrame");
	appSetting->RemoveVariable("WaterLevel");
	appSetting->RemoveVariable("GeoWaveHeight");
	appSetting->RemoveVariable("GeoWaveChoppiness");
	appSetting->RemoveVariable("TexWaveHeight");
	appSetting->RemoveVariable("TexWaveScaling");
	appSetting->RemoveVariable("TexNoise");
	appSetting->RemoveVariable("GeoWaveAngleDeviation");
	appSetting->RemoveVariable("TexWaveAngleDeviation");
	appSetting->RemoveVariable("EnvMapRadius");
	appSetting->RemoveVariable("EnvMapHeight");
	appSetting->RemoveVariable("GeoWaveReInit");
	appSetting->RemoveVariable("TexWaveReInit");
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
	//appSetting->SetVisible("CSMDebugOn", (ShadowMapType == EShadowMapType::CSM_SSM));

	//switch (ShadowMapType)
	//{
	//case EShadowMapType::PCF:
	//case EShadowMapType::PCSS:
	//	appSetting->SetVisible("UsePoissonSample", 1);
	//	break;
	//default:
	//	appSetting->SetVisible("UsePoissonSample", 0);
	//	break;
	//}
}