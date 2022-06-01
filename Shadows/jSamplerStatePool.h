#pragma once

struct jSamplerState;

class jSamplerStatePool
{
public:
	static std::shared_ptr<jSamplerState> CreateSamplerState(const jName& name, const jSamplerStateInfo& info);
	static std::shared_ptr<jSamplerState> GetSamplerState(const jName& name);

	static void CreateDefaultSamplerState();

private:
	static std::map<jName, std::shared_ptr<jSamplerState> > SamplerStateMap;
	static std::map<jSamplerState*, jName > SamplerStateNameVariableMap;
};

