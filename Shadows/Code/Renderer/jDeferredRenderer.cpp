#include "pch.h"
#include "jDeferredRenderer.h"
#include "jRenderContext.h"
#include "jCamera.h"
#include "jObject.h"
#include "jRenderObject.h"
#include "jRenderTargetPool.h"
#include "jPrimitiveUtil.h"
#include "jSamplerStatePool.h"
#include "jImageFileLoader.h"

void jDeferredRenderer::Culling(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "Culling");
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
	SCOPE_DEBUG_EVENT(g_rhi, "DepthPrepass");
	if (DepthRTPtr->Begin())
	{
		g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		g_rhi->SetClear(ERenderBufferType::DEPTH);
		g_rhi->EnableDepthTest(true);
		g_rhi->SetDepthFunc(EComparisonFunc::LESS);

		jShader* shader = jShader::GetShader("NewDepthOnlyWithAlphaTest");
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

	{
		SCOPE_DEBUG_EVENT(g_rhi, "DepthPrepass_HiZ");
		g_rhi->SetDepthFunc(EComparisonFunc::ALWAYS);

		if (HiZRTPtr->Begin())
		{
			jMaterialData HiZCopyFromDepthMaterialData;
			HiZCopyFromDepthMaterialData.AddMaterialParam("TextureSampler", DepthRTPtr->GetTextureDepth(), jSamplerStatePool::GetSamplerState("Point").get());

			jShader* shader = jShader::GetShader("NewDepthCopy");
			g_rhi->SetShader(shader);

			int32 baseBindingIndex = g_rhi->SetMatetrial(&HiZCopyFromDepthMaterialData, shader);

			JASSERT(FullscreenQuad);
			FullscreenQuad->Draw(nullptr, shader, {});

			HiZRTPtr->End();
		}

		if (HiZRTPtr->Begin())
		{
			jMaterialData HiZMaterialData;
			HiZMaterialData.AddMaterialParam("DepthSampler", HiZRTPtr->GetTextureDepth(), jSamplerStatePool::GetSamplerState("Point").get());

			jShader* shader = jShader::GetShader("NewCreateHiZ");
			g_rhi->SetShader(shader);
			
			int32 baseBindingIndex = g_rhi->SetMatetrial(&HiZMaterialData, shader);

			Vector2 LastDepthSampleSize = Vector2(SCR_WIDTH, SCR_HEIGHT);
			Vector2 DepthSampleSize = Vector2(SCR_WIDTH / 2.0f, SCR_HEIGHT / 2.0f);
			const int32 numLevels = jTexture::GetMipLevels(HiZRTPtr->Info.Width, HiZRTPtr->Info.Height);
			for (int i = 1; i < numLevels; i++)
			{
				g_rhi->SetUniformbuffer("PrevMipLevel", i - 1, shader);

				Vector2 DepthRadio = LastDepthSampleSize / DepthSampleSize;
				g_rhi->SetUniformbuffer("DepthRadio", DepthRadio, shader);
				g_rhi->SetUniformbuffer("ScreenSize", LastDepthSampleSize, shader);

				HiZRTPtr->SetDepthMipLevel(i);		// Change Current Rendertarget's MipLevel

				JASSERT(FullscreenQuad);
				FullscreenQuad->Draw(nullptr, shader, {});

				LastDepthSampleSize = DepthSampleSize;
				DepthSampleSize.x = Max(1.0f, floor(DepthSampleSize.x / 2.0f));
				DepthSampleSize.y = Max(1.0f, floor(DepthSampleSize.y / 2.0f));
			}

			HiZRTPtr->SetDepthMipLevel(0);
			HiZRTPtr->End();
		}
		g_rhi->SetDepthFunc(EComparisonFunc::LESS);
	}
}

void jDeferredRenderer::ShowdowMap(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "ShowdowMap");

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
	SCOPE_DEBUG_EVENT(g_rhi, "GBuffer");
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
	{
		SCOPE_DEBUG_EVENT(g_rhi, "SSAO");

		if (SSAORTPtr->Begin())
		{
			g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
			g_rhi->SetClear(ERenderBufferType::COLOR);				// Depth is attached from DepthPrepass, so skip this.
			g_rhi->EnableDepthTest(false);

			jShader* shader = jShader::GetShader("NewSSAO");
			g_rhi->SetShader(shader);

			int32 baseBindingIndex = g_rhi->SetMatetrial(&GBufferMaterialData, shader);
			baseBindingIndex = g_rhi->SetMatetrial(&ShadowMaterialData, shader, baseBindingIndex);
			baseBindingIndex = g_rhi->SetMatetrial(&SSAOMaterialData, shader, baseBindingIndex);
			/*
			uniform vec3 samples[64];
			uniform mat4 Projection;
			*/
			auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
			auto loc = shader_gl->TryGetUniformLocation("samples");
			if (loc != -1)
			{
				const float* pKernel = (float*)ssaoKernel.data();
				glUniform3fv(loc, 64, pKernel);
			}

			InContext->Camera->BindCamera(shader);

			JASSERT(InContext);
			JASSERT(InContext->Camera);
			g_rhi->SetUniformbuffer("Projection", InContext->Camera->Projection, shader);

			JASSERT(FullscreenQuad);
			FullscreenQuad->Draw(nullptr, shader, {});

			SSAORTPtr->End();
		}
	}

	{
		SCOPE_DEBUG_EVENT(g_rhi, "SSAOBlurred");

		if (SSAORTBlurredPtr->Begin())
		{
			// Don't care : skipping color clear
			//g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
			//g_rhi->SetClear(ERenderBufferType::COLOR);				// Depth is attached from DepthPrepass, so skip this.
			g_rhi->EnableDepthTest(false);

			jShader* shader = jShader::GetShader("NewSSAOBlur");
			g_rhi->SetShader(shader);

			g_rhi->SetUniformbuffer("SSAO_Input_Width", (float)SSAORTBlurredPtr->Info.Width, shader);
			g_rhi->SetUniformbuffer("SSAO_Input_Height", (float)SSAORTBlurredPtr->Info.Height, shader);

			int32 baseBindingIndex = g_rhi->SetMatetrial(&SSAOBlurMaterialData, shader);

			JASSERT(FullscreenQuad);
			FullscreenQuad->Draw(nullptr, shader, {});

			SSAORTBlurredPtr->End();
		}
	}
}

void jDeferredRenderer::LightingPass(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "LightingPass");

	if (SceneColorRTPtr->Begin())
	{
		g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
		g_rhi->SetClear(ERenderBufferType::COLOR);				// Depth is attached from DepthPrepass, so skip this.
		g_rhi->EnableDepthTest(false);

		jShader* shader = jShader::GetShader("NewDeferredLighting");
		g_rhi->SetShader(shader);

		int32 baseBindingIndex = g_rhi->SetMatetrial(&GBufferMaterialData, shader);
		baseBindingIndex = g_rhi->SetMatetrial(&ShadowMaterialData, shader, baseBindingIndex);
		baseBindingIndex = g_rhi->SetMatetrial(&SSAOApplyMaterialData, shader, baseBindingIndex);

		const std::list<const jLight*>& Lights = InContext->Lights;

		jLight::BindLights(Lights, shader);

		g_rhi->SetUniformbuffer("Eye", InContext->Camera->Pos, shader);

		JASSERT(FullscreenQuad);
		FullscreenQuad->Draw(nullptr, shader, Lights);

		SceneColorRTPtr->End();
	}
}

void jDeferredRenderer::Tonemap(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "Tonemap");
}

void jDeferredRenderer::SSR(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "SSR");

	g_rhi->SetClearColor(Vector4(0.0f, 0.0f, 0.0f, 1.0f));
	g_rhi->SetClear(ERenderBufferType::COLOR);				// Depth is attached from DepthPrepass, so skip this.
	g_rhi->EnableDepthTest(false);

	//jShader* shader = jShader::GetShader("NewSSR_Linear");
	jShader* shader = jShader::GetShader("NewSSR_HiZ");
	g_rhi->SetShader(shader);

	int32 baseBindingIndex = g_rhi->SetMatetrial(&GBufferMaterialData, shader);
	baseBindingIndex = g_rhi->SetMatetrial(&SSRMaterialData, shader, baseBindingIndex);

	const std::list<const jLight*>& Lights = InContext->Lights;
	jLight::BindLights(Lights, shader);

	InContext->Camera->BindCamera(shader);

	Matrix InvProjection = InContext->Camera->Projection.GetInverse();

	//Vector4 Tmp = InvProjection.Transform(Vector4(0.75f, 0.75f, 0.75f, 1.0f));
	//Vector4 TmpDivZ = Tmp / Tmp.w;

	//Vector4 Tmp2 = InContext->Camera->Projection.Transform(Vector4(0.75f, 0.75f, 0.75f, 1.0f));
	//Vector4 Tmp2DivZ = Tmp2 / Tmp2.w;

	//Vector4 Tmp3 = InContext->Camera->View.Transform(Vector4(0.75f, 0.75f, 0.75f, 1.0f));
	//Vector4 Tmp3DivZ = Tmp3 / Tmp3.w;

	Vector2 ScreenSize(SCR_WIDTH, SCR_HEIGHT);
	g_rhi->SetUniformbuffer("InvP", InvProjection, shader);
	g_rhi->SetUniformbuffer("ScreenSize", ScreenSize, shader);

	g_rhi->SetUniformbuffer("Near", InContext->Camera->Near, shader);
	g_rhi->SetUniformbuffer("Far", InContext->Camera->Far, shader);

	JASSERT(FullscreenQuad);
	FullscreenQuad->Draw(nullptr, shader, Lights);
}

void jDeferredRenderer::AA(jRenderContext* InContext) const
{
	SCOPE_DEBUG_EVENT(g_rhi, "AA");
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

	jRenderTargetInfo HiZRTInfo = DepthRTInfo;
	HiZRTInfo.IsGenerateMipmapDepth = true;
	HiZRTPtr = jRenderTargetPool::GetRenderTarget(HiZRTInfo);

	// GBuffer RenderTarget
	jRenderTargetInfo GBufferRTInfo;
	GBufferRTInfo.TextureCount = 5;
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

	const auto& pPointClampToEdgeSamplerState = jSamplerStatePool::GetSamplerState("Point").get();
	GBufferMaterialData.AddMaterialParam("ColorSampler", GBufferRTPtr->Textures[0].get(), pPointClampToEdgeSamplerState);
	GBufferMaterialData.AddMaterialParam("NormalSampler", GBufferRTPtr->Textures[1].get(), pPointClampToEdgeSamplerState);	
	GBufferMaterialData.AddMaterialParam("PosSampler", GBufferRTPtr->Textures[2].get(), pPointClampToEdgeSamplerState);

	const auto& pShadowLinearSamplerState = jSamplerStatePool::GetSamplerState("LinearClampShadow").get();
	ShadowMaterialData.AddMaterialParam("DirectionalShadowSampler", ShadowRTPtr->GetTextureDepth(), pShadowLinearSamplerState);

	InitSSAO();

	// Debug quad
	DebugQuad = jPrimitiveUtil::CreateUIQuad(Vector2(100.0f, 100.0f), Vector2(100.0f, 100.0f)
		//, ShadowRTPtr->GetTextureDepth());
		//, SSAORTBlurredPtr->GetTexture());
		//, DepthRTPtr->GetTextureDepth());
		, HiZRTPtr->GetTextureDepth());
		//, SceneColorRTPtr->GetTexture());
		//, nullptr);
}

void jDeferredRenderer::Render(jRenderContext* InContext)
{
	SCOPE_DEBUG_EVENT(g_rhi, "Render");

	Culling(InContext);
	DepthPrepass(InContext);
	ShowdowMap(InContext);
	GBuffer(InContext);
	SSAO(InContext);
	LightingPass(InContext);
	Tonemap(InContext);
	SSR(InContext);
	AA(InContext);

	if (DebugQuad->GetTexture())
	{
		jShader* shader = jShader::GetShader("UIShader");
		g_rhi->SetShader(shader);
		DebugQuad->Size = Vector2(400.0f, 400.0f);
		DebugQuad->Pos = Vector2(SCR_WIDTH, SCR_HEIGHT) - DebugQuad->Size - Vector2(10.0f, 10.0f);
		DebugQuad->Draw(InContext->Camera, shader, {});
	}
}

void jDeferredRenderer::Release()
{
	delete FullscreenQuad;
	jRenderTargetPool::ReturnRenderTarget(ShadowRTPtr.get());
	jRenderTargetPool::ReturnRenderTarget(GBufferRTPtr.get());
	jRenderTargetPool::ReturnRenderTarget(DepthRTPtr.get());
}

void jDeferredRenderer::InitSSAO()
{
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
	std::default_random_engine generator;
	
	auto lerp = [](float a, float b, float f) { return a + f * (b - a); };

	for (uint32 i = 0; i < 64; ++i)
	{
		Vector sample(
			randomFloats(generator) * 2.0f - 1.0f, 
			randomFloats(generator) * 2.0f - 1.0f, 
			randomFloats(generator) ); 
		sample.SetNormalize();
		sample *= randomFloats(generator); 
		float scale = (float)i / 64.0f;

		// To make closer to origin position of hemisphere. we use x^2.
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;

		ssaoKernel.push_back(sample);
	}

	std::vector<Vector> ssaoNoise;
	for (uint32 i = 0; i < 16; i++)
	{ 
		Vector noise(
			randomFloats(generator) * 2.0f - 1.0f, 
			randomFloats(generator) * 2.0f - 1.0f, 
			0.0f); 
		ssaoNoise.push_back(noise); 
	}

	jImageData imageData;
	imageData.Width = 4;
	imageData.Height = 4;
	imageData.Format = ETextureFormat::RGB16F;
	imageData.FormatType = EFormatType::FLOAT;
	imageData.sRGB = false;
	imageData.Filename = "SSAONoise";
	imageData.ImageData.resize(imageData.Width * imageData.Height * sizeof(Vector));
	JASSERT(sizeof(float) * 3 == sizeof(Vector));
	memcpy(&imageData.ImageData[0], &ssaoNoise[0], ssaoNoise.size() * sizeof(Vector));

	SSAONoiseTex = std::shared_ptr<jTexture>(g_rhi->CreateTextureFromData(&imageData.ImageData[0], imageData.Width
		, imageData.Height, imageData.sRGB, imageData.FormatType, imageData.Format, false));

	const auto& pNoisePointWrapSamplerState = jSamplerStatePool::GetSamplerState("PointWrap").get();
	SSAOMaterialData.AddMaterialParam("TexNoise", SSAONoiseTex.get(), pNoisePointWrapSamplerState);

	const auto& pPointSamplerState = jSamplerStatePool::GetSamplerState("Point").get();
	SSAOMaterialData.AddMaterialParam("DepthSampler", DepthRTPtr->GetTextureDepth(), pPointSamplerState);

	jRenderTargetInfo SSAORTInfo;
	SSAORTInfo.TextureCount = 1;
	SSAORTInfo.TextureType = ETextureType::TEXTURE_2D;
	SSAORTInfo.InternalFormat = ETextureFormat::R32F;
	SSAORTInfo.Format = ETextureFormat::R;
	SSAORTInfo.FormatType = EFormatType::FLOAT;
	SSAORTInfo.DepthBufferType = EDepthBufferType::NONE;
	SSAORTInfo.Width = SCR_WIDTH;
	SSAORTInfo.Height = SCR_HEIGHT;
	SSAORTInfo.Magnification = ETextureFilter::NEAREST;
	SSAORTInfo.Minification = ETextureFilter::NEAREST;
	SSAORTPtr = jRenderTargetPool::GetRenderTarget(SSAORTInfo);
	SSAORTBlurredPtr = jRenderTargetPool::GetRenderTarget(SSAORTInfo);

	const auto& pLinearClamp = jSamplerStatePool::GetSamplerState("LinearClamp").get();
	SSAOBlurMaterialData.AddMaterialParam("SSAO_Input", SSAORTPtr->GetTexture(), pLinearClamp);

	SSAOApplyMaterialData.AddMaterialParam("SSAOSampler", SSAORTBlurredPtr->GetTexture(), pLinearClamp);

	jRenderTargetInfo SceneColorRTInfo;
	SceneColorRTInfo.TextureCount = 1;
	SceneColorRTInfo.TextureType = ETextureType::TEXTURE_2D;
	SceneColorRTInfo.InternalFormat = ETextureFormat::RGBA32F;
	SceneColorRTInfo.Format = ETextureFormat::RGBA;
	SceneColorRTInfo.FormatType = EFormatType::FLOAT;
	SceneColorRTInfo.DepthBufferType = EDepthBufferType::NONE;
	SceneColorRTInfo.Width = SCR_WIDTH;
	SceneColorRTInfo.Height = SCR_HEIGHT;
	SceneColorRTInfo.Magnification = ETextureFilter::NEAREST;
	SceneColorRTInfo.Minification = ETextureFilter::NEAREST;
	SceneColorRTPtr = jRenderTargetPool::GetRenderTarget(SceneColorRTInfo);
	
	SSRMaterialData.AddMaterialParam("DepthSampler", DepthRTPtr->GetTextureDepth(), pPointSamplerState);
	SSRMaterialData.AddMaterialParam("HiZSampler", HiZRTPtr->GetTextureDepth(), pPointSamplerState);
	SSRMaterialData.AddMaterialParam("SceneColorSampler", SceneColorRTPtr->GetTexture(), pPointSamplerState);
}

std::shared_ptr<jRenderTarget> jDeferredRenderer::GetDebugRTPtr() const
{
	static std::shared_ptr<jRenderTarget> DebugRTPtr;
	static bool IsInit = false;
	if (!IsInit)
	{
		jRenderTargetInfo DebugRTInfo;
		DebugRTInfo.TextureCount = 1;
		DebugRTInfo.TextureType = ETextureType::TEXTURE_2D;
		DebugRTInfo.InternalFormat = ETextureFormat::RGBA32F;
		DebugRTInfo.Format = ETextureFormat::RGBA;
		DebugRTInfo.FormatType = EFormatType::FLOAT;
		DebugRTInfo.DepthBufferType = EDepthBufferType::NONE;
		DebugRTInfo.Width = SCR_WIDTH;
		DebugRTInfo.Height = SCR_HEIGHT;
		DebugRTInfo.Magnification = ETextureFilter::NEAREST;
		DebugRTInfo.Minification = ETextureFilter::NEAREST;
		DebugRTPtr = jRenderTargetPool::GetRenderTarget(DebugRTInfo);
		IsInit = true;
	}
	return DebugRTPtr;
}
