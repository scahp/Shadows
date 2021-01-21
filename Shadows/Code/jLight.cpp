#include "pch.h"
#include "jLight.h"
#include "jRHI.h"
#include "Math\Vector.h"
#include "jRenderTargetPool.h"
#include "jCamera.h"
#include "jRHI_OpenGL.h"
#include "jObject.h"
#include "jSamplerStatePool.h"

namespace jLightUtil
{
	constexpr float width = 400.0f;
	constexpr float height = 400.0f;
	constexpr float nearDist = 1.0f;
	constexpr float farDist = 1000.0f;

	void MakeDirectionalLightViewInfo(Vector& outPos, Vector& outTarget, Vector& outUp, const Vector& direction)
	{
		outPos = Vector(-farDist * 0.5f) * direction;
		outTarget = Vector::ZeroVector;
		outUp = outPos + Vector(0.0f, 1.0f, 0.0f);
	}

	jShadowMapData* CreateShadowMap(const Vector& direction, const Vector&)
	{
		Vector pos, target, up;
		pos = Vector(350.0f, 360.0f, 100.0f);
		//pos = Vector(500.0f, 500.0f, 500.0f);
		//MakeDirectionalLightViewInfoWithPos(target, up, pos, direction);
		MakeDirectionalLightViewInfo(pos, target, up, direction);

		// todo remove constant variable
		auto shadowMapData = new jShadowMapData("DirectionalLight");
		shadowMapData->ShadowMapCamera = jOrthographicCamera::CreateCamera(pos, target, up, -width / 2.0f, -height / 2.0f, width / 2.0f, height / 2.0f, farDist, nearDist);
		
		shadowMapData->ShadowMapRenderTarget = jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::RG32F, ETextureFormat::RG, EFormatType::FLOAT, EDepthBufferType::DEPTH32, SM_WIDTH, SM_HEIGHT, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR });
		shadowMapData->ShadowMapSamplerState = jSamplerStatePool::GetSamplerState("LinearClampShadow");

		return shadowMapData;
	}

	jShadowMapData* CreateCascadeShadowMap(const Vector& direction, const Vector&)
	{
		Vector pos, target, up;
		pos = Vector(350.0f, 360.0f, 100.0f);
		//pos = Vector(500.0f, 500.0f, 500.0f);
		//MakeDirectionalLightViewInfoWithPos(target, up, pos, direction);
		MakeDirectionalLightViewInfo(pos, target, up, direction);

		// todo remove constant variable
		auto shadowMapData = new jShadowMapData("DirectionalLight");
		//shadowMapData->ShadowMapCamera = jCamera::CreateCamera(tempPos, target, up, 3.14f / 4.0f, 300.0f, 900.0f, SM_WIDTH, SM_HEIGHT, true);		// todo for deep shadow map. it should be replaced
		//shadowMapData->ShadowMapCamera = jCamera::CreateCamera(tempPos, target, up, DegreeToRadian(90.0f), 10.0f, 900.0f, SM_WIDTH, SM_HEIGHT, false);
		shadowMapData->ShadowMapCamera = jOrthographicCamera::CreateCamera(pos, target, up, -width / 2.0f, -height / 2.0f, width / 2.0f, height / 2.0f, farDist, nearDist);

		shadowMapData->ShadowMapRenderTarget = jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::RG32F, ETextureFormat::RG, EFormatType::FLOAT, EDepthBufferType::DEPTH32, SM_WIDTH, SM_HEIGHT * NUM_CASCADES, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR });
		shadowMapData->ShadowMapSamplerState = jSamplerStatePool::GetSamplerState("LinearClampShadow");

		return shadowMapData;
	}

	jShadowMapArrayData* CreateShadowMapArray(const Vector& pos, const char* prefix = nullptr)
	{
		// todo remove constant variable
		auto shadowMapData = new jShadowMapArrayData(prefix);

		const float nearDist = 10.0f;
		const float farDist = 500.0f;
		shadowMapData->ShadowMapCamera[0] = jCamera::CreateCamera(pos, pos + Vector(1.0f, 0.0f, 0.0f), pos + Vector(0.0f, 1.0f, 0.0f), DegreeToRadian(90.0f), nearDist, farDist, SM_ARRAY_WIDTH, SM_ARRAY_HEIGHT, true);
		shadowMapData->ShadowMapCamera[1] = jCamera::CreateCamera(pos, pos + Vector(-1.0f, 0.0f, 0.0f), pos + Vector(0.0f, 1.0f, 0.0f), DegreeToRadian(90.0f), nearDist, farDist, SM_ARRAY_WIDTH, SM_ARRAY_HEIGHT, true);
		shadowMapData->ShadowMapCamera[2] = jCamera::CreateCamera(pos, pos + Vector(0.0f, 1.0f, 0.0f), pos + Vector(0.0f, 0.0f, -1.0f), DegreeToRadian(90.0f), nearDist, farDist, SM_ARRAY_WIDTH, SM_ARRAY_HEIGHT, true);
		shadowMapData->ShadowMapCamera[3] = jCamera::CreateCamera(pos, pos + Vector(0.0f, -1.0f, 0.0f), pos + Vector(0.0f, 0.0f, 1.0f), DegreeToRadian(90.0f), nearDist, farDist, SM_ARRAY_WIDTH, SM_ARRAY_HEIGHT, true);
		shadowMapData->ShadowMapCamera[4] = jCamera::CreateCamera(pos, pos + Vector(0.0f, 0.0f, 1.0f), pos + Vector(0.0f, 1.0f, 0.0f), DegreeToRadian(90.0f), nearDist, farDist, SM_ARRAY_WIDTH, SM_ARRAY_HEIGHT, true);
		shadowMapData->ShadowMapCamera[5] = jCamera::CreateCamera(pos, pos + Vector(0.0f, 0.0f, -1.0f), pos + Vector(0.0f, 1.0f, 0.0f), DegreeToRadian(90.0f), nearDist, farDist, SM_ARRAY_WIDTH, SM_ARRAY_HEIGHT, true);
		shadowMapData->ShadowMapRenderTarget = jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D_ARRAY_OMNISHADOW, ETextureFormat::RG32F, ETextureFormat::RG, EFormatType::FLOAT, EDepthBufferType::DEPTH32, SM_ARRAY_WIDTH, SM_ARRAY_HEIGHT * 6, 1, ETextureFilter::NEAREST, ETextureFilter::NEAREST });
		shadowMapData->ShadowMapSamplerState = jSamplerStatePool::GetSamplerState("LinearClampShadow");

		return shadowMapData;
	}

}

jLight* jLight::CreateAmbientLight(const Vector& color, const Vector& intensity)
{
	auto ambientLight = new jAmbientLight();
	JASSERT(ambientLight);
	
	ambientLight->Data.Color = color;
	ambientLight->Data.Intensity = intensity;
	
	return ambientLight;
}

//////////////////////////////////////////////////////////////////////////
jDirectionalLight* jLight::CreateDirectionalLight(const Vector& direction, const Vector& color, const Vector& diffuseIntensity, const Vector& specularIntensity, float specularPower)
{
	auto directionalLight = new jDirectionalLight();
	JASSERT(directionalLight);

	directionalLight->Data.Direction = direction;
	directionalLight->Data.Color = color;
	directionalLight->Data.DiffuseIntensity = diffuseIntensity;
	directionalLight->Data.SpecularIntensity = specularIntensity;
	directionalLight->Data.SpecularPow = specularPower;
	directionalLight->ShadowMapData = jLightUtil::CreateShadowMap(direction, Vector::ZeroVector);

	return directionalLight;
}

jCascadeDirectionalLight* jLight::CreateCascadeDirectionalLight(const Vector& direction, const Vector& color, const Vector& diffuseIntensity, const Vector& specularIntensity, float specularPower)
{
	auto directionalLight = new jCascadeDirectionalLight();
	JASSERT(directionalLight);

	directionalLight->Data.Direction = direction;
	directionalLight->Data.Color = color;
	directionalLight->Data.DiffuseIntensity = diffuseIntensity;
	directionalLight->Data.SpecularIntensity = specularIntensity;
	directionalLight->Data.SpecularPow = specularPower;
	directionalLight->ShadowMapData = jLightUtil::CreateCascadeShadowMap(direction, Vector::ZeroVector);

	return directionalLight;
}

jPointLight* jLight::CreatePointLight(const Vector& pos, const Vector& color, float maxDistance, const Vector& diffuseIntensity, const Vector& specularIntensity, float specularPower)
{
	auto pointLight = new jPointLight();
	pointLight->Data.Position = pos;
	pointLight->Data.Color = color;
	pointLight->Data.DiffuseIntensity = diffuseIntensity;
	pointLight->Data.SpecularIntensity = specularIntensity;
	pointLight->Data.SpecularPow = specularPower;
	pointLight->Data.MaxDistance = maxDistance;
	pointLight->ShadowMapData = jLightUtil::CreateShadowMapArray(pos, "PointLight");

	return pointLight;
}

jSpotLight* jLight::CreateSpotLight(const Vector& pos, const Vector& direction, const Vector& color, float maxDistance
	, float penumbraRadian, float umbraRadian, const Vector& diffuseIntensity, const Vector& specularIntensity, float specularPower)
{
	auto spotLight = new jSpotLight();

	spotLight->Data.Position = pos;
	spotLight->Data.Direction = direction;
	spotLight->Data.Color = color;
	spotLight->Data.DiffuseIntensity = diffuseIntensity;
	spotLight->Data.SpecularIntensity = specularIntensity;
	spotLight->Data.SpecularPow = specularPower;
	spotLight->Data.MaxDistance = maxDistance;
	spotLight->Data.PenumbraRadian = penumbraRadian;
	spotLight->Data.UmbraRadian = umbraRadian;
	spotLight->ShadowMapData = jLightUtil::CreateShadowMapArray(pos, "SpotLight");

	return spotLight;
}

void jLight::BindLights(const std::list<const jLight*>& lights, const jShader* shader)
{
	int ambient = 0;
	int directional = 0;
	int point = 0;
	int spot = 0;
	for (auto light : lights)
	{
		if (!light)
			continue;

		switch (light->Type)
		{
		case ELightType::AMBIENT:
			light->BindLight(shader);
			ambient = 1;
			break;
		case ELightType::DIRECTIONAL:
			light->BindLight(shader);
			++directional;
			break;
		case ELightType::POINT:
			light->BindLight(shader);
			++point;
			break;
		case ELightType::SPOT:
			light->BindLight(shader);
			++spot;
			break;
		default:
			break;
		}
	}

	g_rhi->SetUniformbuffer("UseAmbientLight", ambient, shader);
	g_rhi->SetUniformbuffer("NumOfDirectionalLight", directional, shader);
	g_rhi->SetUniformbuffer("NumOfPointLight", point, shader);
	g_rhi->SetUniformbuffer("NumOfSpotLight", spot, shader);
}

//////////////////////////////////////////////////////////////////////////

void jDirectionalLight::BindLight(const jShader* shader) const
{
	JASSERT(shader);

	//if (!LightDataUniformBlock->Data || *static_cast<LightData*>(LightDataUniformBlock->Data) != Data)
		LightDataUniformBlock->UpdateBufferData(&Data, sizeof(Data));
	LightDataUniformBlock->Bind(shader);

	if (ShadowMapData && ShadowMapData->IsValid())
	{
		struct ShadowData
		{
			Matrix ShadowVP_Transposed;
			Matrix ShadowV_Transposed;
			Vector LightPos;
			float Near;
			float Far;
			float padding0;
			Vector2 ShadowMapSize;

			bool operator == (const ShadowData& rhs) const
			{
				return (ShadowVP_Transposed == rhs.ShadowVP_Transposed) && (ShadowV_Transposed == rhs.ShadowV_Transposed) 
					&& (LightPos == rhs.LightPos) && (Near == rhs.Near) && (Far == rhs.Far) && (ShadowMapSize == rhs.ShadowMapSize);
			}

			bool operator != (const ShadowData& rhs) const
			{
				return !(*this == rhs);
			}

			void SetData(jLightUtil::jShadowMapData* shadowMapData)
			{
				auto camera = shadowMapData->ShadowMapCamera;
				JASSERT(camera);

				const auto& renderTargetInfo = shadowMapData->ShadowMapRenderTarget->Info;

				ShadowVP_Transposed = (camera->Projection * camera->View).GetTranspose();
				ShadowV_Transposed = (camera->View).GetTranspose();
				LightPos = camera->Pos;
				Near = camera->Near;
				Far = camera->Far;
				ShadowMapSize.x = static_cast<float>(renderTargetInfo.Width);
				ShadowMapSize.y = static_cast<float>(renderTargetInfo.Height);
			}
		};

		ShadowData shadowData;
		shadowData.SetData(ShadowMapData);

		IUniformBufferBlock* shadowDataUniformBlock = ShadowMapData->UniformBlock;
		//if (!shadowDataUniformBlock->Data || *static_cast<ShadowData*>(shadowDataUniformBlock->Data) != shadowData)
			shadowDataUniformBlock->UpdateBufferData(&shadowData, sizeof(shadowData));
		shadowDataUniformBlock->Bind(shader);

		char szTemp[128] = { 0 };
		for (int32 i = 0; i < NUM_CASCADES; ++i)
		{
			sprintf_s(szTemp, sizeof(szTemp), "CascadeLightVP[%d]", i);
			g_rhi->SetUniformbuffer(szTemp, ShadowMapData->CascadeLightVP[i], shader);

			sprintf_s(szTemp, sizeof(szTemp), "CascadeEndsW[%d]", i);
			g_rhi->SetUniformbuffer(szTemp, ShadowMapData->CascadeEndsW[i], shader);
		}
	}
}

void jDirectionalLight::GetMaterialData(jMaterialData* OutMaterialData) const
{
	JASSERT(OutMaterialData);

	if (OutMaterialData)
	{
		OutMaterialData->Params.insert(OutMaterialData->Params.end(), MaterialData.Params.begin(), MaterialData.Params.end());
	}
}

jRenderTarget* jDirectionalLight::GetShadowMapRenderTarget() const
{
	return (ShadowMapData ? ShadowMapData->ShadowMapRenderTarget.get() : nullptr);
}

std::shared_ptr<jRenderTarget> jDirectionalLight::GetShadowMapRenderTargetPtr() const
{
	return (ShadowMapData ? ShadowMapData->ShadowMapRenderTarget : nullptr);
}

jCamera* jDirectionalLight::GetLightCamra(int index /*= 0*/) const
{
	return (ShadowMapData ? ShadowMapData->ShadowMapCamera : nullptr);
}

jTexture* jDirectionalLight::GetShadowMap() const
{
	return (ShadowMapData && ShadowMapData->ShadowMapRenderTarget) ? ShadowMapData->ShadowMapRenderTarget->GetTexture() : nullptr;
}

void jDirectionalLight::RenderToShadowMap(const RenderToShadowMapFunc& func, const jShader* shader) const
{
	JASSERT(ShadowMapData);
	JASSERT(ShadowMapData->ShadowMapRenderTarget);
	g_rhi->SetShader(shader);
	const auto& renderTargetInfo = ShadowMapData->ShadowMapRenderTarget->Info;
	std::vector<jViewport> viewports{ jViewport{0.0f, 0.0f, static_cast<float>(renderTargetInfo.Width), static_cast<float>(renderTargetInfo.Height)} };
	func(ShadowMapData->ShadowMapRenderTarget.get(), 0, ShadowMapData->ShadowMapCamera, viewports);
}

void jDirectionalLight::Update(float deltaTime)
{
	if (ShadowMapData && ShadowMapData->ShadowMapCamera)
	{
		auto camera = ShadowMapData->ShadowMapCamera;
		jLightUtil::MakeDirectionalLightViewInfo(camera->Pos, camera->Target, camera->Up, Data.Direction);
		camera->UpdateCamera();

		UpdateMaterialData();
	}
}

void jDirectionalLight::UpdateMaterialData()
{
	if (!DirtyMaterialData)
		return;
	DirtyMaterialData = false;

	MaterialData.Params.clear();
	{
		auto materialParam = new jMaterialParam();
		materialParam->Name = "shadow_object";
		materialParam->Texture = ShadowMapData->ShadowMapRenderTarget->GetTextureDepth();
		materialParam->SamplerState = ShadowMapData->ShadowMapSamplerState.get();
		MaterialData.Params.push_back(materialParam);
	}

	{
		auto materialParam = new jMaterialParam();
		materialParam->Name = "shadow_object_test";

		const auto type = jShadowAppSettingProperties::GetInstance().ShadowMapType;
		if ((type == EShadowMapType::VSM) || (type == EShadowMapType::ESM) || (type == EShadowMapType::EVSM))
		{
			materialParam->Texture = ShadowMapData->ShadowMapRenderTarget->GetTexture();
		}
		else
		{
			materialParam->Texture = ShadowMapData->ShadowMapRenderTarget->GetTextureDepth();
		}
		materialParam->SamplerState = nullptr;
		MaterialData.Params.push_back(materialParam);
	}
}

//////////////////////////////////////////////////////////////////////////
// jCascadeDirectionalLight
void jCascadeDirectionalLight::RenderToShadowMap(const RenderToShadowMapFunc& func, const jShader* shader) const
{
	g_rhi->SetShader(shader);
	char szTemp[128] = { 0, };
	std::vector<jViewport> viewports;
	viewports.reserve(NUM_CASCADES);
	const int32 width = ShadowMapData->ShadowMapRenderTarget->Info.Width;
	const int32 height = ShadowMapData->ShadowMapRenderTarget->Info.Height / NUM_CASCADES;
	for (int i = 0; i < NUM_CASCADES; ++i)
	{
		sprintf_s(szTemp, sizeof(szTemp), "CascadeLightVP[%d]", i);
		g_rhi->SetUniformbuffer(szTemp, ShadowMapData->CascadeLightVP[i], shader);

		viewports.push_back({ 0.0f, static_cast<float>(height * i), static_cast<float>(width), static_cast<float>(height) });
	}
	func(ShadowMapData->ShadowMapRenderTarget.get(), 0, ShadowMapData->ShadowMapCamera, viewports);
}


void jPointLight::BindLight(const jShader* shader) const
{
	//if (!LightDataUniformBlock->Data || *static_cast<LightData*>(LightDataUniformBlock->Data) != Data)
		LightDataUniformBlock->UpdateBufferData(&Data, sizeof(Data));
	LightDataUniformBlock->Bind(shader);

	if (ShadowMapData && ShadowMapData->IsValid())
	{
		//// Both near and far are all the same within camera array.
		//struct ShadowData
		//{
		//	float Near;
		//	float Far;
		//	//Vector2 ShadowMapSize;
		//	// 16 byte aligned don't need a padding

		//	bool operator == (const ShadowData& rhs) const
		//	{
		//		return (Near == rhs.Near) && (Far == rhs.Far);// && (ShadowMapSize == rhs.ShadowMapSize);
		//	}

		//	bool operator != (const ShadowData& rhs) const
		//	{
		//		return !(*this == rhs);
		//	}

		//	void SetData(jLightUtil::jShadowMapArrayData* shadowMapData)
		//	{
		//		auto camera = shadowMapData->ShadowMapCamera[0];
		//		JASSERT(camera);

		//		const auto& renderTargetInfo = shadowMapData->ShadowMapRenderTarget->Info;

		//		Near = camera->Near;
		//		Far = camera->Far;
		//		//ShadowMapSize.x = static_cast<float>(renderTargetInfo.Width);
		//		//ShadowMapSize.y = static_cast<float>(renderTargetInfo.Height);
		//	}
		//};
		struct ShadowData
		{
			float Near;
			float Far;
			Vector2 ShadowMapSize;

			bool operator == (const ShadowData& rhs) const
			{
				return (Near == rhs.Near) && (Far == rhs.Far) && (ShadowMapSize == rhs.ShadowMapSize);
			}

			bool operator != (const ShadowData& rhs) const
			{
				return !(*this == rhs);
			}

			void SetData(jLightUtil::jShadowMapArrayData* shadowMapData)
			{
				auto camera = shadowMapData->ShadowMapCamera[0];
				JASSERT(camera);

				const auto& renderTargetInfo = shadowMapData->ShadowMapRenderTarget->Info;

				Near = camera->Near;
				Far = camera->Far;
				ShadowMapSize.x = static_cast<float>(renderTargetInfo.Width);
				ShadowMapSize.y = static_cast<float>(renderTargetInfo.Height);
			}
		};

		ShadowData shadowData;
		shadowData.SetData(ShadowMapData);

		IUniformBufferBlock* shadowDataUniformBlock = ShadowMapData->UniformBlock;
		//if (!shadowDataUniformBlock->Data || *static_cast<ShadowData*>(shadowDataUniformBlock->Data) != shadowData)
			shadowDataUniformBlock->UpdateBufferData(&shadowData, sizeof(shadowData));
		shadowDataUniformBlock->Bind(shader);
	}
}

void jPointLight::GetMaterialData(jMaterialData* OutMaterialData) const
{
	JASSERT(OutMaterialData);

	if (OutMaterialData)
	{
		OutMaterialData->Params.insert(OutMaterialData->Params.end(), MaterialData.Params.begin(), MaterialData.Params.end());
	}
}

jRenderTarget* jPointLight::GetShadowMapRenderTarget() const
{
	return (ShadowMapData ? ShadowMapData->ShadowMapRenderTarget.get() : nullptr);
}

std::shared_ptr<jRenderTarget> jPointLight::GetShadowMapRenderTargetPtr() const
{
	return (ShadowMapData ? ShadowMapData->ShadowMapRenderTarget : nullptr);
}

jCamera* jPointLight::GetLightCamra(int index /*= 0*/) const
{
	JASSERT(ShadowMapData && _countof(ShadowMapData->ShadowMapCamera) <= index);
	return (ShadowMapData ? ShadowMapData->ShadowMapCamera[index] : nullptr);
}

void jPointLight::RenderToShadowMap(const RenderToShadowMapFunc& func, const jShader* shader) const
{
	JASSERT(ShadowMapData);
	JASSERT(ShadowMapData->ShadowMapRenderTarget);
	if (ShadowMapData)
	{	
		g_rhi->SetShader(shader);
		char szTemp[128] = { 0, };
		std::vector<jViewport> viewports;
		viewports.reserve(6);
		const int32 width = ShadowMapData->ShadowMapRenderTarget->Info.Width;
		const int32 height = ShadowMapData->ShadowMapRenderTarget->Info.Height / 6;
		for (int i = 0; i < 6; ++i)
		{
			auto camera = ShadowMapData->ShadowMapCamera[i];
			const auto vp = (camera->Projection * camera->View);
			sprintf_s(szTemp, sizeof(szTemp), "OmniShadowMapVP[%d]", i);
			g_rhi->SetUniformbuffer(szTemp, vp, shader);
			viewports.push_back({ 0.0f, static_cast<float>(height * i), static_cast<float>(width), static_cast<float>(height) });
		}
		func(ShadowMapData->ShadowMapRenderTarget.get(), 0, ShadowMapData->ShadowMapCamera[0], viewports);
	}
}

void jPointLight::Update(float deltaTime)
{
	if (ShadowMapData)
	{
		auto camers = ShadowMapData->ShadowMapCamera;
		for (int32 i = 0; i < 6; ++i)
		{
			auto currentCamera = camers[i];
			const auto offset = Data.Position - currentCamera->Pos;
			currentCamera->Pos = Data.Position;
			currentCamera->Target += offset;
			currentCamera->Up += offset;
			currentCamera->UpdateCamera();
		}
	}
}

void jPointLight::UpdateMaterialData()
{
	if (!DirtyMaterialData)
		return;
	DirtyMaterialData = false;

	{
		auto materialParam = new jMaterialParam();
		materialParam->Name = "shadow_object_point_shadow";
		materialParam->Texture = ShadowMapData->ShadowMapRenderTarget->GetTextureDepth();
		materialParam->SamplerState = ShadowMapData->ShadowMapSamplerState.get();
		MaterialData.Params.push_back(materialParam);
	}

	{
		auto materialParam = new jMaterialParam();
		materialParam->Name = "shadow_object_point";
		const auto type = jShadowAppSettingProperties::GetInstance().ShadowMapType;
		if ((type == EShadowMapType::VSM) || (type == EShadowMapType::ESM) || (type == EShadowMapType::EVSM))
		{
			materialParam->Texture = ShadowMapData->ShadowMapRenderTarget->GetTexture();
		}
		else
		{
			materialParam->Texture = ShadowMapData->ShadowMapRenderTarget->GetTextureDepth();
		}
		materialParam->SamplerState = nullptr;
		MaterialData.Params.push_back(materialParam);
	}
}

void jSpotLight::BindLight(const jShader* shader) const
{
	//if (!LightDataUniformBlock->Data || *static_cast<LightData*>(LightDataUniformBlock->Data) != Data)
		LightDataUniformBlock->UpdateBufferData(&Data, sizeof(Data));
	LightDataUniformBlock->Bind(shader);

	if (ShadowMapData && ShadowMapData->IsValid())
	{
		// Both near and far are all the same within camera array.
		struct ShadowData
		{
			float Near;
			float Far;
			Vector2 ShadowMapSize;
			// 16 byte aligned don't need a padding

			bool operator == (const ShadowData& rhs) const
			{
				return (Near == rhs.Near) && (Far == rhs.Far) && (ShadowMapSize == rhs.ShadowMapSize);
			}

			bool operator != (const ShadowData& rhs) const
			{
				return !(*this == rhs);
			}

			void SetData(jLightUtil::jShadowMapArrayData* shadowMapData)
			{
				auto camera = shadowMapData->ShadowMapCamera[0];
				JASSERT(camera);

				const auto& renderTargetInfo = shadowMapData->ShadowMapRenderTarget->Info;

				Near = camera->Near;
				Far = camera->Far;
				ShadowMapSize.x = static_cast<float>(renderTargetInfo.Width);
				ShadowMapSize.y = static_cast<float>(renderTargetInfo.Height);
			}
		};

		ShadowData shadowData;
		shadowData.SetData(ShadowMapData);

		IUniformBufferBlock* shadowDataUniformBlock = ShadowMapData->UniformBlock;
		//if (!shadowDataUniformBlock->Data || *static_cast<ShadowData*>(shadowDataUniformBlock->Data) != shadowData)
			shadowDataUniformBlock->UpdateBufferData(&shadowData, sizeof(shadowData));
		shadowDataUniformBlock->Bind(shader);


	}
}

void jSpotLight::GetMaterialData(jMaterialData* OutMaterialData) const
{
	JASSERT(OutMaterialData);

	if (OutMaterialData)
	{
		OutMaterialData->Params.insert(OutMaterialData->Params.end(), MaterialData.Params.begin(), MaterialData.Params.end());
	}
}

jRenderTarget* jSpotLight::GetShadowMapRenderTarget() const
{
	return (ShadowMapData ? ShadowMapData->ShadowMapRenderTarget.get() : nullptr);
}

std::shared_ptr<jRenderTarget> jSpotLight::GetShadowMapRenderTargetPtr() const
{
	return (ShadowMapData ? ShadowMapData->ShadowMapRenderTarget : nullptr);
}

jCamera* jSpotLight::GetLightCamra(int index /*= 0*/) const
{
	JASSERT(ShadowMapData && _countof(ShadowMapData->ShadowMapCamera) <= index);
	return (ShadowMapData ? ShadowMapData->ShadowMapCamera[index] : nullptr);
}

void jSpotLight::RenderToShadowMap(const RenderToShadowMapFunc& func, const jShader* shader) const
{
	JASSERT(ShadowMapData);
	JASSERT(ShadowMapData->ShadowMapRenderTarget);
	if (ShadowMapData)
	{
		g_rhi->SetShader(shader);
		char szTemp[128] = { 0, };
		std::vector<jViewport> viewports;
		viewports.reserve(6);
		const int32 width = ShadowMapData->ShadowMapRenderTarget->Info.Width;
		const int32 height = ShadowMapData->ShadowMapRenderTarget->Info.Height / 6;
		for (int i = 0; i < 6; ++i)
		{
			auto camera = ShadowMapData->ShadowMapCamera[i];
			const auto vp = (camera->Projection * camera->View);
			sprintf_s(szTemp, sizeof(szTemp), "OmniShadowMapVP[%d]", i);
			g_rhi->SetUniformbuffer(szTemp, vp, shader);
			viewports.push_back({ 0.0f, static_cast<float>(height * i), static_cast<float>(width), static_cast<float>(height) });
		}
		func(ShadowMapData->ShadowMapRenderTarget.get(), 0, ShadowMapData->ShadowMapCamera[0], viewports);
	}
}

void jSpotLight::Update(float deltaTime)
{
	auto camers = ShadowMapData->ShadowMapCamera;
	for (int32 i = 0; i < 6; ++i)
	{
		auto currentCamera = camers[i];
		const auto offset = Data.Position - currentCamera->Pos;
		currentCamera->Pos = Data.Position;
		currentCamera->Target += offset;
		currentCamera->Up += offset;
		currentCamera->UpdateCamera();
	}
}

void jSpotLight::UpdateMaterialData()
{
	if (!DirtyMaterialData)
		return;
	DirtyMaterialData = false;

	{
		auto materialParam = new jMaterialParam();
		materialParam->Name = "shadow_object_spot_shadow";
		materialParam->Texture = ShadowMapData->ShadowMapRenderTarget->GetTextureDepth();
		materialParam->SamplerState = ShadowMapData->ShadowMapSamplerState.get();
		MaterialData.Params.push_back(materialParam);
	}

	{
		auto materialParam = new jMaterialParam();
		materialParam->Name = "shadow_object_spot";
		const auto type = jShadowAppSettingProperties::GetInstance().ShadowMapType;
		if ((type == EShadowMapType::VSM) || (type == EShadowMapType::ESM) || (type == EShadowMapType::EVSM))
		{
			materialParam->Texture = ShadowMapData->ShadowMapRenderTarget->GetTexture();
		}
		else
		{
			materialParam->Texture = ShadowMapData->ShadowMapRenderTarget->GetTextureDepth();
		}
		materialParam->SamplerState = nullptr;
		MaterialData.Params.push_back(materialParam);
	}
}

//////////////////////////////////////////////////////////////////////////
void jAmbientLight::BindLight(const jShader* shader) const
{
	//const std::string structName = "AmbientLight";

	g_rhi->SetUniformbuffer("AmbientLight.Color", Data.Color, shader);
	g_rhi->SetUniformbuffer("AmbientLight.Intensity", Data.Intensity, shader);
}
