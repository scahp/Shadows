#pragma once

class jRenderContext;

class jDeferredRenderer
{
public:
	void Culling(jRenderContext* InContext) const;
	void DepthPrepass(jRenderContext* InContext) const;
	void ShowdowMap(jRenderContext* InContext) const;
	void GBuffer(jRenderContext* InContext) const;
	void SSAO(jRenderContext* InContext) const;
	void LightingPass(jRenderContext* InContext) const;
	void Tonemap(jRenderContext* InContext) const;
	void SSR(jRenderContext* InContext) const;
	void AA(jRenderContext* InContext) const;

	void Init();
	void Render(jRenderContext* InContext);

private:
	std::shared_ptr<jRenderTarget> DepthRTPtr;
	std::shared_ptr<jRenderTarget> GBufferRTPtr;
};