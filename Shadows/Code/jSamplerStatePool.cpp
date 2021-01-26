#include "pch.h"
#include "jSamplerStatePool.h"

std::map<std::string, std::shared_ptr<jSamplerState> > jSamplerStatePool::SamplerStateMap;
std::map<jSamplerState*, std::string > jSamplerStatePool::SamplerStateNameVariableMap;

std::shared_ptr<jSamplerState> jSamplerStatePool::CreateSamplerState(const std::string& name, const jSamplerStateInfo& info)
{
	auto it_find = SamplerStateMap.find(name);
	if (SamplerStateMap.end() != it_find)
		return it_find->second;

	auto samplerState = std::shared_ptr<jSamplerState>(g_rhi->CreateSamplerState(info));
	JASSERT(samplerState.get());
	SamplerStateMap.insert(std::make_pair(name, samplerState));
	SamplerStateNameVariableMap.insert(std::make_pair(samplerState.get(), name));
	return samplerState;
}

std::shared_ptr<jSamplerState> jSamplerStatePool::GetSamplerState(const std::string& name)
{
	auto it_find = SamplerStateMap.find(name);
	return (SamplerStateMap.end() != it_find) ? it_find->second : nullptr;
}

void jSamplerStatePool::CreateDefaultSamplerState()
{
	{
		jSamplerStateInfo info;
		info.Minification = ETextureFilter::LINEAR;
		info.Magnification = ETextureFilter::LINEAR;
		info.AddressU = ETextureAddressMode::REPEAT;
		info.AddressV = ETextureAddressMode::REPEAT;
		info.AddressW = ETextureAddressMode::REPEAT;
		CreateSamplerState("LinearWrap", info);

		info.Minification = ETextureFilter::LINEAR_MIPMAP_LINEAR;
		CreateSamplerState("LinearWrapMipmap", info);
	}
	{
		jSamplerStateInfo info;
		info.Minification = ETextureFilter::LINEAR;
		info.Magnification = ETextureFilter::LINEAR;
		info.AddressU = ETextureAddressMode::CLAMP_TO_EDGE;
		info.AddressV = ETextureAddressMode::CLAMP_TO_EDGE;
		info.AddressW = ETextureAddressMode::CLAMP_TO_EDGE;
		CreateSamplerState("LinearClamp", info);
		
		info.Minification = ETextureFilter::LINEAR_MIPMAP_LINEAR;
		CreateSamplerState("LinearClampMipmap", info);
	}
	{
		jSamplerStateInfo info;
		info.Minification = ETextureFilter::LINEAR;
		info.Magnification = ETextureFilter::LINEAR;
		info.AddressU = ETextureAddressMode::CLAMP_TO_BORDER;
		info.AddressV = ETextureAddressMode::CLAMP_TO_BORDER;
		info.AddressW = ETextureAddressMode::CLAMP_TO_BORDER;
		info.TextureComparisonMode = ETextureComparisonMode::COMPARE_REF_TO_TEXTURE;
		info.ComparisonFunc = EComparisonFunc::LESS;
		CreateSamplerState("LinearClampShadow", info);
	}
	{
		jSamplerStateInfo info;
		info.Minification = ETextureFilter::LINEAR;
		info.Magnification = ETextureFilter::LINEAR;
		info.AddressU = ETextureAddressMode::CLAMP_TO_BORDER;
		info.AddressV = ETextureAddressMode::CLAMP_TO_BORDER;
		info.AddressW = ETextureAddressMode::CLAMP_TO_BORDER;
		CreateSamplerState("LinearBorder", info);

		info.Minification = ETextureFilter::LINEAR_MIPMAP_LINEAR;
		CreateSamplerState("LinearBorderMipmap", info);
	}
	{
		jSamplerStateInfo info;
		info.Minification = ETextureFilter::NEAREST;
		info.Magnification = ETextureFilter::NEAREST;
		info.AddressU = ETextureAddressMode::CLAMP_TO_EDGE;
		info.AddressV = ETextureAddressMode::CLAMP_TO_EDGE;
		info.AddressW = ETextureAddressMode::CLAMP_TO_EDGE;
		CreateSamplerState("Point", info);

		info.Minification = ETextureFilter::NEAREST_MIPMAP_NEAREST;
		CreateSamplerState("PointMipmap", info);
	}
	{
		jSamplerStateInfo info;
		info.Minification = ETextureFilter::NEAREST;
		info.Magnification = ETextureFilter::NEAREST;
		info.AddressU = ETextureAddressMode::REPEAT;
		info.AddressV = ETextureAddressMode::REPEAT;
		info.AddressW = ETextureAddressMode::REPEAT;
		CreateSamplerState("PointWrap", info);

		info.Minification = ETextureFilter::NEAREST_MIPMAP_NEAREST;
		CreateSamplerState("PointWrapMipmap", info);
	}
}
