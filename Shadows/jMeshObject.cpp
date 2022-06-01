#include "pch.h"
#include "jMeshObject.h"
#include "jRenderObject.h"
#include "jRHI.h"
#include "jSamplerStatePool.h"
#include "jShader.h"

jMeshMaterial jMeshObject::NullMeshMateral;

jName jMeshMaterial::MaterialTextureTypeString[(int32)jMeshMaterial::EMaterialTextureType::Max + 1] = {
	jName("DiffuseSampler"),
	jName("SpecularSampler"),
	jName("AmbientSampler"),
	jName("EmissiveSampler"),
	jName("HeightSampler"),
	jName("NormalSampler"),
	jName("ShininessSampler"),
	jName("OpacitySampler"),
	jName("DisplacementSampler"),
	jName("LightmapSampler"),
	jName("ReflectionSampler"),
	jName("Max")
};

//////////////////////////////////////////////////////////////////////////
// LightData
void jMeshMaterial::Material::BindMaterialData(const jShader* shader) const
{
	SET_UNIFORM_BUFFER_STATIC("Material.Ambient", Ambient, shader);
	SET_UNIFORM_BUFFER_STATIC("Material.Diffuse", Diffuse, shader);
	SET_UNIFORM_BUFFER_STATIC("Material.Specular", Specular, shader);
	SET_UNIFORM_BUFFER_STATIC("Material.Emissive", Emissive, shader);
	SET_UNIFORM_BUFFER_STATIC("Material.SpecularShiness", SpecularShiness, shader);
	SET_UNIFORM_BUFFER_STATIC("Material.Opacity", Opacity, shader);
	SET_UNIFORM_BUFFER_STATIC("Material.Reflectivity", Reflectivity, shader);
	SET_UNIFORM_BUFFER_STATIC("Material.IndexOfRefraction", IndexOfRefraction, shader);
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

		//if (subMesh.MaterialData.Params.empty())
		//	subMesh.MaterialData.Params.resize((int32)jMeshMaterial::EMaterialTextureType::Max);

		bool UseOpacitySampler = false;
		bool UseDisplacementSampler = false;
		bool UseAmbientSampler = false;
		bool UseNormalSampler = false;

		subMesh.MaterialData.Params.clear();

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

				const jSamplerState* pSamplerState = jSamplerStatePool::GetSamplerState(jName("LinearWrapMipmap")).get();
				materialData.AddMaterialParam(jMeshMaterial::MaterialTextureTypeString[i], pTexture, pSamplerState);

				switch ((jMeshMaterial::EMaterialTextureType)i)
				{
				case jMeshMaterial::EMaterialTextureType::OpacitySampler:
					UseOpacitySampler = true;
					break;
				case jMeshMaterial::EMaterialTextureType::DisplacementSampler:
					UseDisplacementSampler = true;
					break;
				case jMeshMaterial::EMaterialTextureType::AmbientSampler:
					UseAmbientSampler = true;
					break;
				case jMeshMaterial::EMaterialTextureType::NormalSampler:
					UseNormalSampler = true;
					break;
				}
			}
			SetMaterialUniform(shader, it_find->second);
		}
		else
		{
			SetMaterialUniform(shader, &NullMeshMateral);
		}
				
		SET_UNIFORM_BUFFER_STATIC("UseOpacitySampler", UseOpacitySampler, shader);
		SET_UNIFORM_BUFFER_STATIC("UseDisplacementSampler", UseDisplacementSampler, shader);
		SET_UNIFORM_BUFFER_STATIC("UseAmbientSampler", UseAmbientSampler, shader);
		SET_UNIFORM_BUFFER_STATIC("UseNormalSampler", UseNormalSampler, shader);
	}

	if (subMesh.EndFace > 0)
		RenderObject->DrawBaseVertexIndex(camera, shader, lights, subMesh.MaterialData, subMesh.StartFace, subMesh.EndFace - subMesh.StartFace, subMesh.StartVertex);
	else
		RenderObject->DrawBaseVertexIndex(camera, shader, lights, subMesh.MaterialData, subMesh.StartVertex, subMesh.EndVertex - subMesh.StartVertex, subMesh.StartVertex);
}

