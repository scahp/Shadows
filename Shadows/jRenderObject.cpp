#include "pch.h"
#include "jRenderObject.h"
#include "jGame.h"
#include "jRHI.h"
#include "jCamera.h"
#include "jLight.h"
#include "jRHI_OpenGL.h"
#include "jRenderTargetPool.h"
#include "jShadowAppProperties.h"
#include "jObject.h"


jRenderObject::jRenderObject()
{
}


jRenderObject::~jRenderObject()
{
}

void jRenderObject::CreateRenderObject(const std::shared_ptr<jVertexStreamData>& vertexStream, const std::shared_ptr<jIndexStreamData>& indexStream)
{
	VertexStream = vertexStream;
	IndexStream = indexStream;
	VertexBuffer = g_rhi->CreateVertexBuffer(VertexStream);
	IndexBuffer = g_rhi->CreateIndexBuffer(IndexStream);
}

void jRenderObject::UpdateVertexStream(const std::shared_ptr<jVertexStreamData>& vertexStream)
{
	VertexStream = vertexStream;
	g_rhi->UpdateVertexBuffer(VertexBuffer, VertexStream);
}

void jRenderObject::UpdateVertexStream()
{
	g_rhi->UpdateVertexBuffer(VertexBuffer, VertexStream);
}

//void jRenderObject::Draw(const jCamera* camera, const jShader* shader, int32 startIndex, int32 count)
//{
//	if (VertexBuffer->VertexStreamData.expired())
//		return;
//
//	g_rhi->SetShader(shader);
//	g_rhi->EnableCullFace(camera->IsEnableCullMode && !IsTwoSided);
//
//	jMaterialData materialData;
//
//	SetRenderProperty(shader);
//	SetCameraProperty(shader, camera);
//	SetLightProperty(shader, camera, &materialData);
//	SetTextureProperty(shader, &materialData);
//	SetMaterialProperty(shader, &materialData);
//
//	startIndex = startIndex != -1 ? startIndex : 0;
//
//	auto vertexStreamData = VertexBuffer->VertexStreamData.lock();
//	auto primitiveType = vertexStreamData->PrimitiveType;
//	if (IndexBuffer)
//	{
//		auto indexStreamData = IndexBuffer->IndexStreamData.lock();		
//		count = count != -1 ? count : indexStreamData->ElementCount;
//		g_rhi->DrawElement(primitiveType, static_cast<int32>(indexStreamData->Param->GetElementSize()), startIndex, count);
//	}
//	else
//	{
//		count = count != -1 ? count : vertexStreamData->ElementCount;
//		g_rhi->DrawArray(primitiveType, 0, count);
//	}
//}

void jRenderObject::Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 startIndex, int32 count, int32 instanceCount)
{
	if (VertexBuffer->VertexStreamData.expired())
		return;

	g_rhi->SetShader(shader);
	if (camera)
		g_rhi->EnableCullFace(camera->IsEnableCullMode && !IsTwoSided);
	else
		g_rhi->EnableCullFace(!IsTwoSided);

	DynamicMaterialData.clear();

	SetRenderProperty(shader);
	SetCameraProperty(shader, camera);
	for (auto iter : lights)
	{
		auto matData = iter->GetMaterialData();
		if (matData)
			DynamicMaterialData.push_back(matData);
	}
	SetTextureProperty(shader, &MaterialData);
	SetMaterialProperty(shader, &MaterialData, DynamicMaterialData);
	
	startIndex = startIndex != -1 ? startIndex : 0;

	auto vertexStreamData = VertexBuffer->VertexStreamData.lock();
	auto primitiveType = vertexStreamData->PrimitiveType;
	if (IndexBuffer)
	{
		auto indexStreamData = IndexBuffer->IndexStreamData.lock();
		count = count != -1 ? count : indexStreamData->ElementCount;
		if (instanceCount <= 0)
			g_rhi->DrawElements(primitiveType, static_cast<int32>(indexStreamData->Param->GetElementSize()), startIndex, count);
		else
			g_rhi->DrawElementsInstanced(primitiveType, static_cast<int32>(indexStreamData->Param->GetElementSize()), startIndex, count, instanceCount);
	}
	else
	{
		count = count != -1 ? count : vertexStreamData->ElementCount;
		if (instanceCount <= 0)
			g_rhi->DrawArrays(primitiveType, 0, count);
		else
			g_rhi->DrawArraysInstanced(primitiveType, 0, count, instanceCount);
	}
}

void jRenderObject::DrawBaseVertexIndex(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, const jMaterialData& materialData, int32 startIndex, int32 count, int32 baseVertexIndex, int32 instanceCount /*= 1*/)
{
	if (VertexBuffer->VertexStreamData.expired())
		return;

	g_rhi->SetShader(shader);
	g_rhi->EnableCullFace(camera->IsEnableCullMode && !IsTwoSided);

	{
		//SCOPE_PROFILE(jRenderObject_UpdateMaterialData);
		std::vector<const jMaterialData*> DynamicMaterialData;
		DynamicMaterialData.reserve(lights.size());

		SetRenderProperty(shader);
		SetCameraProperty(shader, camera);
		//SetLightProperty(shader, camera, lights, &DynamicMaterialData);
		for (auto iter : lights)
		{
			auto matData = iter->GetMaterialData();
			if (matData)
				DynamicMaterialData.push_back(matData);
		}
		SetTextureProperty(shader, &materialData);
		SetMaterialProperty(shader, &materialData, DynamicMaterialData);
	}

	{
		//SCOPE_PROFILE(jRenderObject_DrawBaseVertexIndex);
		auto vertexStreamData = VertexBuffer->VertexStreamData.lock();
		auto primitiveType = vertexStreamData->PrimitiveType;
		if (IndexBuffer)
		{
			auto indexStreamData = IndexBuffer->IndexStreamData.lock();
			if (instanceCount <= 0)
				g_rhi->DrawElementsBaseVertex(primitiveType, static_cast<int32>(indexStreamData->Param->GetElementSize()), startIndex, count, baseVertexIndex);
			else
				g_rhi->DrawElementsInstancedBaseVertex(primitiveType, static_cast<int32>(indexStreamData->Param->GetElementSize()), startIndex, count, baseVertexIndex, instanceCount);
		}
		else
		{
			if (instanceCount <= 0)
				g_rhi->DrawArrays(primitiveType, baseVertexIndex, count);
			else
				g_rhi->DrawArraysInstanced(primitiveType, baseVertexIndex, count, instanceCount);
		}
	}
}

//void jRenderObject::Draw(const jCamera* camera, const jShader* shader, int32 startIndex, int32 count, int32 baseVertexIndex)
//{
//	if (VertexBuffer->VertexStreamData.expired())
//		return;
//
//	g_rhi->SetShader(shader);
//	g_rhi->EnableCullFace(camera->IsEnableCullMode && !IsTwoSided);
//
//	jMaterialData materialData;
//
//	SetRenderProperty(shader);
//	SetCameraProperty(shader, camera);
//	SetLightProperty(shader, camera, &materialData);
//	SetTextureProperty(shader, &materialData);
//	SetMaterialProperty(shader, &materialData);
//
//	startIndex = startIndex != -1 ? startIndex : 0;
//
//	auto vertexStreamData = VertexBuffer->VertexStreamData.lock();
//	auto primitiveType = vertexStreamData->PrimitiveType;
//	if (IndexBuffer)
//	{
//		auto indexStreamData = IndexBuffer->IndexStreamData.lock();
//		count = count != -1 ? count : indexStreamData->ElementCount;
//		g_rhi->DrawElementBaseVertex(primitiveType, static_cast<int32>(indexStreamData->Param->GetElementSize()), startIndex, count, baseVertexIndex);
//	}
//	else
//	{
//		count = count != -1 ? count : vertexStreamData->ElementCount;
//		g_rhi->DrawArray(primitiveType, baseVertexIndex, count);
//	}
//}

void jRenderObject::SetRenderProperty(const jShader* shader)
{
	if (VertexBuffer)
		VertexBuffer->Bind(shader);
	if (IndexBuffer)
		IndexBuffer->Bind(shader);
}

void jRenderObject::UpdateWorldMatrix()
{
    if (static_cast<int32>(DirtyFlags) & static_cast<int32>(EDirty::POS_ROT_SCALE))
    {
        auto posMatrix = Matrix::MakeTranslate(Pos);
        auto rotMatrix = Matrix::MakeRotate(Rot);
        auto scaleMatrix = Matrix::MakeScale(Scale);
        World = posMatrix * rotMatrix * scaleMatrix;

        ClearDirtyFlags(EDirty::POS_ROT_SCALE);
    }
}

Matrix jRenderObject::GetWorld() const
{
	return World;
}

void jRenderObject::SetCameraProperty(const jShader* shader, const jCamera* camera)
{
	if (!camera)
		return;

	if (static_cast<int32>(DirtyFlags) & static_cast<int32>(EDirty::POS_ROT_SCALE))
	{
		auto posMatrix = Matrix::MakeTranslate(Pos);
		auto rotMatrix = Matrix::MakeRotate(Rot);
		auto scaleMatrix = Matrix::MakeScale(Scale);
		World = posMatrix * rotMatrix * scaleMatrix;

		ClearDirtyFlags(EDirty::POS_ROT_SCALE);
	}

	auto MV = camera->View * World;
	auto MVP = camera->Projection * MV;

	SET_UNIFORM_BUFFER_STATIC("MVP", MVP, shader);
	SET_UNIFORM_BUFFER_STATIC("MV", MV, shader);
	SET_UNIFORM_BUFFER_STATIC("M", World, shader);
	SET_UNIFORM_BUFFER_STATIC("InvM", World.GetInverse(), shader);
	SET_UNIFORM_BUFFER_STATIC("Collided", Collided, shader);
	SET_UNIFORM_BUFFER_STATIC("UseUniformColor", UseUniformColor, shader);
	SET_UNIFORM_BUFFER_STATIC("Color", Color, shader);
	SET_UNIFORM_BUFFER_STATIC("UseMaterial", UseMaterial, shader);
	SET_UNIFORM_BUFFER_STATIC("ShadingModel", static_cast<int>(ShadingModel), shader);
	SET_UNIFORM_BUFFER_STATIC("IsTwoSided", IsTwoSided, shader);
}

void jRenderObject::SetLightProperty(const jShader* shader, const jCamera* camera, const std::list<const jLight*>& lights, jMaterialData* materialData)
{
	for (auto iter : lights)
		iter->GetMaterialData(materialData);
}

void jRenderObject::SetTextureProperty(const jShader* shader, const jMaterialData* materialData)
{
	const bool useTexture = (materialData->Params.size() > 0);
	SET_UNIFORM_BUFFER_STATIC("UseTexture", useTexture, shader);
}

void jRenderObject::SetMaterialProperty(const jShader* shader, const jMaterialData* materialData, const std::vector<const jMaterialData*>& dynamicMaterialData)
{
	int32 lastIndex = 0;
	if (materialData)
		lastIndex = g_rhi->SetMatetrial(materialData, shader, lastIndex);
	
	for (auto MatData : dynamicMaterialData)
	{
		lastIndex = g_rhi->SetMatetrial(MatData, shader, lastIndex);
	}
}

const std::vector<float>& jRenderObject::GetVertices() const
{
	if (VertexStream && !VertexStream->Params.empty())
		return static_cast<jStreamParam<float>*>(VertexStream->Params[0])->Data;

	static const std::vector<float> s_emtpy;
	return s_emtpy;
}

void jRenderObject::SetTexture(int32 index, const jName& name, const jTexture* texture, const jSamplerState* samplerState)
{
	MaterialData.SetMaterialParam(index, name, texture, samplerState);
}

void jRenderObject::SetTextureWithCommonName(int32 index, const jTexture* texture, const jSamplerState* samplerState)
{
	MaterialData.SetMaterialParam(index, GetCommonTextureName(index), texture, samplerState);
}

void jRenderObject::ClearTexture()
{
	MaterialData.Clear();
}
