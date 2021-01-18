#pragma once
#include "jRHI.h"
#include "jRenderer.h"
#include "jGBuffer.h"
#include "jPipeline.h"

class jCamera;

class jDeferredRenderer_Deprecated : public jRenderer
{
public:
	jDeferredRenderer_Deprecated(const jRenderTargetInfo& geometryBufferInfo);
	virtual ~jDeferredRenderer_Deprecated();

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
	jRenderTargetInfo GeometryBufferInfo;
	jPipelineSet* DeferredDeepShadowMapPipelineSet = nullptr;
	std::shared_ptr<jRenderTarget> LuminanceRenderTarget;
	std::shared_ptr<jRenderTarget> OutRenderTarget;
	std::shared_ptr<jRenderTarget> PostPrceoss_AA_DeepShadowAddition;
	std::shared_ptr<jPostProcessInOutput> PostProcessOutput;
	std::shared_ptr<jPostProcessInOutput> PostProcessOutput2;
	std::shared_ptr<jPostProcessInOutput> PostProcessLuminanceOutput;	
};

