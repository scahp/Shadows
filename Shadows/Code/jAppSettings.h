#pragma once
#include "jShadowTypes.h"
#include "AntTweakBar.h"
#include "Math/Vector.h"

class jAppSettingBase
{
public:
	jAppSettingBase(TwBar* bar) : Bar(bar) {}
	~jAppSettingBase() { TwRemoveAllVars(Bar); TwDeleteBar(Bar); }

	template <typename T> void AddVariable(const char* variableName, T& variable) { JASSERT(0); }
	template <> void AddVariable(const char* variableName, bool& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_BOOLCPP, &variable, ""); }
	template <> void AddVariable(const char* variableName, char& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_INT8, &variable, ""); }
	template <> void AddVariable(const char* variableName, int32& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_INT32, &variable, ""); }
	template <> void AddVariable(const char* variableName, uint32& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_UINT32, &variable, ""); }
	template <> void AddVariable(const char* variableName, float& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_FLOAT, &variable, ""); }
	template <> void AddVariable(const char* variableName, double& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_DOUBLE, &variable, ""); }
	template <> void AddVariable(const char* variableName, const char*& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_CDSTRING, &variable, ""); }
	template <> void AddVariable(const char* variableName, std::string& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_STDSTRING, &variable, ""); }

	template <typename T> void AddColorVariable(const char* variableName, T& variable) { JASSERT(0); }
	template <> void AddColorVariable(const char* variableName, Vector& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_COLOR3F, &variable, ""); }
	template <> void AddColorVariable(const char* variableName, Vector4& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_COLOR4F, &variable, ""); }

	template <typename T> void AddDirectionVariable(const char* variableName, T& variable) { JASSERT(0); }
	template <> void AddDirectionVariable(const char* variableName, Vector& variable) { TwAddVarRW(Bar, variableName, TW_TYPE_DIR3F, &variable, ""); }

	template <typename T>
	void AddEnumVariable(const char* titleName, T& variable, const char* enumTypeName, const char** enumVariableString)
	{
		std::vector<TwEnumVal> enumVariable(static_cast<int32>(T::MAX));
		for (int32 i = 0; i < static_cast<int32>(T::MAX); ++i)
		{
			enumVariable[i].Value = i;
			enumVariable[i].Label = enumVariableString[i];
		}

		TwType twType = TwDefineEnum(enumTypeName, enumVariable.data(), static_cast<uint32>(enumVariable.size()));
		TwAddVarRW(Bar, titleName, twType, &variable, nullptr);
	}

	void SetGroup(const char* variableName, const char* groupName)
	{
		TwSetParam(Bar, variableName, "group", TW_PARAM_CSTRING, 1, groupName);
	}

	void SetVisible(const char* variableName, int32 isVisible)
	{
		TwSetParam(Bar, variableName, "visible", TW_PARAM_INT32, 1, &isVisible);
	}

	void SetReadOnly(const char* variableName, int32 isReadOnly)
	{
		TwSetParam(Bar, variableName, "readonly", TW_PARAM_INT32, 1, &isReadOnly);
	}

	void SetLabel(const char* variableName, const char* label)
	{
		TwSetParam(Bar, variableName, "label", TW_PARAM_CSTRING, 1, label);
	}

	void SetHelpText(const char* variableName, const char* helpText)
	{
		TwSetParam(Bar, variableName, "help", TW_PARAM_CSTRING, 1, helpText);
	}

	void RemoveVariable(const char* variableName)
	{
		TwRemoveVar(Bar, variableName);
	}

	void SetStep(const char* variableName, float step)
	{
		TwSetParam(Bar, variableName, "step", TW_PARAM_FLOAT, 1, &step);
	}

	void SetMinMax(const char* variableName, float min, float max)
	{
		TwSetParam(Bar, variableName, "min", TW_PARAM_FLOAT, 1, &min);
		TwSetParam(Bar, variableName, "max", TW_PARAM_FLOAT, 1, &max);
	}

	void SetMinMax(const char* variableName, int32 min, int32 max)
	{
		TwSetParam(Bar, variableName, "min", TW_PARAM_INT32, 1, &min);
		TwSetParam(Bar, variableName, "max", TW_PARAM_INT32, 1, &max);
	}
	void SetSizeBar(int sizeX, int sizeY);
	void SetPositionBar(int ix, int iy);
	void SetValueSizeFit();
	void SetValueSize(int32 valueWidth);

private:
	TwBar* Bar = nullptr;
};

class jAppSettingProperties
{
public:
	virtual ~jAppSettingProperties() {}

	virtual void Setup(jAppSettingBase* appSetting) = 0;
	virtual void Teardown(jAppSettingBase* appSetting) = 0;
};

class jAppSettings
{
public:
	static jAppSettings& GetInstance()
	{
		if (!_instance)
			_instance = new jAppSettings();
		return *_instance;
	}

	void Init(int32 width, int32 height);
	jAppSettingBase* AddTwBar(const char* barName);
	void DeleteTwBar(const char* barName);

	jAppSettingBase* Get(const std::string& name)
	{
		auto it_find = BarMap.find(name);
		return (BarMap.end() != it_find) ? it_find->second : nullptr;
	}

public:
	std::map<std::string, jAppSettingBase*> BarMap;

private:
	jAppSettings() {}

	static jAppSettings* _instance;
};

