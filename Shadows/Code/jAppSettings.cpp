#include <pch.h>
#include "jAppSettings.h"

jAppSettings* jAppSettings::_instance = nullptr;

//////////////////////////////////////////////////////////////////////////
void jAppSettings::Init(int32 width, int32 height)
{
	// todo it's depend on rhi type
	TwInit(TW_OPENGL_CORE, nullptr);
	TwWindowSize(width, height);

	auto mainPannel = jAppSettings::GetInstance().AddTwBar("MainPannel");		// Default Pannel
	mainPannel->SetSizeBar(250, 100);
	mainPannel->SetValueSize(150);
}

jAppSettingBase* jAppSettings::AddTwBar(const char* barName)
{
	auto it_find = BarMap.find(barName);
	if (it_find != BarMap.end())
		delete it_find->second;

	auto appSettings = new jAppSettingBase(TwNewBar(barName));
	BarMap[barName] = appSettings;
	return appSettings;
}

void jAppSettings::DeleteTwBar(const char* barName)
{
	auto it_find = BarMap.find(barName);
	if (it_find != BarMap.end())
	{
		delete it_find->second;
		BarMap.erase(it_find);
	}
}

//////////////////////////////////////////////////////////////////////////

void jAppSettingBase::SetSizeBar(int sizeX, int sizeY)
{
	char szTemp[256] = { 0, };
	sprintf_s(szTemp, sizeof(szTemp) - 1, "%s size='%d %d'", TwGetBarName(Bar), sizeX, sizeY);
	TwDefine(szTemp);
}

void jAppSettingBase::SetPositionBar(int ix, int iy)
{
	char szTemp[256] = { 0, };
	sprintf_s(szTemp, sizeof(szTemp) - 1, "%s position='%d %d'", TwGetBarName(Bar), ix, iy);
	TwDefine(szTemp);
}

void jAppSettingBase::SetValueSizeFit()
{
	char szTemp[256] = { 0, };
	sprintf_s(szTemp, sizeof(szTemp) - 1, "%s valueswidth=fit", TwGetBarName(Bar));
	TwDefine(szTemp);
}

void jAppSettingBase::SetValueSize(int32 valueWidth)
{
	char szTemp[256] = { 0, };
	sprintf_s(szTemp, sizeof(szTemp) - 1, "%s valueswidth=%d", TwGetBarName(Bar), valueWidth);
	TwDefine(szTemp);
}
