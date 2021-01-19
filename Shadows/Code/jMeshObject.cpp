#include "pch.h"
#include "jMeshObject.h"
#include "jRenderObject.h"
#include "jRHI.h"
#include "jSamplerStatePool.h"
#include "jShader.h"

jMeshMaterial jMeshObject::NullMeshMateral;

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

		auto it_find = MeshData->Materials.find(subMesh.MaterialIndex);
		if (MeshData->Materials.end() != it_find)
		{
			const jMeshMaterial* curMeshMaterial = it_find->second;
			JASSERT(curMeshMaterial);

			const jMeshMaterial::TextureData& DiffuseTextureData = curMeshMaterial->TexData[(int32)jMeshMaterial::EMaterialTextureType::Diffuse];
			const jTexture* pDiffuseTexture = DiffuseTextureData.TextureWeakPtr.lock().get();
			if (pDiffuseTexture)
			{
				if (RenderObject->MaterialData.Params.empty())
				{
					auto param = jRenderObject::CreateMaterialParam("tex_object2", pDiffuseTexture, jSamplerStatePool::GetSamplerState("LinearWrapMipmap").get());
					RenderObject->MaterialData.Params.push_back(param);
				}
				else
				{
					RenderObject->MaterialData.Params[0]->Texture = pDiffuseTexture;
				}
			}
			//RenderObject->tex_object2 = it_find->second->Texture;
			//RenderObject->samplerState2 = jSamplerStatePool::GetSamplerState("LinearWrapMipmap").get();
			SetMaterialUniform(shader, it_find->second);
		}
		else
		{
			//RenderObject->tex_object2 = nullptr;
			SetMaterialUniform(shader, &NullMeshMateral);
		}
	}

	if (subMesh.EndFace > 0)
		RenderObject->DrawBaseVertexIndex(camera, shader, lights, subMesh.StartFace, subMesh.EndFace - subMesh.StartFace, subMesh.StartVertex);
	else
		RenderObject->DrawBaseVertexIndex(camera, shader, lights, subMesh.StartVertex, subMesh.EndVertex - subMesh.StartVertex, subMesh.StartVertex);
}
