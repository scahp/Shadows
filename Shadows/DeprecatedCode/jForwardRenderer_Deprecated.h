#pragma once
#include "jRHI.h"
#include "jRenderer.h"
#include "jGBuffer.h"
#include "jPipeline.h"

class jCamera;

class jForwardRenderer_Deprecated : public jRenderer
{
public:
	using jRenderer::jRenderer;
	virtual ~jForwardRenderer_Deprecated();

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

	std::shared_ptr<jRenderTarget> RenderTarget;
	std::shared_ptr<jRenderTarget> RenderTarget2;
	std::shared_ptr<jRenderTarget> LuminanceRenderTarget;
	std::shared_ptr<jRenderTarget> AvgLuminanceRenderTarget;
	std::shared_ptr<jPostProcessInOutput> PostProcessInput;
};

