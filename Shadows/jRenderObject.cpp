#include "pch.h"
#include "jRenderObject.h"
#include "jGame.h"
#include "jRHI.h"
#include "jCamera.h"
#include "jLight.h"
#include "jRHI_OpenGL.h"
#include "jRenderTargetPool.h"
#include "jShadowAppProperties.h"


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
	g_rhi->EnableCullFace(camera->IsEnableCullMode && !IsTwoSided);

	jMaterialData materialData;

	SetRenderProperty(shader);
	SetCameraProperty(shader, camera);
	SetLightProperty(shader, camera, lights, &materialData);
	SetTextureProperty(shader, &materialData);
	SetMaterialProperty(shader, &materialData);
	
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

void jRenderObject::DrawBaseVertexIndex(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights, int32 startIndex, int32 count, int32 baseVertexIndex, int32 instanceCount)
{
	if (VertexBuffer->VertexStreamData.expired())
		return;

	g_rhi->SetShader(shader);
	g_rhi->EnableCullFace(camera->IsEnableCullMode && !IsTwoSided);

	jMaterialData materialData;

	SetRenderProperty(shader);
	SetCameraProperty(shader, camera);
	SetLightProperty(shader, camera, lights, &materialData);
	SetTextureProperty(shader, &materialData);
	SetMaterialProperty(shader, &materialData);

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

void jRenderObject::SetCameraProperty(const jShader* shader, const jCamera* camera)
{
	auto posMatrix = Matrix::MakeTranslate(Pos);
	auto rotMatrix = Matrix::MakeRotate(Rot);
	auto scaleMatrix = Matrix::MakeScale(Scale);

	World = posMatrix * rotMatrix * scaleMatrix;
	auto MV = camera->View * World;
	auto MVP = camera->Projection * MV;

	SET_UNIFORM_BUFFER_STATIC(Matrix, "MVP", MVP, shader);
	SET_UNIFORM_BUFFER_STATIC(Matrix, "MV", MV, shader);
	SET_UNIFORM_BUFFER_STATIC(Matrix, "M", World, shader);
	SET_UNIFORM_BUFFER_STATIC(int, "Collided", Collided, shader);
	SET_UNIFORM_BUFFER_STATIC(int, "UseUniformColor", UseUniformColor, shader);
	SET_UNIFORM_BUFFER_STATIC(Vector4, "Color", Color, shader);
	SET_UNIFORM_BUFFER_STATIC(int, "UseMaterial", UseMaterial, shader);
	SET_UNIFORM_BUFFER_STATIC(int, "ShadingModel", static_cast<int>(ShadingModel), shader);
	SET_UNIFORM_BUFFER_STATIC(int, "IsTwoSided", IsTwoSided, shader);
}

void jRenderObject::SetLightProperty(const jShader* shader, const jCamera* camera, const std::list<const jLight*>& lights, jMaterialData* materialData)
{
	for (auto iter : lights)
		iter->GetMaterialData(materialData);
}

//void jRenderObject::SetLightProperty(const jShader* shader, const jLight* light, jMaterialData* materialData)
//{
//	int ambient = 0;
//	int directional = 0;
//	int point = 0;
//	int spot = 0;
//	if (light)
//	{
//		switch (light->Type)
//		{
//		case ELightType::AMBIENT:
//			light->BindLight(shader, materialData);
//			ambient = 1;
//			break;
//		case ELightType::DIRECTIONAL:
//			light->BindLight(shader, materialData, directional);
//			++directional;
//			break;
//		case ELightType::POINT:
//			light->BindLight(shader, materialData, point);
//			++point;
//			break;
//		case ELightType::SPOT:
//			light->BindLight(shader, materialData, spot);
//			++spot;
//			break;
//		default:
//			break;
//		}
//	}
//
//	g_rhi->SetUniformbuffer(&jUniformBuffer<int>("UseAmbientLight", ambient), shader);
//	g_rhi->SetUniformbuffer(&jUniformBuffer<int>("NumOfDirectionalLight", directional), shader);
//	g_rhi->SetUniformbuffer(&jUniformBuffer<int>("NumOfPointLight", point), shader);
//	g_rhi->SetUniformbuffer(&jUniformBuffer<int>("NumOfSpotLight", spot), shader);
//}

void jRenderObject::SetTextureProperty(const jShader* shader, jMaterialData* materialData)
{
	if (materialData)
	{
		bool useTexture = false;

		char szTemp[128] = {};
		for (int32 i = 0; i < MAX_TEX; ++i)
		{
			auto tex_object_param = new jMaterialParam();
			if (i == 0)
			{
				tex_object_param->Name = "tex_object";
			}
			else if (i > 0)
			{
				sprintf_s(szTemp, sizeof(szTemp), "tex_object%d", i + 1);
				tex_object_param->Name = szTemp;
			}
			tex_object_param->Texture = tex_object[i];
			tex_object_param->SamplerState = samplerState[i];
			materialData->Params.push_back(tex_object_param);
			useTexture = true;
		}
		SET_UNIFORM_BUFFER_STATIC(int, "UseTexture", useTexture, shader);

		if (tex_object_array)
		{
			auto tex_objectArray_param = new jMaterialParam();
			tex_objectArray_param->Name = "tex_object_array";
			tex_objectArray_param->Texture = tex_object_array;
			tex_objectArray_param->SamplerState = samplerStateTexArray;
			materialData->Params.push_back(tex_objectArray_param);
		}
	}
}

void jRenderObject::SetMaterialProperty(const jShader* shader, jMaterialData* materialData)
{
	g_rhi->SetMatetrial(materialData, shader);
}
