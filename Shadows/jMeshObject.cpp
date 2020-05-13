#include "pch.h"
#include "jMeshObject.h"
#include "jRenderObject.h"
#include "jRHI.h"

jMeshMaterial jMeshObject::NullMeshMateral;

jMeshObject::jMeshObject()
{	
}

void jMeshObject::Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 instanceCount /*= 0*/)
{
	if (Visible && RenderObject)
		DrawNode(RootNode, camera, shader, lights);
}

void jMeshObject::SetMaterialUniform(const jShader* shader, const jMeshMaterial* material)
{
	g_rhi->SetShader(shader);
	SET_UNIFORM_BUFFER_STATIC(Vector4, "Material.Diffuse", material->Data.Diffuse, shader);

	// todo vec4인데 일단 vec3으로 넘김.. 고민임.
	SET_UNIFORM_BUFFER_STATIC(Vector4, "Material.Specular", material->Data.Specular, shader);
	
	SET_UNIFORM_BUFFER_STATIC(Vector, "Material.Emissive", material->Data.Emissive, shader);
	SET_UNIFORM_BUFFER_STATIC(float, "Material.Shininess", material->Data.SpecularPow, shader);
}

void jMeshObject::DrawNode(const jMeshNode* node, const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights)
{
	for (auto& iter : node->MeshIndex)
		DrawSubMesh(iter, camera, shader, lights);

	for (auto& iter : node->childNode)
		DrawNode(iter, camera, shader, lights);
}

void jMeshObject::DrawSubMesh(int32 meshIndex, const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights)
{
	auto& subMesh = SubMeshes[meshIndex];
	if (SetMaterialOverride)
	{
		SetMaterialOverride(this);
	}
	else
	{
		auto it_find = MeshData->Materials.find(subMesh.MaterialIndex);
		if (MeshData->Materials.end() != it_find)
		{
			RenderObject->tex_object[1] = it_find->second->Texture;
			SetMaterialUniform(shader, it_find->second);
		}
		else
		{
			RenderObject->tex_object[1] = nullptr;
			SetMaterialUniform(shader, &NullMeshMateral);
		}
	}

	if (subMesh.EndFace > 0)
		RenderObject->DrawBaseVertexIndex(camera, shader, lights, subMesh.StartFace, subMesh.EndFace - subMesh.StartFace, subMesh.StartVertex);
	else
		RenderObject->DrawBaseVertexIndex(camera, shader, lights, subMesh.StartVertex, subMesh.EndVertex - subMesh.StartVertex, subMesh.StartVertex);
}

