#include "pch.h"
#include "jPipeline.h"
#include "jObject.h"
#include "jLight.h"
#include "jCamera.h"
#include "jGBuffer.h"
#include "jRenderTargetPool.h"
#include "jPostProcess.h"
#include "jRenderObject.h"
#include "jShadowVolume.h"
#include "jShadowAppProperties.h"
#include "jRHI.h"
#include "jVertexAdjacency.h"
#include "jPrimitiveUtil.h"

const std::list<jObject*> jPipelineContext::emptyObjectList;
//const std::list<const jLight*> jPipelineContext::emptyLightList;

std::unordered_map<std::string, IPipeline*> IPipeline::s_pipelinesNameMap;
std::unordered_set<IPipeline*> IPipeline::s_pipelinesNameSet;

void IPipeline::AddPipeline(const std::string& name, IPipeline* newPipeline)
{
	newPipeline->Name = name;
	s_pipelinesNameMap.insert(std::make_pair(name, newPipeline));
	s_pipelinesNameSet.insert(newPipeline);
}

IPipeline* IPipeline::GetPipeline(const std::string& name)
{
	auto it_find = s_pipelinesNameMap.find(name);
	return s_pipelinesNameMap.end() != it_find ? it_find->second : nullptr;
}

void IPipeline::SetupPipelines()
{
	for (auto it : s_pipelinesNameMap)
	{
		if (it.second)
			it.second->Setup();
	}
}

void IPipeline::TeardownPipelines()
{
	for (auto it : s_pipelinesNameMap)
	{
		if (it.second)
			it.second->Teardown();
	}
}

struct jShadowPipelinCreation
{
	jShadowPipelinCreation()
	{
		ADD_FORWARD_SHADOWMAP_GEN_PIPELINE(Forward_ShadowMapGen_SSM_Pipeline, "ShadowGen_SSM", "ShadowGen_Omni_SSM");
		ADD_FORWARD_SHADOWMAP_GEN_PIPELINE(Forward_ShadowMapGen_VSM_Pipeline, "ShadowGen_VSM", "ShadowGen_Omni_SSM");
		ADD_FORWARD_SHADOWMAP_GEN_PIPELINE(Forward_ShadowMapGen_ESM_Pipeline, "ShadowGen_ESM", "ShadowGen_Omni_ESM");
		ADD_FORWARD_SHADOWMAP_GEN_PIPELINE(Forward_ShadowMapGen_EVSM_Pipeline, "ShadowGen_EVSM", "ShadowGen_Omni_EVSM");

		ADD_FORWARD_SHADOW_PIPELINE(Forward_SSM_Pipeline, "SSM");
		ADD_FORWARD_SHADOW_PIPELINE(Forward_SSM_PCF_Pipeline, "SSM_PCF");
		ADD_FORWARD_SHADOW_PIPELINE(Forward_SSM_PCSS_Pipeline, "SSM_PCSS");
		ADD_FORWARD_SHADOW_PIPELINE(Forward_SSM_PCF_Poisson_Pipeline, "SSM_PCF_Poisson");
		ADD_FORWARD_SHADOW_PIPELINE(Forward_SSM_PCSS_Poisson_Pipeline, "SSM_PCSS_Poisson");
		ADD_FORWARD_SHADOW_PIPELINE(Forward_VSM_Pipeline, "VSM");
		ADD_FORWARD_SHADOW_PIPELINE(Forward_ESM_Pipeline, "ESM");
		ADD_FORWARD_SHADOW_PIPELINE(Forward_EVSM_Pipeline, "EVSM");
		ADD_FORWARD_SHADOW_PIPELINE(Forward_CSM_SSM_Pipeline, "CSM_SSM");

		IPipeline::AddPipeline("Forward_BoundVolume_Pipeline", new jForward_DebugObject_Pipeline("BoundVolumeShader"));
		IPipeline::AddPipeline("Forward_DebugObject_Pipeline", new jForward_DebugObject_Pipeline("DebugObjectShader"));
		IPipeline::AddPipeline("Forward_UI_Pipeline", new jForward_UIObject_Pipeline("UIShader"));

		IPipeline::AddPipeline("Forward_ShadowMapGen_CSM_SSM_Pipeline", new jForward_ShadowMapGen_CSM_SSM_Pipeline("CSM_SSM_TEX2D_ARRAY", "ShadowGen_Omni_SSM"));
	}
} s_shadowPipelinCreation;

//////////////////////////////////////////////////////////////////////////
// jRenderPipeline
void jRenderPipeline::Do(const jPipelineContext& pipelineContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, Name.c_str());

	Draw(pipelineContext, Shader);
}

void jRenderPipeline::Draw(const jPipelineContext& pipelineContext) const
{
	Draw(pipelineContext, Shader);
}

void jRenderPipeline::Draw(const jPipelineContext& pipelineContext, const jShader* shader) const
{
	if (EnableClear)
	{
		g_rhi->SetClearColor(ClearColor);
		g_rhi->SetClear(ClearType);
	}

	g_rhi->EnableDepthTest(EnableDepthTest);
	g_rhi->SetDepthFunc(DepthStencilFunc);

	g_rhi->EnableBlend(EnableBlend);
	g_rhi->SetBlendFunc(BlendSrc, BlendDest);

	g_rhi->EnableDepthBias(EnableDepthBias);
	g_rhi->SetDepthBias(DepthConstantBias, DepthSlopeBias);

	g_rhi->SetShader(shader);

	for (const auto& iter : Buffers)
		iter->Bind(shader);

	pipelineContext.Camera->BindCamera(shader);
	jLight::BindLights(pipelineContext.Lights, shader);

	for (const auto& iter : pipelineContext.Objects)
		iter->Draw(pipelineContext.Camera, shader, pipelineContext.Lights);
}


//////////////////////////////////////////////////////////////////////////
// jDeferredGeometryPipeline
void jDeferredGeometryPipeline::Setup()
{
	// ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	ClearColor = Vector4(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);	// light sky blue
	ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
}

void jDeferredGeometryPipeline::Draw(const jPipelineContext& pipelineContext, const jShader* shader) const
{
	JASSERT(GBuffer);
	if (GBuffer->Begin())
	{
		if (auto currentShader = jShadowAppSettingProperties::GetInstance().ExponentDeepShadowOn ? jShader::GetShader("ExpDeferred") : jShader::GetShader("Deferred"))
		{
			__super::Draw(pipelineContext, currentShader);
		}
		GBuffer->End();
	}
}

//////////////////////////////////////////////////////////////////////////
// jDeepShadowMap_ShadowPass_Pipeline
void jDeepShadowMap_ShadowPass_Pipeline::Setup()
{
	ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
	EnableDepthTest = true;
	DepthStencilFunc = EComparisonFunc::LESS;
	EnableDepthBias = true;
	DepthSlopeBias = 1.0f;
	DepthConstantBias = 1.0f;

	Buffers.push_back(DeepShadowMapBuffers.AtomicBuffer);
	Buffers.push_back(DeepShadowMapBuffers.StartElementBuf);
	Buffers.push_back(DeepShadowMapBuffers.LinkedListEntryDepthAlphaNext);
}

void jDeepShadowMap_ShadowPass_Pipeline::Draw(const jPipelineContext& pipelineContext, const jShader* shader) const
{
	DeepShadowMapBuffers.AtomicBuffer->ClearBuffer(0);
	DeepShadowMapBuffers.StartElementBuf->ClearBuffer(-1);

	for (auto light : pipelineContext.Lights)
	{
		JASSERT(light);
		JASSERT(light->GetShadowMapRenderTarget());
		JASSERT(light->GetLightCamra());
		const auto renderTarget = light->GetShadowMapRenderTarget();
		const auto lightCamera = light->GetLightCamra();

		if (renderTarget->Begin())
		{
			if (auto currentShader = jShadowAppSettingProperties::GetInstance().ExponentDeepShadowOn ? jShader::GetShader("ExpDeepShadowMapGen") : jShader::GetShader("DeepShadowMapGen"))
			{
				g_rhi->SetShader(currentShader);

				__super::Draw(jPipelineContext(pipelineContext.DefaultRenderTarget, pipelineContext.Objects, lightCamera, { light }), currentShader);
			}
			renderTarget->End();
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// jForward_ShadowMapGen_Pipeline
void jForward_ShadowMapGen_Pipeline::Setup()
{
	ClearColor = Vector4(FLT_MAX, FLT_MAX, FLT_MAX, 1.0f);
	ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
	EnableDepthTest = true;
	DepthStencilFunc = EComparisonFunc::LESS;
	ShadowGenShader = jShader::GetShader(DirectionalLightShaderName);
	JASSERT(ShadowGenShader);
	OmniShadowGenShader = jShader::GetShader(OmniDirectionalLightShaderName);
	JASSERT(OmniShadowGenShader);
}

void jForward_ShadowMapGen_Pipeline::Do(const jPipelineContext& pipelineContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, Name.c_str());

	//light->Update(0); // todo remove
	g_rhi->SetRenderTarget(pipelineContext.DefaultRenderTarget);

	for (auto light : pipelineContext.Lights)
	{
		JASSERT(light);

		bool skip = false;

		jShader* currentShader = nullptr;
		switch (light->Type)
		{
		case ELightType::DIRECTIONAL:
			currentShader = ShadowGenShader;
			break;
		case ELightType::POINT:
		case ELightType::SPOT:
			currentShader = OmniShadowGenShader;
			break;
		case ELightType::AMBIENT:
			skip = true;
			break;
		default:
			JASSERT(0);
			return;
		}

		if (skip)
			continue;

		light->RenderToShadowMap([&pipelineContext, currentShader, light, this](const jRenderTarget* renderTarget
			, int32 renderTargetIndex, const jCamera* camera, const std::vector<jViewport>& viewports)
		{
			g_rhi->SetRenderTarget(renderTarget, renderTargetIndex);
			if (viewports.empty())
				g_rhi->SetViewport({ 0, 0, renderTarget->Info.Width, renderTarget->Info.Height });
			else
				g_rhi->SetViewportIndexedArray(0, static_cast<int32>(viewports.size()), &viewports[0]);
			this->jRenderPipeline::Draw(jPipelineContext(pipelineContext.DefaultRenderTarget, pipelineContext.Objects, camera, { light }), currentShader);
		}, currentShader);
	}

	g_rhi->SetRenderTarget(pipelineContext.DefaultRenderTarget);
}

//////////////////////////////////////////////////////////////////////////
// jForward_ShadowMapGen_CSM_SSM_Pipeline
void jForward_ShadowMapGen_CSM_SSM_Pipeline::Setup()
{
	ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
	EnableDepthTest = true;
	DepthStencilFunc = EComparisonFunc::LESS;
	EnableBlend = true;
	BlendSrc = EBlendSrc::ONE;
	BlendDest = EBlendDest::ZERO;

	ShadowGenShader = jShader::GetShader(DirectionalLightShaderName);
	JASSERT(ShadowGenShader);
	OmniShadowGenShader = jShader::GetShader(OmniDirectionalLightShaderName);
	JASSERT(OmniShadowGenShader);
}

void jForward_ShadowMapGen_CSM_SSM_Pipeline::Do(const jPipelineContext& pipelineContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, Name.c_str());

	// todo 여러개의 DIrectional light인경우 고려 필요.
	if (pipelineContext.Lights.empty())
		return;

	const jDirectionalLight* directionalLight = [&pipelineContext]() -> const jDirectionalLight*
	{
		for (auto& iter : pipelineContext.Lights)
		{
			if (iter->Type == ELightType::DIRECTIONAL)
				return static_cast<const jDirectionalLight*>(iter);
		}
		return nullptr;
	}();

	g_rhi->SetRenderTarget(pipelineContext.DefaultRenderTarget);

	for (auto light : pipelineContext.Lights)
	{
		bool skip = false;

		jShader* currentShader = nullptr;
		switch (light->Type)
		{
		case ELightType::DIRECTIONAL:
			currentShader = ShadowGenShader;
			break;
		case ELightType::POINT:
		case ELightType::SPOT:
		{
			currentShader = OmniShadowGenShader;

			light->RenderToShadowMap([&pipelineContext, currentShader, light, this](const jRenderTarget* renderTarget
				, int32 renderTargetIndex, const jCamera* camera, const std::vector<jViewport>& viewports)
				{
					g_rhi->SetRenderTarget(renderTarget, renderTargetIndex);
					if (viewports.empty())
						g_rhi->SetViewport({ 0, 0, renderTarget->Info.Width, renderTarget->Info.Height });
					else
						g_rhi->SetViewportIndexedArray(0, static_cast<int32>(viewports.size()), &viewports[0]);
					this->Draw(jPipelineContext(pipelineContext.DefaultRenderTarget, pipelineContext.Objects, camera, { light }), currentShader);
				}, currentShader);
			g_rhi->SetRenderTarget(pipelineContext.DefaultRenderTarget);
			skip = true;
		}
		break;
		case ELightType::AMBIENT:
			skip = true;
			break;
		default:
			JASSERT(0);
			return;
		}

		if (skip)
			continue;

		//////////////////////////////////////////////////////////////////////////
		// CSM Diretional Light 그리는 곳
		auto camera = pipelineContext.Camera;
		const auto shadowMapData = directionalLight->ShadowMapData;
		const auto shadowCameraNear = shadowMapData->ShadowMapCamera->Near;
		const auto shadowCameraFar = shadowMapData->ShadowMapCamera->Far;

		float cascadeEnds[NUM_CASCADES + 1] = { 0.0f, 0.2f, 0.4f, 1.0f };
		Matrix cascadeLightVP[NUM_CASCADES];

		char szTemp[128] = { 0, };
		for (int k = 0; k < NUM_CASCADES; ++k)
		{
			shadowMapData->CascadeEndsW[k] = shadowCameraNear + (shadowCameraFar - shadowCameraNear) * cascadeEnds[k + 1];

			// Get the 8 points of the view frustum in world space
			Vector frustumCornersWS[8] =
			{
				Vector(-1.0f,  1.0f, -1.0f),
				Vector( 1.0f,  1.0f, -1.0f),
				Vector( 1.0f, -1.0f, -1.0f),
				Vector(-1.0f, -1.0f, -1.0f),
				Vector(-1.0f,  1.0f,  1.0f),
				Vector( 1.0f,  1.0f,  1.0f),
				Vector( 1.0f, -1.0f,  1.0f),
				Vector(-1.0f, -1.0f,  1.0f),
			};

			Matrix invViewProj = (camera->Projection * camera->View).GetInverse();
			for (uint32 i = 0; i < 8; ++i)
				frustumCornersWS[i] = invViewProj.Transform(frustumCornersWS[i]);

			// Get the corners of the current cascade slice of the view frustum
			for (uint32 i = 0; i < 4; ++i)
			{
				Vector cornerRay = frustumCornersWS[i + 4] - frustumCornersWS[i];
				Vector nearCornerRay = cornerRay * cascadeEnds[k];
				Vector farCornerRay = cornerRay * cascadeEnds[k + 1];
				frustumCornersWS[i + 4] = frustumCornersWS[i] + farCornerRay;
				frustumCornersWS[i] = frustumCornersWS[i] + nearCornerRay;
			}			

			// Calculate the centroid of the view frustum slice
			Vector frustumCenter(0.0f);
			for (uint32 i = 0; i < 8; ++i)
				frustumCenter += frustumCornersWS[i];
			frustumCenter *= (1.0f / 8.0f);

			auto upDir = Vector::UpVector;

			// Create a temporary view matrix for the light
			Vector lightCameraPos = frustumCenter;
			Vector lookAt = lightCameraPos + directionalLight->Data.Direction.GetNormalize();
			Matrix lightView = jCameraUtil::CreateViewMatrix(lightCameraPos, lookAt, lightCameraPos + upDir);

			// Calculate an AABB around the frustum corners
			Vector mins(FLT_MAX);
			Vector maxes(-FLT_MAX);

			float sphereRadius = 0.0f;
			for (uint32 i = 0; i < 8; ++i)
			{
				float dist = (frustumCornersWS[i] - frustumCenter).Length();
				sphereRadius = std::max(sphereRadius, dist);
			}

			sphereRadius = std::ceil(sphereRadius * 16.0f) / 16.0f;

			maxes = Vector(sphereRadius, sphereRadius, sphereRadius);
			mins = -maxes;

			Vector cascadeExtents = maxes - mins;

			// Get position of the shadow camera
			Vector shadowCameraPos = frustumCenter - directionalLight->Data.Direction.GetNormalize() * fabs(mins.z);

			auto shadowCamera = jOrthographicCamera::CreateCamera(shadowCameraPos, frustumCenter, shadowCameraPos + upDir
				, mins.x, mins.y, maxes.x, maxes.y, cascadeExtents.z, 1.0f);
			shadowCamera->UpdateCamera();

			shadowMapData->CascadeLightVP[k] = shadowCamera->Projection * shadowCamera->View;
		}

		g_rhi->EnableDepthClip(false);

		directionalLight->RenderToShadowMap([&pipelineContext, light, currentShader, this](const jRenderTarget* renderTarget
			, int32 renderTargetIndex, const jCamera* camera, const std::vector<jViewport>& viewports)
			{
				g_rhi->SetRenderTarget(renderTarget, renderTargetIndex);
				if (viewports.empty())
					g_rhi->SetViewport({ 0, 0, renderTarget->Info.Width, renderTarget->Info.Height });
				else
					g_rhi->SetViewportIndexedArray(0, static_cast<int32>(viewports.size()), &viewports[0]);
				this->Draw(jPipelineContext(pipelineContext.DefaultRenderTarget, pipelineContext.Objects, camera, { light }), currentShader);
			}, currentShader);
		g_rhi->EnableDepthClip(true);
		g_rhi->SetRenderTarget(pipelineContext.DefaultRenderTarget);
	}
}

//////////////////////////////////////////////////////////////////////////
// jForwardShadowMap_Blur_Pipeline
void jForwardShadowMap_Blur_Pipeline::Setup()
{
	ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
	EnableDepthTest = false;
	DepthStencilFunc = EComparisonFunc::LESS;

	PostProcessBlur = std::shared_ptr<jPostProcess_Blur>(new jPostProcess_Blur("Blur", true));
	PostProcessInput = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	PostProcessOutput = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
}

void jForwardShadowMap_Blur_Pipeline::Draw(const jPipelineContext& pipelineContext, const jShader* shader) const
{
	for (auto light : pipelineContext.Lights)
	{
		JASSERT(light);

		bool skip = false;
		bool isOmniDirectional = false;
		const jShader* shader = nullptr;
		switch (light->Type)
		{
		case ELightType::DIRECTIONAL:
			isOmniDirectional = false;
			break;
		case ELightType::POINT:
		case ELightType::SPOT:
			isOmniDirectional = true;
			break;
		case ELightType::AMBIENT:
			skip = true;
			break;
		default:
			JASSERT(0);
			return;
		}

		if (skip)
			continue;

		JASSERT(light->GetShadowMapRenderTarget());
		auto tempRenderTargetPtr = jRenderTargetPool::GetRenderTarget(light->GetShadowMapRenderTarget()->Info);

		PostProcessBlur->OmniDirectional = isOmniDirectional;

		PostProcessBlur->IsVertical = true;
		PostProcessBlur->ClearInOutputs();

		PostProcessInput->RenderTarget = light->GetShadowMapRenderTarget();
		PostProcessBlur->AddInput(PostProcessInput);

		PostProcessOutput->RenderTarget = tempRenderTargetPtr.get();
		PostProcessBlur->SetOutput(PostProcessOutput);

		PostProcessBlur->Process(pipelineContext.Camera);

		PostProcessBlur->IsVertical = false;
		PostProcessBlur->ClearInOutputs();
	
		PostProcessInput->RenderTarget = tempRenderTargetPtr.get();
		PostProcessBlur->AddInput(PostProcessInput);
		
		PostProcessOutput->RenderTarget = light->GetShadowMapRenderTargetPtr().get();
		PostProcessBlur->SetOutput(PostProcessOutput);

		PostProcessBlur->Process(pipelineContext.Camera);

		jRenderTargetPool::ReturnRenderTarget(tempRenderTargetPtr.get());
	}
}

//////////////////////////////////////////////////////////////////////////
// jForward_Shadow_Pipeline
void jForward_Shadow_Pipeline::Setup()
{
	//ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	ClearColor = Vector4(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);	// light sky blue
	ClearType = ERenderBufferType::COLOR | ERenderBufferType::DEPTH;
	EnableDepthTest = true;
	DepthStencilFunc = EComparisonFunc::LESS;
	EnableBlend = true;
	BlendSrc = EBlendSrc::ONE;
	BlendDest = EBlendDest::ZERO;
	Shader = jShader::GetShader(ShaderName);
}

void jForward_Shadow_Pipeline::Do(const jPipelineContext& pipelineContext) const
{
	g_rhi->SetRenderTarget(pipelineContext.DefaultRenderTarget);
	__super::Do(pipelineContext);
}

//////////////////////////////////////////////////////////////////////////
// jComputePipeline
void jComputePipeline::Do(const jPipelineContext& pipelineContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, Name.c_str());
	Dispatch();
}

void jComputePipeline::Dispatch() const
{
	g_rhi->SetShader(Shader);

	for (const auto& iter : Buffers)
		iter->Bind(Shader);

	g_rhi->DispatchCompute(NumGroupsX, NumGroupsY, NumGroupsZ);
}

//////////////////////////////////////////////////////////////////////////
// jDeepShadowMap_Sort_ComputePipeline
void jDeepShadowMap_Sort_ComputePipeline::Setup()
{
	Shader = jShader::GetShader("cs_sort");
	ShadowMapWidthUniform = new jUniformBuffer<int>("ShadowMapWidth", SM_LINKED_LIST_WIDTH);
	ShadowMapHeightUniform = new jUniformBuffer<int>("ShadowMapHeight", SM_LINKED_LIST_HEIGHT);

	Buffers.push_back(ShadowMapWidthUniform);
	Buffers.push_back(ShadowMapHeightUniform);
	Buffers.push_back(DeepShadowMapBuffers.AtomicBuffer);
	Buffers.push_back(DeepShadowMapBuffers.StartElementBuf);
	Buffers.push_back(DeepShadowMapBuffers.LinkedListEntryDepthAlphaNext);

	NumGroupsX = SM_LINKED_LIST_WIDTH / 16;
	NumGroupsY = SM_LINKED_LIST_HEIGHT / 8;
	NumGroupsZ = 1;
}

void jDeepShadowMap_Sort_ComputePipeline::Teardown()
{
	delete ShadowMapWidthUniform;
	delete ShadowMapHeightUniform;
}

//////////////////////////////////////////////////////////////////////////
// jDeepShadowMap_Sort_ComputePipeline
void jDeepShadowMap_Link_ComputePipeline::Setup()
{
	Shader = jShader::GetShader("cs_link");
	
	ShadowMapWidthUniform = new jUniformBuffer<int>("ShadowMapWidth", SM_LINKED_LIST_WIDTH);
	ShadowMapHeightUniform = new jUniformBuffer<int>("ShadowMapHeight", SM_LINKED_LIST_HEIGHT);

	Buffers.push_back(ShadowMapWidthUniform);
	Buffers.push_back(ShadowMapHeightUniform);
	Buffers.push_back(DeepShadowMapBuffers.AtomicBuffer);
	Buffers.push_back(DeepShadowMapBuffers.StartElementBuf);
	Buffers.push_back(DeepShadowMapBuffers.LinkedListEntryDepthAlphaNext);
	Buffers.push_back(DeepShadowMapBuffers.LinkedListEntryNeighbors);

	NumGroupsX = SM_LINKED_LIST_WIDTH / 16;
	NumGroupsY = SM_LINKED_LIST_HEIGHT / 8;
	NumGroupsZ = 1;
}

void jDeepShadowMap_Link_ComputePipeline::Teardown()
{
	delete ShadowMapWidthUniform;
	delete ShadowMapHeightUniform;
}

//////////////////////////////////////////////////////////////////////////
// jDeferredDeepShadowMapPipelineSet
void jDeferredDeepShadowMapPipelineSet::Setup()
{
	// Setup SSBO
	DeepShadowMapBuffers.LinkedListEntryDepthAlphaNext = static_cast<jShaderStorageBufferObject_OpenGL*>(g_rhi->CreateShaderStorageBufferObject("LinkedListEntryDepthAlphaNext"));
	DeepShadowMapBuffers.LinkedListEntryDepthAlphaNext->UpdateBufferData(nullptr, jDeepShadowMapBuffers::LinkedListDepthSize * (sizeof(float) * 2 + sizeof(uint32) * 2));

	DeepShadowMapBuffers.StartElementBuf = static_cast<jShaderStorageBufferObject_OpenGL*>(g_rhi->CreateShaderStorageBufferObject("StartElementBufEntry"));
	DeepShadowMapBuffers.StartElementBuf->UpdateBufferData(nullptr, jDeepShadowMapBuffers::LinkedListDepthSize * sizeof(int32));

	DeepShadowMapBuffers.LinkedListEntryNeighbors = static_cast<jShaderStorageBufferObject_OpenGL*>(g_rhi->CreateShaderStorageBufferObject("LinkedListEntryNeighbors"));
	DeepShadowMapBuffers.LinkedListEntryNeighbors->UpdateBufferData(nullptr, jDeepShadowMapBuffers::LinkedListDepthSize * sizeof(int32) * 2);

	DeepShadowMapBuffers.AtomicBuffer = static_cast<jAtomicCounterBuffer_OpenGL*>(g_rhi->CreateAtomicCounterBuffer("LinkedListCounter", 3));

	uint32 zero = 0;
	DeepShadowMapBuffers.AtomicBuffer->UpdateBufferData(&zero, sizeof(zero));

	// Setup ShadowPrePass
	ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(ShadowPrePass, jDeepShadowMap_ShadowPass_Pipeline, DeepShadowMapBuffers);
	ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(RenderPass, jDeferredGeometryPipeline, GBuffer);
	ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(PostRenderPass, jDeepShadowMap_Sort_ComputePipeline, DeepShadowMapBuffers);
	ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(PostRenderPass, jDeepShadowMap_Link_ComputePipeline, DeepShadowMapBuffers);

	// Deferred 이지만 Debug 정보는 Forward로 렌더링 함.
	ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(DebugRenderPass, jForward_DebugObject_Pipeline, "DebugObjectShader");
	ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(BoundVolumeRenderPass, jForward_DebugObject_Pipeline, "BoundVolumeShader");
}

//////////////////////////////////////////////////////////////////////////
// jForward_DebugObject_Pipeline
void jForward_DebugObject_Pipeline::Setup()
{
	EnableClear = false;
	EnableDepthTest = true;
	DepthStencilFunc = EComparisonFunc::LESS;
	EnableBlend = true;
	BlendSrc = EBlendSrc::SRC_ALPHA;
	BlendDest = EBlendDest::ONE_MINUS_SRC_ALPHA;
	Shader = jShader::GetShader(ShaderName);
}

//////////////////////////////////////////////////////////////////////////
// jForward_ShadowVolume_Pipeline
void jForward_ShadowVolume_Pipeline::Setup()
{
	EnableClear = true;
	EnableDepthTest = true;
	DepthStencilFunc = EComparisonFunc::LESS;
	EnableBlend = true;
	BlendSrc = EBlendSrc::ONE;
	BlendDest = EBlendDest::ZERO;
	ClearColor = Vector4(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);	// light sky blue
}

//////////////////////////////////////////////////////////////////////////
// jForward_ShadowVolume_Pipeline
bool jForward_ShadowVolume_Pipeline::CanSkipShadowObject(const IShadowVolume* shadowVolume, const jCamera* camera, const jObject* object
	, const Vector& lightPosOrDirection, bool isOmniDirectional, const jLight* light) const
{
	if (!shadowVolume || object->SkipUpdateShadowVolume)
		return true;

	const float radius = object->RenderObject->Scale.x;
	//var radius = 0.0;
	//if (obj.hasOwnProperty('radius'))
	//	radius = obj.radius;
	//else
	//	radius = obj.scale.x;

	if (isOmniDirectional)  // Sphere or Spot Light
	{
		float maxDistance = 0.0f;
		if (light->Type == ELightType::POINT)
			maxDistance = static_cast<const jPointLight*>(light)->Data.MaxDistance;
		else if (light->Type == ELightType::SPOT)
			maxDistance = static_cast<const jSpotLight*>(light)->Data.MaxDistance;

		// 1. check out of light radius with obj
		const auto isCasterOutOfLightRadius = ((lightPosOrDirection - object->RenderObject->Pos).Length() > maxDistance);
		if (isCasterOutOfLightRadius)
			return true;

		// 2. check direction against frustum
		if (!camera->IsInFrustumWithDirection(object->RenderObject->Pos, object->RenderObject->Pos - lightPosOrDirection, radius))
			return true;

		// 3. check Spot light range with obj
		if (light->Type == ELightType::SPOT)
		{
			const auto lightToObjVector = object->RenderObject->Pos - lightPosOrDirection;
			const auto radianOfRadiusOffset = atanf(radius / lightToObjVector.Length());

			auto spotLight = static_cast<const jSpotLight*>(light);
			const auto radian = lightToObjVector.GetNormalize().DotProduct(spotLight->Data.Direction);
			const auto limitRadian = cosf(Max(spotLight->Data.UmbraRadian, spotLight->Data.PenumbraRadian)) - radianOfRadiusOffset;
			if (limitRadian > radian)
				return true;
		}
	}
	else       // Directional light
	{
		// 1. check direction against frustum
		if (!camera->IsInFrustumWithDirection(object->RenderObject->Pos, lightPosOrDirection, radius))
			return true;
	}
	return false;
}


void jForward_ShadowVolume_Pipeline::Do(const jPipelineContext& pipelineContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, Name.c_str());

	g_rhi->SetRenderTarget(pipelineContext.DefaultRenderTarget);

	auto camera = pipelineContext.Camera;

	const auto ambientShader = jShader::GetShader("AmbientOnly");
	const auto shadowVolumeBaseShader = jShader::GetShader("ShadowVolume");
	const auto ShadowVolumeCPU = jShader::GetShader("ShadowVolumeCPU");
	const auto ShadowVolumeGPU = jShader::GetShader("ShadowVolumeGPU");
	const auto isGPUShadowVolume = jShadowAppSettingProperties::GetInstance().IsGPUShadowVolume;
	const auto shadowVolumeShader = (isGPUShadowVolume ? ShadowVolumeGPU : ShadowVolumeCPU);

	//////////////////////////////////////////////////////////////////
	// 1. Render objects to depth buffer and Ambient & Emissive to color buffer.
	g_rhi->EnableDepthBias(false);

	g_rhi->EnableBlend(true);
	g_rhi->SetBlendFunc(EBlendSrc::ONE, EBlendDest::ZERO);

	g_rhi->EnableDepthTest(true);

	const_cast<jCamera*>(camera)->IsEnableCullMode = true;		// todo remove
	g_rhi->SetClearColor(ClearColor);
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH | ERenderBufferType::STENCIL);

	g_rhi->SetDepthFunc(EComparisonFunc::LESS);
	g_rhi->EnableStencil(false);

	g_rhi->SetDepthMask(true);
	g_rhi->SetColorMask(true, true, true, true);

	const_cast<jCamera*>(camera)->UseAmbient = true;			// todo remove
	const auto& staticObjects = jObject::GetStaticObject();
	g_rhi->SetShader(ambientShader);
	camera->BindCamera(ambientShader);
	jLight::BindLights({ camera->Ambient }, ambientShader);
	for (auto& iter : staticObjects)
		iter->Draw(camera, ambientShader, { camera->Ambient });

	//////////////////////////////////////////////////////////////////
	// 2. Stencil volume update & rendering (z-fail)
	const_cast<jCamera*>(camera)->UseAmbient = false;			// todo remove
	g_rhi->EnableStencil(true);

	for(auto& light : pipelineContext.Lights)
	{
		bool isOmniDirectional = false;
		Vector lightPosOrDirection;

		bool skip = false;
		switch (light->Type)
		{
		case ELightType::DIRECTIONAL:
		{
			auto directionalLight = static_cast<const jDirectionalLight*>(light);
			if (directionalLight)
				lightPosOrDirection = directionalLight->Data.Direction;
			break;
		}
		case ELightType::POINT:
		{
			auto pointLight = static_cast<const jPointLight*>(light);
			if (pointLight)
				lightPosOrDirection = pointLight->Data.Position;
			isOmniDirectional = true;
			break;
		}
		case ELightType::SPOT:
		{
			auto spotLight = static_cast<const jSpotLight*>(light);
			if (spotLight)
				lightPosOrDirection = spotLight->Data.Position;
			isOmniDirectional = true;
			break;
		}
		default:
			skip = true;
			break;
		}
		if (skip)
			continue;

		g_rhi->SetShader(shadowVolumeShader);
		jLight::BindLights({ light }, shadowVolumeShader);
		camera->BindCamera(shadowVolumeShader);

		g_rhi->SetClear(ERenderBufferType::STENCIL);
		g_rhi->SetStencilOpSeparate(EFace::FRONT, EStencilOp::KEEP, EStencilOp::DECR_WRAP, EStencilOp::KEEP);
		g_rhi->SetStencilOpSeparate(EFace::BACK, EStencilOp::KEEP, EStencilOp::INCR_WRAP, EStencilOp::KEEP);

		g_rhi->SetStencilFunc(EComparisonFunc::ALWAYS, 0, 0xff);
		g_rhi->SetDepthFunc(EComparisonFunc::LESS);
		g_rhi->SetDepthMask(false);
		g_rhi->SetColorMask(false, false, false, false);

		const_cast<jCamera*>(camera)->IsEnableCullMode = false;			// todo remove

		if (jShadowAppSettingProperties::GetInstance().ShadowOn)
		{
			// todo
			g_rhi->EnableDepthBias(true);
			g_rhi->SetDepthBias(0.0f, 100.0f);

			const auto& staticObjects = jObject::GetStaticObject();
			for (auto& iter : staticObjects)
			{
				auto shadowVolume = isGPUShadowVolume ? iter->ShadowVolumeGPU : iter->ShadowVolumeCPU;
				if (CanSkipShadowObject(shadowVolume, camera, iter, lightPosOrDirection, isOmniDirectional, light))
					continue;

				shadowVolume->Update(lightPosOrDirection, isOmniDirectional, iter);
				shadowVolume->Draw(jPipelineContext(pipelineContext.DefaultRenderTarget, pipelineContext.Objects, pipelineContext.Camera, { light }), shadowVolumeShader);
			}

			// todo
			// disable polygon offset fill
			g_rhi->EnableDepthBias(false);
		}

		//////////////////////////////////////////////////////////////////
		// 3. Final light(Directional, Point, Spot) rendering.
		g_rhi->SetStencilFunc(EComparisonFunc::EQUAL, 0, 0xff);
		g_rhi->SetStencilOpSeparate(EFace::FRONT, EStencilOp::KEEP, EStencilOp::KEEP, EStencilOp::KEEP);
		g_rhi->SetStencilOpSeparate(EFace::BACK, EStencilOp::KEEP, EStencilOp::KEEP, EStencilOp::KEEP);

		g_rhi->SetDepthMask(false);
		g_rhi->SetColorMask(true, true, true, true);
		const_cast<jCamera*>(camera)->IsEnableCullMode = true;			// todo remove

		g_rhi->SetDepthFunc(EComparisonFunc::EQUAL);
		g_rhi->SetBlendFunc(EBlendSrc::ONE, EBlendDest::ONE);

		g_rhi->SetShader(shadowVolumeBaseShader);
		jLight::BindLights({ light }, shadowVolumeBaseShader);
		camera->BindCamera(shadowVolumeBaseShader);
		const auto& staticObjects = jObject::GetStaticObject();
		for (auto& iter : staticObjects)
			iter->Draw(camera, shadowVolumeBaseShader, { light });
	}
	const_cast<jCamera*>(camera)->UseAmbient = true;			// todo remove

	g_rhi->EnableBlend(true);
	g_rhi->SetBlendFunc(EBlendSrc::SRC_ALPHA, EBlendDest::ONE_MINUS_SRC_ALPHA);
	g_rhi->SetDepthFunc(EComparisonFunc::LESS);
	g_rhi->SetDepthMask(true);
	g_rhi->SetColorMask(true, true, true, true);
	g_rhi->EnableStencil(false);

	// Debug
	{
		g_rhi->SetClear(ERenderBufferType::STENCIL);
		const_cast<jCamera*>(camera)->IsEnableCullMode = false;		// todo remove
		g_rhi->EnableBlend(true);
		g_rhi->SetBlendFunc(EBlendSrc::SRC_ALPHA, EBlendDest::ONE_MINUS_SRC_ALPHA);

		for (auto& light : camera->LightList)
		{
			bool isOmniDirectional = false;
			Vector lightPosOrDirection;

			bool skip = false;
			switch (light->Type)
			{
			case ELightType::DIRECTIONAL:
				if (!jShadowAppSettingProperties::GetInstance().ShowSilhouette_DirectionalLight)
					skip = true;

				lightPosOrDirection = static_cast<jDirectionalLight*>(light)->Data.Direction;
				break;
			case ELightType::POINT:
				if (!jShadowAppSettingProperties::GetInstance().ShowSilhouette_PointLight)
					skip = true;

				lightPosOrDirection = static_cast<jPointLight*>(light)->Data.Position;
				isOmniDirectional = true;
				break;
			case ELightType::SPOT:
				if (!jShadowAppSettingProperties::GetInstance().ShowSilhouette_SpotLight)
					skip = true;

				lightPosOrDirection = static_cast<jSpotLight*>(light)->Data.Position;
				isOmniDirectional = true;
				break;
			default:
				skip = true;
				break;
			}
			if (skip)
				continue;

			g_rhi->SetShader(shadowVolumeShader);
			camera->BindCamera(shadowVolumeShader);
			jLight::BindLights({ light }, shadowVolumeShader);

			const auto& staticObjects = jObject::GetStaticObject();
			for (auto& iter : staticObjects)
			{
				auto shadowVolume = isGPUShadowVolume ? iter->ShadowVolumeGPU : iter->ShadowVolumeCPU;
				if (CanSkipShadowObject(shadowVolume, camera, iter, lightPosOrDirection, isOmniDirectional, light))
					continue;

				shadowVolume->Update(lightPosOrDirection, isOmniDirectional, iter);
				shadowVolume->Draw(jPipelineContext(pipelineContext.DefaultRenderTarget, pipelineContext.Objects, pipelineContext.Camera, { light }), shadowVolumeShader);
			}
		}

		g_rhi->EnableBlend(false);
		const_cast<jCamera*>(camera)->IsEnableCullMode = false;			// todo remove
	}
}

//////////////////////////////////////////////////////////////////////////
// jForward_UIObject_Pipeline
void jForward_UIObject_Pipeline::Setup()
{
	EnableClear = false;
	EnableDepthTest = false;
	DepthStencilFunc = EComparisonFunc::LESS;
	EnableBlend = false;
	Shader = jShader::GetShader(ShaderName);
}

