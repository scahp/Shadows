#pragma once

class jRenderContext;
class jFullscreenQuadPrimitive;
class jUIQuadPrimitive;

class jDeferredRenderer
{
public:
	void Culling(jRenderContext* InContext) const;
	void DepthPrepass(jRenderContext* InContext) const;
	void ShowdowMap(jRenderContext* InContext) const;
	void GBuffer(jRenderContext* InContext) const;
	void SSAO(jRenderContext* InContext) const;
	void LightingPass(jRenderContext* InContext) const;
	void SSR(jRenderContext* InContext) const;
	void PPR(jRenderContext* InContext) const;
	void AA(jRenderContext* InContext) const;
	void Tonemap(jRenderContext* InContext) const;

	void Init();
	void Render(jRenderContext* InContext);
	void Release();

	void InitSSAO();

private:
	std::shared_ptr<jRenderTarget> ShadowRTPtr;
	std::shared_ptr<jRenderTarget> DepthRTPtr;
	std::shared_ptr<jRenderTarget> HiZRTPtr;
	std::shared_ptr<jRenderTarget> GBufferRTPtr;
	jMaterialData GBufferMaterialData;
	jMaterialData ShadowMaterialData;

	jFullscreenQuadPrimitive* FullscreenQuad = nullptr;	
	jUIQuadPrimitive* DebugQuad = nullptr;

	std::shared_ptr<jTexture> SSAONoiseTex;
	std::shared_ptr<jRenderTarget> SSAORTPtr;
	std::shared_ptr<jRenderTarget> SSAORTBlurredPtr;
	std::vector<Vector> ssaoKernel;
	jMaterialData SSAOMaterialData;
	jMaterialData SSAOBlurMaterialData;
	jMaterialData SSAOApplyMaterialData;

	std::shared_ptr<jRenderTarget> SceneColorRTPtr;
	jMaterialData SSRMaterialData;

	std::shared_ptr<jRenderTarget> SSRRTPtr;
	jMaterialData AAMaterialData;

	std::shared_ptr<jRenderTarget> AARTPtr;
	jMaterialData FinalMaterialData;

	std::shared_ptr<jRenderTarget> GetDebugRTPtr() const;

	std::shared_ptr<jRenderTarget> IntermediateBufferPtr;
	std::shared_ptr<jRenderTarget> PPRRTPtr;
	jMaterialData PPRMaterialData;

	std::shared_ptr<jRenderTarget> TonemapRTPtr;
	jMaterialData TonemapMaterialData;
};