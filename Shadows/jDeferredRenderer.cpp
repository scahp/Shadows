#include "pch.h"
#include "jDeferredRenderer.h"
#include "jObject.h"
#include "jShadowAppProperties.h"
#include "jRenderTargetPool.h"

#include "jRHI_OpenGL.h"		// todo it should be removed.
#include "jCamera.h"
#include "jLight.h"
#include "jSamplerStatePool.h"

//////////////////////////////////////////////////////////////////////////
// jDeferredRenderer
jDeferredRenderer::jDeferredRenderer(const jRenderTargetInfo& geometryBufferInfo)
{
	GeometryBufferInfo = geometryBufferInfo;
}

jDeferredRenderer::~jDeferredRenderer()
{
}

void jDeferredRenderer::Setup()
{
	GBuffer.GeometryBuffer = jRenderTargetPool::GetRenderTarget(GeometryBufferInfo);

	DeferredDeepShadowMapPipelineSet = new jDeferredDeepShadowMapPipelineSet(&GBuffer);
	DeferredDeepShadowMapPipelineSet->Setup();

	SetChangePipelineSet(DeferredDeepShadowMapPipelineSet);

	////////////////////////////////////////////////////////////////////////////
	//// Setup a postprocess chain

	//jRenderTargetInfo info = jRenderTargetInfo(ETextureType::TEXTURE_2D, ETextureFormat::RGBA32F, ETextureFormat::RGBA, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH, SCR_HEIGHT, 1, ETextureFilter::NEAREST, ETextureFilter::NEAREST, false, 1);
	//static auto ResolveTarget = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget(info));
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<jTexture_OpenGL*>(GBuffer.GeometryBuffer->GetTexture(0))->TextureID);
	//glReadBuffer(GL_COLOR_ATTACHMENT0);

	//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<jTexture_OpenGL*>(ResolveTarget->GetTexture())->TextureID);
	//glDrawBuffer(GL_COLOR_ATTACHMENT0);

	//glDisable(GL_DEPTH_TEST);
	//glDisable(GL_STENCIL_TEST);
	//glBlitFramebuffer(0, 0, SCR_WIDTH, SCR_HEIGHT, 0, 0, SCR_WIDTH, SCR_HEIGHT, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	
	// Render DeepShadow
	OutRenderTarget = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::RGBA32F, ETextureFormat::RGBA, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH, SCR_HEIGHT, 1 }));
	PostProcessOutput = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	PostProcessOutput->RenderTarget = OutRenderTarget.get();
	{
		// this postprocess have to have DeepShadowMap PipelineSet.
		JASSERT(PipelineSet->GetType() == EPipelineSetType::DeepShadowMap);
		auto deferredDeepShadowMapPipelineSet = static_cast<jDeferredDeepShadowMapPipelineSet*>(PipelineSet);
		auto& DeepShadowMapBuffers = deferredDeepShadowMapPipelineSet->DeepShadowMapBuffers;

		// LightPass of DeepShadowMap
		auto postprocess = new jPostProcess_DeepShadowMap("DeepShadow", { DeepShadowMapBuffers.StartElementBuf, DeepShadowMapBuffers.LinkedListEntryDepthAlphaNext, DeepShadowMapBuffers.LinkedListEntryNeighbors }
		, &GBuffer);
		postprocess->SetOutput(PostProcessOutput);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	//// Anti-Aliasing for DeepShadow
	//{
	//	PostPrceoss_AA_DeepShadowAddition = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::RGBA32F, ETextureFormat::RGBA, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH, SCR_HEIGHT, 1 }));
	//	PostProcessOutput2 = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	//	PostProcessOutput2->RenderTarget = PostPrceoss_AA_DeepShadowAddition.get();

	//	auto postprocess = new jPostProcess_AA_DeepShadowAddition("AA_DeepShadowAddition");
	//	postprocess->AddInput(PostProcessOutput);
	//	postprocess->SetOutput(PostProcessOutput2);
	//	PostProcessChain.AddNewPostprocess(postprocess);
	//}

	PostProcessOutput2 = PostProcessOutput;

	// Luminance And Adaptive Luminance
	{
		LuminanceRenderTarget = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R32F, ETextureFormat::R, EFormatType::FLOAT, EDepthBufferType::NONE, LUMINANCE_WIDTH, LUMINANCE_HEIGHT, 1 }));
		PostProcessLuminanceOutput = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
		PostProcessLuminanceOutput->RenderTarget = LuminanceRenderTarget.get();

		auto postprocess = new jPostProcess_LuminanceMapGeneration("LuminanceMapGeneration");
		postprocess->AddInput(PostProcessOutput2, jSamplerStatePool::GetSamplerState("LinearClamp"));
		postprocess->SetOutput(PostProcessLuminanceOutput);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	auto avgLuminancePostProcessOutput = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	{
		auto postprocess = new jPostProcess_AdaptiveLuminance("AdaptiveLuminance");
		postprocess->AddInput(PostProcessLuminanceOutput, jSamplerStatePool::GetSamplerState("PointMipmap"));
		postprocess->SetOutput(avgLuminancePostProcessOutput);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	// Bloom
	auto bloomThresdholdRT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R11G11B10F, ETextureFormat::RGB, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH, SCR_HEIGHT, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR }));
	auto bloomThresholdPostProcessOut = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	bloomThresholdPostProcessOut->RenderTarget = bloomThresdholdRT.get();
	{
		auto postprocess = new jPostProcess_BloomThreshold("BloomThreshold");
		postprocess->AddInput(PostProcessOutput2, jSamplerStatePool::GetSamplerState("LinearClamp"));
		postprocess->AddInput(avgLuminancePostProcessOutput);
		postprocess->SetOutput(bloomThresholdPostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	auto scale1_RT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R11G11B10F, ETextureFormat::RGB, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH / 2, SCR_HEIGHT / 2, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR }));
	auto scale1_PostProcessOut = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	scale1_PostProcessOut->RenderTarget = scale1_RT.get();
	{
		auto postprocess = new jPostProcess_Scale("Bloom Downscale");
		postprocess->AddInput(bloomThresholdPostProcessOut, jSamplerStatePool::GetSamplerState("LinearClamp"));
		postprocess->SetOutput(scale1_PostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	auto scale2_RT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R11G11B10F, ETextureFormat::RGB, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH / 4, SCR_HEIGHT / 4, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR }));
	auto scale2_PostProcessOut = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	scale2_PostProcessOut->RenderTarget = scale2_RT.get();
	{
		auto postprocess = new jPostProcess_Scale("Bloom Downscale");
		postprocess->AddInput(scale1_PostProcessOut, jSamplerStatePool::GetSamplerState("LinearClamp"));
		postprocess->SetOutput(scale2_PostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	auto scale3_RT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R11G11B10F, ETextureFormat::RGB, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH / 8, SCR_HEIGHT / 8, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR }));
	auto scale3_PostProcessOut = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	scale3_PostProcessOut->RenderTarget = scale3_RT.get();
	{
		auto postprocess = new jPostProcess_Scale("Bloom Downscale");
		postprocess->AddInput(scale2_PostProcessOut, jSamplerStatePool::GetSamplerState("LinearClamp"));
		postprocess->SetOutput(scale3_PostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	auto gaussianBlurRT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R11G11B10F, ETextureFormat::RGB, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH / 8, SCR_HEIGHT / 8, 1 }));
	auto gaussianBlur_PostProcessOut = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	gaussianBlur_PostProcessOut->RenderTarget = gaussianBlurRT.get();
	{
		for (int i = 0; i < 4; ++i)
		{
			{
				auto postprocess = new jPostProcess_GaussianBlurH("GaussianBlurH");
				postprocess->AddInput(scale3_PostProcessOut, jSamplerStatePool::GetSamplerState("Point"));
				postprocess->SetOutput(gaussianBlur_PostProcessOut);
				PostProcessChain.AddNewPostprocess(postprocess);
			}

			{
				auto postprocess = new jPostProcess_GaussianBlurV("GaussianBlurV");
				postprocess->AddInput(gaussianBlur_PostProcessOut, jSamplerStatePool::GetSamplerState("Point"));
				postprocess->SetOutput(scale3_PostProcessOut);
				PostProcessChain.AddNewPostprocess(postprocess);
			}
		}
	}

	{
		auto postprocess = new jPostProcess_Scale("Bloom Upscale");
		postprocess->AddInput(scale3_PostProcessOut, jSamplerStatePool::GetSamplerState("LinearClamp"));
		postprocess->SetOutput(scale2_PostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	{
		auto postprocess = new jPostProcess_Scale("Bloom Upscale");
		postprocess->AddInput(scale2_PostProcessOut, jSamplerStatePool::GetSamplerState("LinearClamp"));
		postprocess->SetOutput(scale1_PostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	// ToneMapAndBloom
	{
		auto postprocess = new jPostProcess_Tonemap("ToneMapAndBloom");
		postprocess->AddInput(PostProcessOutput2, jSamplerStatePool::GetSamplerState("Point"));
		postprocess->AddInput(avgLuminancePostProcessOutput);
		postprocess->AddInput(scale1_PostProcessOut, jSamplerStatePool::GetSamplerState("Linear"));
		postprocess->SetOutput(nullptr);
		PostProcessChain.AddNewPostprocess(postprocess);
	}
}

void jDeferredRenderer::Teardown()
{
	
}

void jDeferredRenderer::ShadowPrePass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "ShadowPrePass");

	const auto directionalLight = camera->GetLight(ELightType::DIRECTIONAL);
	std::list<const jLight*> lights;
	if (directionalLight)
		lights.push_back(directionalLight);

	// todo Directional Light 만 쓸건가?
	const jPipelineContext data(nullptr, jObject::GetShadowCasterObject(), camera, lights);

	for (auto& iter : PipelineSet->ShadowPrePass)
		iter->Do(data);
}

void jDeferredRenderer::RenderPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "RenderPass");

	const auto directionalLight = camera->GetLight(ELightType::DIRECTIONAL);

	std::list<const jLight*> lights;
	if (directionalLight)
		lights.push_back(directionalLight);

	// Geometry Pass
	// todo Directional Light 만 쓸건가?
	const jPipelineContext data(nullptr, jObject::GetStaticObject(), camera, lights);

	for (auto& iter : PipelineSet->RenderPass)
		iter->Do(data);
}

void jDeferredRenderer::DebugRenderPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "DebugRenderPass");

	if (GBuffer.Begin())
	{
		const jPipelineContext data(nullptr, jObject::GetDebugObject(), camera, {});
		for (auto& iter : PipelineSet->DebugRenderPass)
			iter->Do(data);
		GBuffer.End();
	}
}

void jDeferredRenderer::BoundVolumeRenderPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "BoundVolumeRenderPass");

	if (GBuffer.Begin())
	{
		if (jShadowAppSettingProperties::GetInstance().ShowBoundBox)
		{
			const jPipelineContext data(nullptr, jObject::GetBoundBoxObject(), camera, {});
			for (auto& iter : PipelineSet->BoundVolumeRenderPass)
				iter->Do(data);
		}

		if (jShadowAppSettingProperties::GetInstance().ShowBoundSphere)
		{
			const jPipelineContext data(nullptr, jObject::GetBoundSphereObject(), camera, {});
			for (auto& iter : PipelineSet->BoundVolumeRenderPass)
				iter->Do(data);
		}
		GBuffer.End();
	}
}

void jDeferredRenderer::PostProcessPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "PostProcessPass");
	PostProcessChain.Process(camera);
}

void jDeferredRenderer::PostRenderPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "PostRenderPass");
	for (auto& iter : PipelineSet->PostRenderPass)
		iter->Do({});
}
