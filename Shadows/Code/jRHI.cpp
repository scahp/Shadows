#include "pch.h"
#include "jRHI.h"
#include "jShader.h"


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

jMaterialParam* jMaterialData::CreateMaterialParam(const char* name, jTexture* texture, jSamplerState* samplerstate)
{
	auto param = new jMaterialParam();
	param->Name = name;
	param->Texture = texture;
	param->SamplerState = samplerstate;
	return param;
}

void jMaterialData::AddMaterialParam(const char* name, jTexture* texture, jSamplerState* samplerstate)
{
	Params.push_back(CreateMaterialParam(name, texture, samplerstate));
}
