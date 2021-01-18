#pragma once
#include "jRHI.h"
#include "jPostProcess.h"

class jCamera;
class jPipelineSet;

class jRenderer
{
public:
	jRenderer() {}
	jRenderer(jPipelineSet* pipelineSet)
		: PipelineSet(pipelineSet)
	{}
	virtual ~jRenderer() {}

	typedef void (*RenderPassFunc)(jCamera*);

	virtual void Setup() {}
	virtual void Teardown() {}
	virtual void Reset() {}

	virtual void ShadowPrePass(const jCamera* camera) {}
	virtual void DebugShadowPrePass(const jCamera* camera) {}

	virtual void RenderPass(const jCamera* camera) {}
	virtual void PostRenderPass(const jCamera* camera) {}
	virtual void DebugRenderPass(const jCamera* camera) {}
	virtual void BoundVolumeRenderPass(const jCamera* camera) {}
	
	virtual void PostProcessPass(const jCamera* camera) {}
	virtual void DebugPostProcessPass(const jCamera* camera) {}
	
	virtual void UIPass(const jCamera* camera) {}
	virtual void DebugUIPass(const jCamera* camera) {}

	virtual void Render(const jCamera* camera);
	virtual void UpdateSettings() {}

	virtual void SetChangePipelineSet(jPipelineSet* newPipelineSet);

	jPipelineSet* PipelineSet = nullptr;
	jPostprocessChain PostProcessChain;
};

