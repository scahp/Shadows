﻿#pragma once
#include "jRHI.h"
#include "jRenderer.h"
#include "jGBuffer.h"
#include "jPipeline.h"

class jCamera;

class jDeferredRenderer : public jRenderer
{
public:
	jDeferredRenderer(const jFrameBufferInfo& geometryBufferInfo);
	virtual ~jDeferredRenderer();

	typedef void (*RenderPassFunc)(jCamera*);

	virtual void Setup();
	virtual void Teardown();

	virtual void ShadowPrePass(const jCamera* camera) override;
	virtual void RenderPass(const jCamera* camera) override;
	virtual void DebugRenderPass(const jCamera* camera) override;
	virtual void BoundVolumeRenderPass(const jCamera* camera) override;
	virtual void PostProcessPass(const jCamera* camera) override;
	virtual void PostRenderPass(const jCamera* camera) override;

	virtual void UpdateSettings() {}

private:
	jGBuffer GBuffer;
	jFrameBufferInfo GeometryBufferInfo;
	jPipelineSet* DeferredDeepShadowMapPipelineSet = nullptr;
	std::shared_ptr<jFrameBuffer> LuminanceFrameBuffer;
	std::shared_ptr<jFrameBuffer> OutFrameBuffer;
	std::shared_ptr<jFrameBuffer> PostPrceoss_AA_DeepShadowAddition;
	std::shared_ptr<jPostProcessInOutput> PostProcessOutput;
	std::shared_ptr<jPostProcessInOutput> PostProcessOutput2;
	std::shared_ptr<jPostProcessInOutput> PostProcessLuminanceOutput;	
};

