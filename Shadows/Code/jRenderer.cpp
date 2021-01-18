#include "pch.h"
#include "jRenderer.h"
#include "jObject.h"
#include "jShadowAppProperties.h"
#include "jShader.h"
#include "jPipeline.h"

void jRenderer::Render(const jCamera* camera)
{
	ShadowPrePass(camera);
	DebugShadowPrePass(camera);

	RenderPass(camera);
	PostRenderPass(camera);
	DebugRenderPass(camera);
	BoundVolumeRenderPass(camera);

	PostProcessPass(camera);

	UIPass(camera);
	DebugUIPass(camera);
}

void jRenderer::SetChangePipelineSet(jPipelineSet* newPipelineSet)
{
	if (!newPipelineSet)
		return;

	PipelineSet = newPipelineSet;
}