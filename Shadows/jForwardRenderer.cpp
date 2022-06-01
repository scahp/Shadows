#include "pch.h"
#include "jForwardRenderer.h"
#include "jObject.h"
#include "jShadowAppProperties.h"
#include "jRenderTargetPool.h"

#include "jRHI_OpenGL.h"		// todo it should be removed.
#include "jCamera.h"
#include "jLight.h"
#include "jSamplerStatePool.h"

//////////////////////////////////////////////////////////////////////////
// jForwardRenderer
jForwardRenderer::~jForwardRenderer()
{
}

void jForwardRenderer::Setup()
{
	RenderTarget = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::RGBA16F, ETextureFormat::RGBA, EFormatType::FLOAT, EDepthBufferType::DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT, 1 }));

	PostProcessInput = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	PostProcessInput->RenderTarget = RenderTarget.get();

	//////////////////////////////////////////////////////////////////////////
	// Setup a postprocess chain

	// Luminance And Adaptive Luminance
	LuminanceRenderTarget = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R32F, ETextureFormat::R, EFormatType::FLOAT, EDepthBufferType::NONE, LUMINANCE_WIDTH, LUMINANCE_HEIGHT, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR_MIPMAP_LINEAR }));
	auto luminancePostProcessOutput = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	luminancePostProcessOutput->RenderTarget = LuminanceRenderTarget.get();
	{
		auto postprocess = new jPostProcess_LuminanceMapGeneration("LuminanceMapGeneration");
		postprocess->AddInput(PostProcessInput, jSamplerStatePool::GetSamplerState(jName("LinearClamp")));
		postprocess->SetOutput(luminancePostProcessOutput);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	auto avgLuminancePostProcessOutput = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	{
		auto postprocess = new jPostProcess_AdaptiveLuminance("AdaptiveLuminance");
		postprocess->AddInput(luminancePostProcessOutput, jSamplerStatePool::GetSamplerState(jName("PointMipmap")));
		postprocess->SetOutput(avgLuminancePostProcessOutput);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	// Bloom
	auto bloomThresdholdRT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R11G11B10F, ETextureFormat::RGB, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH, SCR_HEIGHT, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR }));
	auto bloomThresholdPostProcessOut = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	bloomThresholdPostProcessOut->RenderTarget = bloomThresdholdRT.get();
	{
		auto postprocess = new jPostProcess_BloomThreshold("BloomThreshold");
		postprocess->AddInput(PostProcessInput, jSamplerStatePool::GetSamplerState(jName("LinearClamp")));
		postprocess->AddInput(avgLuminancePostProcessOutput);
		postprocess->SetOutput(bloomThresholdPostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	auto scale1_RT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R11G11B10F, ETextureFormat::RGB, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH / 2, SCR_HEIGHT / 2, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR }));
	auto scale1_PostProcessOut = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	scale1_PostProcessOut->RenderTarget = scale1_RT.get();
	{
		auto postprocess = new jPostProcess_Scale("Bloom Downscale");
		postprocess->AddInput(bloomThresholdPostProcessOut, jSamplerStatePool::GetSamplerState(jName("LinearClamp")));
		postprocess->SetOutput(scale1_PostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	auto scale2_RT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R11G11B10F, ETextureFormat::RGB, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH / 4, SCR_HEIGHT / 4, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR }));
	auto scale2_PostProcessOut = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	scale2_PostProcessOut->RenderTarget = scale2_RT.get();
	{
		auto postprocess = new jPostProcess_Scale("Bloom Downscale");
		postprocess->AddInput(scale1_PostProcessOut, jSamplerStatePool::GetSamplerState(jName("LinearClamp")));
		postprocess->SetOutput(scale2_PostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	auto scale3_RT = std::shared_ptr<jRenderTarget>(jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R11G11B10F, ETextureFormat::RGB, EFormatType::FLOAT, EDepthBufferType::NONE, SCR_WIDTH / 8, SCR_HEIGHT / 8, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR }));
	auto scale3_PostProcessOut = std::shared_ptr<jPostProcessInOutput>(new jPostProcessInOutput());
	scale3_PostProcessOut->RenderTarget = scale3_RT.get();
	{
		auto postprocess = new jPostProcess_Scale("Bloom Downscale");
		postprocess->AddInput(scale2_PostProcessOut, jSamplerStatePool::GetSamplerState(jName("LinearClamp")));
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
				postprocess->AddInput(scale3_PostProcessOut, jSamplerStatePool::GetSamplerState(jName("Point")));
				postprocess->SetOutput(gaussianBlur_PostProcessOut);
				PostProcessChain.AddNewPostprocess(postprocess);
			}

			{
				auto postprocess = new jPostProcess_GaussianBlurV("GaussianBlurV");
				postprocess->AddInput(gaussianBlur_PostProcessOut, jSamplerStatePool::GetSamplerState(jName("Point")));
				postprocess->SetOutput(scale3_PostProcessOut);
				PostProcessChain.AddNewPostprocess(postprocess);
			}
		}
	}

	{
		auto postprocess = new jPostProcess_Scale("Bloom Upscale");
		postprocess->AddInput(scale3_PostProcessOut, jSamplerStatePool::GetSamplerState(jName("LinearClamp")));
		postprocess->SetOutput(scale2_PostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	{
		auto postprocess = new jPostProcess_Scale("Bloom Upscale");
		postprocess->AddInput(scale2_PostProcessOut, jSamplerStatePool::GetSamplerState(jName("LinearClamp")));
		postprocess->SetOutput(scale1_PostProcessOut);
		PostProcessChain.AddNewPostprocess(postprocess);
	}

	// ToneMapAndBloom
	{
		auto postprocess = new jPostProcess_Tonemap("TonemapAndBloom");
		postprocess->AddInput(PostProcessInput, jSamplerStatePool::GetSamplerState(jName("Point")));
		postprocess->AddInput(avgLuminancePostProcessOutput);
		postprocess->AddInput(scale1_PostProcessOut, jSamplerStatePool::GetSamplerState(jName("Linear")));
		postprocess->SetOutput(nullptr);
		PostProcessChain.AddNewPostprocess(postprocess);
	}
}

void jForwardRenderer::Teardown()
{
	
}

void jForwardRenderer::ShadowPrePass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "ShadowPrePass");
	SCOPE_PROFILE(ShadowPrePass);
	SCOPE_GPU_PROFILE(ShadowPrePass);

	std::list<const jLight*> lights;
	lights.insert(lights.end(), camera->LightList.begin(), camera->LightList.end());
	const jPipelineContext data(RenderTarget.get(), jObject::GetShadowCasterObject(), camera, lights);

	for (auto& iter : PipelineSet->ShadowPrePass)
		iter->Do(data);
}

void jForwardRenderer::RenderPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "RenderPass");
	SCOPE_PROFILE(RenderPass);
	SCOPE_GPU_PROFILE(RenderPass);

	std::list<const jLight*> lights;
	lights.insert(lights.end(), camera->LightList.begin(), camera->LightList.end());
	const jPipelineContext data(RenderTarget.get(), jObject::GetStaticObject(), camera, lights);

	for (auto& iter : PipelineSet->RenderPass)
		iter->Do(data);
}

void jForwardRenderer::DebugRenderPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "DebugRenderPass");
	SCOPE_PROFILE(DebugRenderPass);
	SCOPE_GPU_PROFILE(DebugRenderPass);

	const jPipelineContext data(RenderTarget.get(), jObject::GetDebugObject(), camera, {});
	for (auto& iter : PipelineSet->DebugRenderPass)
		iter->Do(data);
}

void jForwardRenderer::BoundVolumeRenderPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "BoundVolumeRenderPass");
	SCOPE_PROFILE(BoundVolumeRenderPass);
	SCOPE_GPU_PROFILE(BoundVolumeRenderPass);

	if (jShadowAppSettingProperties::GetInstance().ShowBoundBox)
	{
		const jPipelineContext data(RenderTarget.get(), jObject::GetBoundBoxObject(), camera, {});
		for (auto& iter : PipelineSet->BoundVolumeRenderPass)
			iter->Do(data);
	}

	if (jShadowAppSettingProperties::GetInstance().ShowBoundSphere)
	{
		const jPipelineContext data(RenderTarget.get(), jObject::GetBoundSphereObject(), camera, {});
		for (auto& iter : PipelineSet->BoundVolumeRenderPass)
			iter->Do(data);
	}
}

void jForwardRenderer::PostProcessPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "PostProcessPass");
	SCOPE_PROFILE(PostProcessPass);
	SCOPE_GPU_PROFILE(PostProcessPass);
	PostProcessChain.Process(camera);
}

void jForwardRenderer::PostRenderPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "PostRenderPass");
	SCOPE_PROFILE(PostRenderPass);
	SCOPE_GPU_PROFILE(PostRenderPass);
	for (auto& iter : PipelineSet->PostRenderPass)
		iter->Do({});
}

void jForwardRenderer::DebugUIPass(const jCamera* camera)
{
	SCOPE_DEBUG_EVENT(g_rhi, "DebugUIPass");
	SCOPE_PROFILE(DebugUIPass);
	SCOPE_GPU_PROFILE(DebugUIPass);
	const jPipelineContext data(RenderTarget.get(), jObject::GetUIDebugObject(), camera, {});
	for (auto& iter : PipelineSet->DebugUIPass)
		iter->Do(data);
}
