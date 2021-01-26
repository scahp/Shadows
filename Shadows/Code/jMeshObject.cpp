#include "pch.h"
#include "jMeshObject.h"
#include "jRenderObject.h"
#include "jRHI.h"
#include "jSamplerStatePool.h"
#include "jShader.h"

jMeshMaterial jMeshObject::NullMeshMateral;

const char* jMeshMaterial::MaterialTextureTypeString[(int32)jMeshMaterial::EMaterialTextureType::Max + 1] = {
	"DiffuseSampler",
	"SpecularSampler",
	"AmbientSampler",
	"EmissiveSampler",
	"HeightSampler",
	"NormalSampler",
	"ShininessSampler",
	"OpacitySampler",
	"DisplacementSampler",
	"LightmapSampler",
	"ReflectionSampler",
	"Max"
};

//////////////////////////////////////////////////////////////////////////
// LightData
void jMeshMaterial::Material::BindMaterialData(const jShader* shader) const
{
	shader->SetUniformbuffer("Material.Ambient", Ambient);
	shader->SetUniformbuffer("Material.Diffuse", Diffuse);
	shader->SetUniformbuffer("Material.Specular", Specular);
	shader->SetUniformbuffer("Material.Emissive", Emissive);
	shader->SetUniformbuffer("Material.SpecularShiness", SpecularShiness);
	shader->SetUniformbuffer("Material.Opacity", Opacity);
	shader->SetUniformbuffer("Material.Reflectivity", Reflectivity);
	shader->SetUniformbuffer("Material.IndexOfRefraction", IndexOfRefraction);
}

//////////////////////////////////////////////////////////////////////////
// jMeshObject
jMeshObject::jMeshObject()
{
}

void jMeshObject::Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount /*= 0*/) const
{
	if (Visible && RenderObject)
		DrawNode(RootNode, camera, shader, lights);
}

void jMeshObject::SetMaterialUniform(const jShader* shader, const jMeshMaterial* material) const
{
	g_rhi->SetShader(shader);
	material->Data.BindMaterialData(shader);
}

void jMeshObject::DrawNode(const jMeshNode* node, const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights) const
{
	if (SubMeshes.empty())
		return;

	for (auto& iter : node->MeshIndex)
		DrawSubMesh(iter, camera, shader, lights);

	for (auto& iter : node->childNode)
		DrawNode(iter, camera, shader, lights);
}

void jMeshObject::DrawSubMesh(int32 meshIndex, const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights) const
{
	auto& subMesh = SubMeshes[meshIndex];
	{
		//SCOPE_PROFILE(jMeshObject_DrawSubMesh_SetMaterialUniform);

		if (subMesh.MaterialData.Params.empty())
			subMesh.MaterialData.Params.resize((int32)jMeshMaterial::EMaterialTextureType::Max);

		auto it_find = MeshData->Materials.find(subMesh.MaterialIndex);
		if (MeshData->Materials.end() != it_find)
		{
			const jMeshMaterial* curMeshMaterial = it_find->second;
			JASSERT(curMeshMaterial);

			for (int32 i = (int32)jMeshMaterial::EMaterialTextureType::DiffuseSampler; i < (int32)jMeshMaterial::EMaterialTextureType::Max; ++i)
			{
				const jMeshMaterial::TextureData& TextureData = curMeshMaterial->TexData[i];
				const jTexture* pTexture = TextureData.GetTexture();
				if (!pTexture)
					continue;

				jMaterialData& materialData = subMesh.MaterialData;
				if (!materialData.Params[i])
				{
					const jSamplerState* pSamplerState = jSamplerStatePool::GetSamplerState("LinearWrapMipmap").get();
					materialData.Params[i] = jRenderObject::CreateMaterialParam(jMeshMaterial::MaterialTextureTypeString[i], pTexture, pSamplerState);
				}
			}
			SetMaterialUniform(shader, it_find->second);
		}
		else
		{
			SetMaterialUniform(shader, &NullMeshMateral);
		}

		const bool UseOpacitySampler = (nullptr != subMesh.MaterialData.Params[(int32)jMeshMaterial::EMaterialTextureType::OpacitySampler]);
		shader->SetUniformbuffer("UseOpacitySampler", UseOpacitySampler);

		const bool UseDisplacementSampler = (nullptr != subMesh.MaterialData.Params[(int32)jMeshMaterial::EMaterialTextureType::DisplacementSampler]);
		shader->SetUniformbuffer("UseDisplacementSampler", UseDisplacementSampler);

		const bool UseAmbientSampler = (nullptr != subMesh.MaterialData.Params[(int32)jMeshMaterial::EMaterialTextureType::AmbientSampler]);
		shader->SetUniformbuffer("UseAmbientSampler", UseAmbientSampler);

		const bool UseNormalSampler = (nullptr != subMesh.MaterialData.Params[(int32)jMeshMaterial::EMaterialTextureType::NormalSampler]);
		shader->SetUniformbuffer("UseNormalSampler", UseNormalSampler);
	}

	if (subMesh.EndFace > 0)
		RenderObject->DrawBaseVertexIndex(camera, shader, lights, subMesh.MaterialData, subMesh.StartFace, subMesh.EndFace - subMesh.StartFace, subMesh.StartVertex);
	else
		RenderObject->DrawBaseVertexIndex(camera, shader, lights, subMesh.MaterialData, subMesh.StartVertex, subMesh.EndVertex - subMesh.StartVertex, subMesh.StartVertex);
}
