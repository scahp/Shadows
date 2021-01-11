#include <pch.h>
#include "jShadowAppProperties.h"

jShadowAppSettingProperties* jShadowAppSettingProperties::_instance = nullptr;

void jShadowAppSettingProperties::Setup(jAppSettingBase* appSetting)
{
	appSetting->AddVariable("ShooterPatchWire", SelectedShooterPatchAsDrawLine);
	appSetting->AddVariable("WireFrame", IsWireFrame);
	appSetting->AddVariable("WireFrameWhite", IsWireFrameWhite);
	appSetting->AddVariable("UseAmbient", IsAddAmbient);
	appSetting->AddVariable("DrawFormfactorsCam", IsDrawFormfactorsCamera);
	appSetting->AddVariable("UntilConverged", UntilConverged);
	appSetting->AddVariable("OnceProcess", OnceProcess);
}

void jShadowAppSettingProperties::Teardown(jAppSettingBase* appSetting)
{
	appSetting->RemoveVariable("ShooterPatchWire");	
	appSetting->RemoveVariable("WireFrame");
	appSetting->RemoveVariable("WireFrameWhite");
	appSetting->RemoveVariable("UseAmbient");
	appSetting->RemoveVariable("DrawFormfactorsCam");
	appSetting->RemoveVariable("UntilConverged");
	appSetting->RemoveVariable("OnceProcess");
}

void jShadowAppSettingProperties::SwitchShadowType(jAppSettingBase* appSetting)
{
}

void jShadowAppSettingProperties::SwitchShadowMapType(jAppSettingBase* appSetting)
{
}
