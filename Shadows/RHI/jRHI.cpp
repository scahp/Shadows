﻿#include "pch.h"
#include "jRHI.h"
#include "Shader/jShader.h"
#include "Scene/jLight.h"

//////////////////////////////////////////////////////////////////////////
void IUniformBuffer::Bind(const jShader* shader) const
{
	SetUniformbuffer(shader);
}

//////////////////////////////////////////////////////////////////////////

jRHI::jRHI()
{
}

void jRHI::MapBufferdata(IBuffer* buffer) const
{

}

void jQueryPrimitiveGenerated::Begin() const
{
	g_rhi->BeginQueryPrimitiveGenerated(this);
}

void jQueryPrimitiveGenerated::End() const
{
	g_rhi->EndQueryPrimitiveGenerated();
}

uint64 jQueryPrimitiveGenerated::GetResult()
{
	g_rhi->GetQueryPrimitiveGeneratedResult(this);
	return NumOfGeneratedPrimitives;
}

void jMaterialData::AddMaterialParam(const jName& name, const jTexture* texture, const jSamplerStateInfo* samplerState)
{
	Params.emplace_back(jMaterialParam(name, texture, samplerState));
}

void jMaterialData::SetMaterialParam(int32 index, const jName& name, const jTexture* texture, const jSamplerStateInfo* samplerState /*= nullptr*/)
{
	if (Params.size() <= index)
	{
		Params.resize(index + 1);
		OutputDebugStringA("Resized Params in SetMaterialParam\n");
	}

	Params[index].Name = name;
    Params[index].Texture = texture;
    Params[index].SamplerState = samplerState;
}

void jMaterialData::SetMaterialParam(int32 index, const jName& name)
{
    if (Params.size() <= index)
        return;

    Params[index].Name = name;
}

void jMaterialData::SetMaterialParam(int32 index, const jTexture* texture)
{
    if (Params.size() <= index)
        return;

    Params[index].Texture = texture;
}

void jMaterialData::SetMaterialParam(int32 index, const jSamplerStateInfo* samplerState)
{
    if (Params.size() <= index)
        return;

    Params[index].SamplerState = samplerState;
}

jName GetCommonTextureName(int32 index)
{
	static std::vector<jName> s_tex_name;

	if ((s_tex_name.size() > index) && s_tex_name[index].IsValid())
	{
		return s_tex_name[index];
	}

	if (s_tex_name.size() < (index + 1))
		s_tex_name.resize(index + 1);

	char szTemp[128] = { 0, };
	if (index > 0)
		sprintf_s(szTemp, sizeof(szTemp), "tex_object%d", index + 1);
	else
		sprintf_s(szTemp, sizeof(szTemp), "tex_object");

	s_tex_name[index] = jName(szTemp);
	return s_tex_name[index];
}

jName GetCommonTextureSRGBName(int32 index)
{
	static std::vector<jName> s_tex_srgb_name;

	if ((s_tex_srgb_name.size() > index) && s_tex_srgb_name[index].IsValid())
	{
		return s_tex_srgb_name[index];
	}

	if (s_tex_srgb_name.size() < (index + 1))
		s_tex_srgb_name.resize(index + 1);

	char szTemp[128] = { 0, };
	sprintf_s(szTemp, sizeof(szTemp), "TextureSRGB[%d]", index);

	s_tex_srgb_name[index] = jName(szTemp);
	return s_tex_srgb_name[index];
}

size_t jRenderPass::GetHash() const
{
	if (Hash)
		return Hash;

	Hash = STATIC_NAME_CITY_HASH("ColorAttachments");
	for (int32 i = 0; i < ColorAttachments.size(); ++i)
	{
		Hash ^= ColorAttachments[i]->GetHash();
	}
	Hash ^= STATIC_NAME_CITY_HASH("DepthAttachment");
	if (DepthAttachment)
	{
		Hash ^= DepthAttachment->GetHash();
	}
	Hash ^= STATIC_NAME_CITY_HASH("ColorAttachmentResolve");
	if (ColorAttachmentResolve)
	{
		Hash ^= ColorAttachmentResolve->GetHash();
	}
	Hash ^= CityHash64((const char*)&RenderOffset, sizeof(RenderOffset));
	Hash ^= CityHash64((const char*)&RenderExtent, sizeof(RenderExtent));
	return Hash;
}

size_t jShaderBindings::GenerateHash(const std::vector<jShaderBinding>& InUniformBuffers, const std::vector<jShaderBinding>& InTextures)
{
	size_t result = 0;

	result ^= STATIC_NAME_CITY_HASH("UniformBuffer");
	result ^= CityHash64((const char*)InUniformBuffers.data(), sizeof(jShaderBinding) * InUniformBuffers.size());

	result ^= STATIC_NAME_CITY_HASH("Texture");
	result ^= CityHash64((const char*)InTextures.data(), sizeof(jShaderBinding) * InTextures.size());

	return result;
}

size_t jShaderBindings::GetHash() const
{
	if (Hash)
		return Hash;
	
	Hash = GenerateHash(UniformBuffers, Textures);
	return Hash;
}


void jView::GetShaderBindingInstance(std::vector<const jShaderBindingInstance*>& OutShaderBindingInstance)
{
	if (DirectionalLight)
	{
		OutShaderBindingInstance.push_back(DirectionalLight->ShaderBindingInstance);
	}
}