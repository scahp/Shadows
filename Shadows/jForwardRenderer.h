﻿#pragma once
#include "jRHI.h"
#include "jRenderer.h"
#include "jGBuffer.h"
#include "jPipeline.h"

class jCamera;

class jForwardRenderer : public jRenderer
{
public:
	using jRenderer::jRenderer;
	virtual ~jForwardRenderer();

	typedef void (*RenderPassFunc)(jCamera*);

	virtual void Setup();
	virtual void Teardown();

	virtual void ShadowPrePass(const jCamera* camera) override;
	virtual void RenderPass(const jCamera* camera) override;
	virtual void DebugRenderPass(const jCamera* camera) override;
	virtual void BoundVolumeRenderPass(const jCamera* camera) override;
	virtual void PostProcessPass(const jCamera* camera) override;
	virtual void PostRenderPass(const jCamera* camera) override;
	virtual void DebugUIPass(const jCamera* camera) override;

	virtual void UpdateSettings() {}

	std::shared_ptr<jFrameBuffer> FrameBuffer;
	std::shared_ptr<jFrameBuffer> FrameBuffer2;
	std::shared_ptr<jFrameBuffer> LuminanceFrameBuffer;
	std::shared_ptr<jFrameBuffer> AvgLuminanceFrameBuffer;
	std::shared_ptr<jPostProcessInOutput> PostProcessInput;
};

