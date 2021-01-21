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
	if (camera)
		g_rhi->EnableCullFace(camera->IsEnableCullMode && !IsTwoSided);
	else
		g_rhi->EnableCullFace(!IsTwoSided);

	std::vector<const jMaterialData*> DynamicMaterialData;

	SetRenderProperty(shader);
	SetCameraProperty(shader, camera);
	//SetLightProperty(shader, camera, lights, &DynamicMaterialData);
	for (auto iter : lights)
		DynamicMaterialData.push_back(iter->GetMaterialData());
	SetTextureProperty(shader, nullptr);
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
			DynamicMaterialData.push_back(iter->GetMaterialData());
		SetTextureProperty(shader, nullptr);
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

	g_rhi->SetUniformbuffer("MVP", MVP, shader);
	g_rhi->SetUniformbuffer("MV", MV, shader);
	g_rhi->SetUniformbuffer("M", World, shader);
	g_rhi->SetUniformbuffer("Collided", Collided, shader);
	g_rhi->SetUniformbuffer("UseUniformColor", UseUniformColor, shader);
	g_rhi->SetUniformbuffer("Color", Color, shader);
	g_rhi->SetUniformbuffer("UseMaterial", UseMaterial, shader);
	g_rhi->SetUniformbuffer("ShadingModel", static_cast<int>(ShadingModel), shader);
	g_rhi->SetUniformbuffer("IsTwoSided", IsTwoSided, shader);
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
	//if (materialData)
	{
		//if (tex_object)
		//{
		//	auto tex_object_param = new jMaterialParam();
		//	tex_object_param->Name = "tex_object";
		//	tex_object_param->Texture = tex_object;
		//	tex_object_param->SamplerState = samplerState;
		//	materialData->Params.push_back(tex_object_param);
		//}

		//bool useTexture = false;
		//if (tex_object2)
		//{
		//	auto tex_object2_param = new jMaterialParam();
		//	tex_object2_param->Name = "tex_object2";
		//	tex_object2_param->Texture = tex_object2;
		//	tex_object2_param->SamplerState = samplerState2;
		//	materialData->Params.push_back(tex_object2_param);
		//	useTexture = true;
		//}
		//if (tex_object3)
		//{
		//	auto tex_object3_param = new jMaterialParam();
		//	tex_object3_param->Name = "tex_object3";
		//	tex_object3_param->Texture = tex_object3;
		//	tex_object3_param->SamplerState = samplerState3;
		//	materialData->Params.push_back(tex_object3_param);
		//	useTexture = true;
		//}

		// todo remove this.
		bool useTexture = true;
		g_rhi->SetUniformbuffer("UseTexture", useTexture, shader);
		//for (jMaterialParam* pParam : MaterialData.Params)
		//{
		//	if (pParam->Name == "tex_object2")
		//	{
		//		useTexture = true;
		//		break;
		//	}
		//}
		//g_rhi->SetUniformbuffer("UseTexture", useTexture, shader);
		//static jUniformBuffer<int> temp("UseTexture", useTexture);
		//g_rhi->SetUniformbuffer(&temp, shader);

		//if (tex_object_array)
		//{
		//	auto tex_objectArray_param = new jMaterialParam();
		//	tex_objectArray_param->Name = "tex_object_array";
		//	tex_objectArray_param->Texture = tex_object_array;
		//	tex_objectArray_param->SamplerState = samplerStateTexArray;
		//	materialData->Params.push_back(tex_objectArray_param);
		//}
	}
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
