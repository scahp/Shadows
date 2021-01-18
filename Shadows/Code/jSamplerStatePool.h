#pragma once

struct jSamplerState;

class jSamplerStatePool
{
public:
	static std::shared_ptr<jSamplerState> CreateSamplerState(const std::string& name, const jSamplerStateInfo& info);
	static std::shared_ptr<jSamplerState> GetSamplerState(const std::string& name);

	static void CreateDefaultSamplerState();

private:
	static std::map<std::string, std::shared_ptr<jSamplerState> > SamplerStateMap;
	static std::map<jSamplerState*, std::string > SamplerStateNameVariableMap;
};

