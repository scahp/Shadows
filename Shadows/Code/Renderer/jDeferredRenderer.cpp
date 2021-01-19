#include "pch.h"
#include "jDeferredRenderer.h"
#include "jRenderContext.h"
#include "jCamera.h"
#include "jObject.h"
#include "jRenderObject.h"
#include "jRenderTargetPool.h"
#include "jPrimitiveUtil.h"
#include "jSamplerStatePool.h"

void jDeferredRenderer::Culling(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "jDeferredRenderer::Culling");
	JASSERT(InContext);

	InContext->ResetVisibleArray();

	const jCamera* CurCamera = InContext->Camera;
	JASSERT(CurCamera);

	// Furustum culling
	const int32 ObjectCount = (int32)InContext->AllObjects.size();
	for (int32 i = 0; i < ObjectCount; ++i)
	{
		const jObject* obj = InContext->AllObjects[i];
		JASSERT(obj);

		const bool IsInFrustum = CurCamera->Frustum.IsInFrustum(obj->RenderObject->GetPos(), obj->BoundSphere.Radius);
		if (IsInFrustum)
			InContext->Visibles[i] = 1;
	}
}

void jDeferredRenderer::DepthPrepass(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "jDeferredRenderer::DepthPrepass");
	if (DepthRTPtr->Begin())
	{
		g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		g_rhi->SetClear(ERenderBufferType::DEPTH);
		g_rhi->EnableDepthTest(true);
		g_rhi->SetDepthFunc(EComparisonFunc::LESS);

		jShader* shader = jShader::GetShader("NewDepthOnly");
		g_rhi->SetShader(shader);

		const jCamera* CurCamera = InContext->Camera;
		JASSERT(CurCamera);

		const std::list<const jLight*>& Lights = InContext->Lights;

		const int32 ObjectCount = (int32)InContext->AllObjects.size();
		for (int32 i = 0; i < ObjectCount; ++i)
		{
			const jObject* obj = InContext->AllObjects[i];
			JASSERT(obj);

			obj->Draw(CurCamera, shader, Lights);
		}

		DepthRTPtr->End();
	}
}

void jDeferredRenderer::ShowdowMap(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "jDeferredRenderer::ShowdowMap");

	if (ShadowRTPtr->Begin())
	{
		g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);
		g_rhi->EnableDepthTest(true);
		g_rhi->SetDepthFunc(EComparisonFunc::LESS);

		if (!InContext->Lights.empty())
		{
			// todo : apply all type of shadow
			const jLight* light = *InContext->Lights.begin();
			JASSERT(light->Type == ELightType::DIRECTIONAL);

			std::list<const jLight*> CurLights{ light };

			jShader* shader = jShader::GetShader("NewSSM");
			g_rhi->SetShader(shader);

			const jCamera* CurCamera = light->GetLightCamra();
			JASSERT(CurCamera);

			const int32 ObjectCount = (int32)InContext->AllObjects.size();
			for (int32 i = 0; i < ObjectCount; ++i)
			{
				const jObject* obj = InContext->AllObjects[i];
				JASSERT(obj);

				obj->Draw(CurCamera, shader, CurLights);
			}
		}
		ShadowRTPtr->End();
	}
}

void jDeferredRenderer::GBuffer(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "jDeferredRenderer::GBuffer");
	if (GBufferRTPtr->Begin())
	{
		g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		g_rhi->SetClear(ERenderBufferType::COLOR);				// Depth is attached from DepthPrepass, so skip this.
		g_rhi->EnableDepthTest(true);
		g_rhi->SetDepthFunc(EComparisonFunc::EQUAL);

		jShader* shader = jShader::GetShader("NewDeferred");
		g_rhi->SetShader(shader);

		const jCamera* CurCamera = InContext->Camera;
		JASSERT(CurCamera);

		const std::list<const jLight*>& Lights = InContext->Lights;

		const int32 ObjectCount = (int32)InContext->AllObjects.size();
		for (int32 i = 0; i < ObjectCount; ++i)
		{
			const jObject* obj = InContext->AllObjects[i];
			JASSERT(obj);

			obj->Draw(CurCamera, shader, Lights);
		}

		GBufferRTPtr->End();
	}
}

void jDeferredRenderer::SSAO(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "jDeferredRenderer::SSAO");
}

void jDeferredRenderer::LightingPass(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "jDeferredRenderer::LightingPass");

	g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	g_rhi->SetClear(ERenderBufferType::COLOR);				// Depth is attached from DepthPrepass, so skip this.
	g_rhi->EnableDepthTest(false);

	jShader* shader = jShader::GetShader("NewDeferredLighting");
	g_rhi->SetShader(shader);

	int32 baseBindingIndex = g_rhi->SetMatetrial(&GBufferMaterialData, shader);
	baseBindingIndex = g_rhi->SetMatetrial(&ShadowMaterialData, shader, baseBindingIndex);

	const std::list<const jLight*>& Lights = InContext->Lights;

	jLight::BindLights(Lights, shader);

	g_rhi->SetUniformbuffer("Eye", InContext->Camera->Pos, shader);

	JASSERT(FullscreenQuad);
	FullscreenQuad->Draw(nullptr, shader, Lights);
}

void jDeferredRenderer::Tonemap(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "jDeferredRenderer::Tonemap");
}

void jDeferredRenderer::SSR(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "jDeferredRenderer::SSR");
}

void jDeferredRenderer::AA(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "jDeferredRenderer::AA");
}

void jDeferredRenderer::Init()
{
	// Depth only framebuffer
	jRenderTargetInfo DepthRTInfo;
	DepthRTInfo.TextureCount = 0;		// No color attachment
	DepthRTInfo.TextureType = ETextureType::TEXTURE_2D;
	DepthRTInfo.DepthBufferType = EDepthBufferType::DEPTH16;
	DepthRTInfo.Width = SCR_WIDTH;
	DepthRTInfo.Height = SCR_HEIGHT;
	DepthRTInfo.Magnification = ETextureFilter::NEAREST;
	DepthRTInfo.Minification = ETextureFilter::NEAREST;
	DepthRTPtr = jRenderTargetPool::GetRenderTarget(DepthRTInfo);

	// GBuffer RenderTarget
	jRenderTargetInfo GBufferRTInfo;
	GBufferRTInfo.TextureCount = 3;
	GBufferRTInfo.TextureType = ETextureType::TEXTURE_2D;
	GBufferRTInfo.InternalFormat = ETextureFormat::RGBA32F;
	GBufferRTInfo.Format = ETextureFormat::RGBA;
	GBufferRTInfo.FormatType = EFormatType::FLOAT;
	GBufferRTInfo.DepthBufferType = EDepthBufferType::NONE;
	GBufferRTInfo.Width = SCR_WIDTH;
	GBufferRTInfo.Height = SCR_HEIGHT;
	GBufferRTInfo.Magnification = ETextureFilter::NEAREST;
	GBufferRTInfo.Minification = ETextureFilter::NEAREST;
	GBufferRTPtr = jRenderTargetPool::GetRenderTarget(GBufferRTInfo);

	GBufferRTPtr->SetDepthAttachment(DepthRTPtr->TextureDepth);

	// ShadowMap RenderTarget
	jRenderTargetInfo ShadowRTInfo;
	ShadowRTInfo.TextureCount = 0;		// No color attachment
	ShadowRTInfo.TextureType = ETextureType::TEXTURE_2D;
	ShadowRTInfo.DepthBufferType = EDepthBufferType::DEPTH16;
	ShadowRTInfo.Width = SM_WIDTH;
	ShadowRTInfo.Height = SM_HEIGHT;
	ShadowRTInfo.Magnification = ETextureFilter::NEAREST;
	ShadowRTInfo.Minification = ETextureFilter::NEAREST;
	ShadowRTPtr = jRenderTargetPool::GetRenderTarget(ShadowRTInfo);

	// Fullscreen Quad Object
	FullscreenQuad = jPrimitiveUtil::CreateFullscreenQuad(nullptr);

	GBufferMaterialData.AddMaterialParam("ColorSampler", GBufferRTPtr->Textures[0].get());
	GBufferMaterialData.AddMaterialParam("NormalSampler", GBufferRTPtr->Textures[1].get());
	GBufferMaterialData.AddMaterialParam("PosInWorldSampler", GBufferRTPtr->Textures[2].get());

	const auto& pShadowLinearSamplerState = jSamplerStatePool::GetSamplerState("LinearClampShadow").get();
	ShadowMaterialData.AddMaterialParam("DirectionalShadowSampler", ShadowRTPtr->GetTextureDepth(), pShadowLinearSamplerState);

	// Debug quad
	DebugQuad = jPrimitiveUtil::CreateUIQuad(Vector2(100.0f, 100.0f), Vector2(100.0f, 100.0f), ShadowRTPtr->GetTextureDepth());
}

void jDeferredRenderer::Render(jRenderContext* InContext)
{
	SCOPE_DEBUG_EVENT(g_rhi, "jDeferredRenderer::Render");

	Culling(InContext);
	DepthPrepass(InContext);
	ShowdowMap(InContext);
	GBuffer(InContext);
	SSAO(InContext);
	LightingPass(InContext);
	Tonemap(InContext);
	SSR(InContext);
	AA(InContext);

	jShader* shader = jShader::GetShader("UIShader");
	g_rhi->SetShader(shader);
	DebugQuad->Size = Vector2(200.0f, 200.0f);
	DebugQuad->Pos = Vector2(SCR_WIDTH, SCR_HEIGHT) - DebugQuad->Size - Vector2(10.0f, 10.0f);
	DebugQuad->Draw(InContext->Camera, shader, {});
}

void jDeferredRenderer::Release()
{
	delete FullscreenQuad;
	jRenderTargetPool::ReturnRenderTarget(ShadowRTPtr.get());
	jRenderTargetPool::ReturnRenderTarget(GBufferRTPtr.get());
	jRenderTargetPool::ReturnRenderTarget(DepthRTPtr.get());
}
