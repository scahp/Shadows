#include "pch.h"
#include "jPostProcess.h"
#include "jCamera.h"
#include "jRHI.h"
#include "jLight.h"
#include "jPrimitiveUtil.h"
#include "jShadowAppProperties.h"
#include "jGBuffer.h"
#include "jRenderTargetPool.h"

//////////////////////////////////////////////////////////////////////////
// IPostprocess
void IPostprocess::Setup()
{
}

bool IPostprocess::Process(const jCamera* camera) const
{
	SCOPE_DEBUG_EVENT(g_rhi, Name.c_str());

	auto pCurrentRenderTarget = (PostProcessOutput ? PostProcessOutput.get()->RenderTarget : nullptr);

	if (pCurrentRenderTarget)
		pCurrentRenderTarget->Begin();
	else
		g_rhi->SetRenderTarget(nullptr);

	auto result = Do(camera);

	if (pCurrentRenderTarget)
		pCurrentRenderTarget->End();

	return result;
}

std::weak_ptr<jPostProcessInOutput> IPostprocess::GetPostProcessOutput() const
{
	return PostProcessOutput;
}

void IPostprocess::AddInput(const std::weak_ptr<jPostProcessInOutput>& input, const std::shared_ptr<jSamplerState>& samplerState)
{
	PostProcessInputList.push_back({ input, samplerState });
}

void IPostprocess::SetOutput(const std::shared_ptr<jPostProcessInOutput>& output)
{
	PostProcessOutput = output;
}

void IPostprocess::ClearInputs()
{
	PostProcessInputList.clear();
}

void IPostprocess::ClearOutputs()
{
	PostProcessOutput = nullptr;
}

void IPostprocess::ClearInOutputs()
{
	ClearInputs();
	ClearOutputs();
}

jFullscreenQuadPrimitive* IPostprocess::GetFullscreenQuad() const
{
	static jFullscreenQuadPrimitive* s_fullscreenQuad = jPrimitiveUtil::CreateFullscreenQuad(nullptr);
	return s_fullscreenQuad;
}

void IPostprocess::BindInputs(jFullscreenQuadPrimitive* fsQuad) const
{
	int index = 0;
	for (auto it = PostProcessInputList.begin(); PostProcessInputList.end() != it; ++it,++index)
	{
		const auto& input = it->Input;
		const auto texture = input.expired() ? nullptr : input.lock()->RenderTarget->GetTexture();
		fsQuad->SetTexture(index, texture, it->SamplerState.get());
	}
}

void IPostprocess::UnbindInputs(jFullscreenQuadPrimitive* fsQuad) const
{
	const int32 size = static_cast<int32>(PostProcessInputList.size());
	for (int32 i = 0; i < size; ++i)
		fsQuad->SetTexture(i, nullptr, nullptr);
}

void IPostprocess::Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights) const
{
	GetFullscreenQuad()->Draw(camera, shader, lights);
}

//////////////////////////////////////////////////////////////////////////
// jPostprocessChain
void jPostprocessChain::AddNewPostprocess(IPostprocess* postprocess)
{
	JASSERT(postprocess);
	postprocess->Setup();
	PostProcesses.push_back(postprocess);
}

void jPostprocessChain::ReleaseAllPostprocesses()
{
	for (auto iter : PostProcesses)
		iter->TearDown();
	PostProcesses.clear();
}

bool jPostprocessChain::Process(const jCamera* camera) const
{
	for (auto iter : PostProcesses)
		iter->Process(camera);
	return true;
}

//////////////////////////////////////////////////////////////////////////
// jPostProcess_DeepShadowMap
void jPostProcess_DeepShadowMap::jSSBO_DeepShadowMap::Bind(const jShader* shader) const
{
	JASSERT(shader);
	JASSERT(StartElementBuf);
	JASSERT(LinkedListEntryDepthAlphaNext);
	JASSERT(LinkedListEntryNeighbors);

	StartElementBuf->Bind(shader);
	LinkedListEntryDepthAlphaNext->Bind(shader);
	LinkedListEntryNeighbors->Bind(shader);
}

bool jPostProcess_DeepShadowMap::Do(const jCamera* camera) const
{
	g_rhi->SetClearColor(135.0f / 255.0f, 206.0f / 255.0f, 250.0f / 255.0f, 1.0f);	// light sky blue
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);

	const auto ambientLight = camera->GetLight(ELightType::AMBIENT);
	const auto directionalLight = camera->GetLight(ELightType::DIRECTIONAL);

	std::list<const jLight*> lights;
	if (ambientLight)
		lights.push_back(ambientLight);
	if (directionalLight)
		lights.push_back(directionalLight);

	if (auto deepShadowFull_Shader = jShadowAppSettingProperties::GetInstance().ExponentDeepShadowOn ? jShader::GetShader("ExpDeepShadowFull") : jShader::GetShader("DeepShadowFull"))
	{
		g_rhi->SetShader(deepShadowFull_Shader);

		camera->BindCamera(deepShadowFull_Shader);
		jLight::BindLights(lights, deepShadowFull_Shader);

		SSBOs.Bind(deepShadowFull_Shader);

		JASSERT(GBuffer);
		GBuffer->BindGeometryBuffer(deepShadowFull_Shader);

		Draw(camera, deepShadowFull_Shader, lights);
	}

	return true;
}

void jPostProcess_AA_DeepShadowAddition::Setup()
{
	__super::Setup();
	Shader = jShader::GetShader("DeepShadowAA");
}

bool jPostProcess_AA_DeepShadowAddition::Do(const jCamera* camera) const
{
	JASSERT(Shader);
	//JASSERT(!PostProcessInput.expired());

	g_rhi->SetClearColor(0.025f, 0.025f, 0.025f, 1.0f);
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);

	auto fullscreenQuad = GetFullscreenQuad();
	BindInputs(fullscreenQuad);
	camera->BindCamera(Shader);
	fullscreenQuad->Draw(camera, Shader, {});
	UnbindInputs(fullscreenQuad);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// jPostProcess_Blur
bool jPostProcess_Blur::Do(const jCamera* camera) const
{
//	JASSERT(!PostProcessInput.expired());

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);

	auto shader = OmniDirectional ? jShader::GetShader("BlurOmni") : jShader::GetShader("Blur");

	auto fullscreenQuad = GetFullscreenQuad();

	BindInputs(fullscreenQuad);

	g_rhi->SetShader(shader);
	g_rhi->SetUniformbuffer("IsVertical", IsVertical, shader);
	g_rhi->SetUniformbuffer("MaxDist", MaxDist, shader);

	fullscreenQuad->Draw(camera, shader, {});
	
	UnbindInputs(fullscreenQuad);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// jPostProcess_Tonemap
void jPostProcess_Tonemap::Setup()
{
	__super::Setup();
	Shader = jShader::GetShader("Tonemap");
}

bool jPostProcess_Tonemap::Do(const jCamera* camera) const
{
	JASSERT(Shader);

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);

	auto fullscreenQuad = GetFullscreenQuad();
	BindInputs(fullscreenQuad);
	camera->BindCamera(Shader);
	
	g_rhi->SetShader(Shader);
	g_rhi->SetUniformbuffer("UseTonemap", jShadowAppSettingProperties::GetInstance().UseTonemap, Shader);
	g_rhi->SetUniformbuffer("AutoExposureKeyValue", jShadowAppSettingProperties::GetInstance().AutoExposureKeyValue, Shader);
	g_rhi->SetUniformbuffer("BloomMagnitude", jShadowAppSettingProperties::GetInstance().BloomMagnitude, Shader);

	fullscreenQuad->Draw(camera, Shader, {});
	UnbindInputs(fullscreenQuad);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// jPostProcess_LuminanceMapGeneration
void jPostProcess_LuminanceMapGeneration::Setup()
{
	__super::Setup();
	Shader = jShader::GetShader("LuminanceMapGeneration");
}

bool jPostProcess_LuminanceMapGeneration::Process(const jCamera* camera) const
{
	SCOPE_DEBUG_EVENT(g_rhi, Name.c_str());

	auto pCurrentRenderTarget = (PostProcessOutput ? PostProcessOutput.get()->RenderTarget : nullptr);

	if (pCurrentRenderTarget)
		pCurrentRenderTarget->Begin();
	else
		g_rhi->SetRenderTarget(nullptr);

	auto result = Do(camera);

	if (pCurrentRenderTarget)
		pCurrentRenderTarget->End();

	if (pCurrentRenderTarget)
		g_rhi->GenerateMips(pCurrentRenderTarget->GetTexture());

	return result;
}

bool jPostProcess_LuminanceMapGeneration::Do(const jCamera* camera) const
{
	JASSERT(Shader);
	//JASSERT(!PostProcessInput.expired());

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);

	auto fullscreenQuad = GetFullscreenQuad();
	
	BindInputs(fullscreenQuad);
	camera->BindCamera(Shader);
	fullscreenQuad->Draw(camera, Shader, {});
	UnbindInputs(fullscreenQuad);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// jPostProcess_AdaptiveLuminance
int32 jPostProcess_AdaptiveLuminance::s_index = 0;

void jPostProcess_AdaptiveLuminance::BindInputs(jFullscreenQuadPrimitive* fsQuad) const
{
	int index = 0;
	for (auto it = PostProcessInputList.begin(); PostProcessInputList.end() != it; ++it, ++index)
	{
		const auto& input = it->Input;
		const auto texture = input.expired() ? nullptr : input.lock()->RenderTarget->GetTexture();
		fsQuad->SetTexture(index, texture, it->SamplerState.get());
	}
	auto pLastLuminanceRenderTarget = LastLumianceRenderTarget[!s_index].get()->GetTexture();
	fsQuad->SetTexture(index++, pLastLuminanceRenderTarget, nullptr);
}

void jPostProcess_AdaptiveLuminance::UnbindInputs(jFullscreenQuadPrimitive* fsQuad) const
{
	const int32 size = static_cast<int32>(PostProcessInputList.size() + 1);
	for (int32 i = 0; i < size; ++i)
		fsQuad->SetTexture(i, nullptr, nullptr);
}

void jPostProcess_AdaptiveLuminance::Setup()
{
	__super::Setup();
	Shader = jShader::GetShader("AdaptiveLuminance");
	LastLumianceRenderTarget[0] = jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R32F, ETextureFormat::R, EFormatType::FLOAT, EDepthBufferType::NONE, 1, 1, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR_MIPMAP_LINEAR });
	LastLumianceRenderTarget[1] = jRenderTargetPool::GetRenderTarget({ ETextureType::TEXTURE_2D, ETextureFormat::R32F, ETextureFormat::R, EFormatType::FLOAT, EDepthBufferType::NONE, 1, 1, 1, ETextureFilter::LINEAR, ETextureFilter::LINEAR_MIPMAP_LINEAR });
}

bool jPostProcess_AdaptiveLuminance::Do(const jCamera* camera) const
{
	JASSERT(Shader);

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);

	auto fullscreenQuad = GetFullscreenQuad();

	BindInputs(fullscreenQuad);
	camera->BindCamera(Shader);
	g_rhi->SetShader(Shader);
	g_rhi->SetUniformbuffer("TimeDeltaSecond", g_timeDeltaSecond, Shader);
	g_rhi->SetUniformbuffer("AdaptationRate", jShadowAppSettingProperties::GetInstance().AdaptationRate, Shader);
	fullscreenQuad->Draw(camera, Shader, {});
	UnbindInputs(fullscreenQuad);
	return true;
}

bool jPostProcess_AdaptiveLuminance::Process(const jCamera* camera) const
{
	SCOPE_DEBUG_EVENT(g_rhi, Name.c_str());
	UpdateLuminanceIndex();

	auto pCurrentRenderTarget = (LastLumianceRenderTarget[s_index] ? LastLumianceRenderTarget[s_index] : nullptr);

	if (pCurrentRenderTarget)
		pCurrentRenderTarget->Begin();
	else
		g_rhi->SetRenderTarget(nullptr);

	auto result = Do(camera);

	if (pCurrentRenderTarget)
		pCurrentRenderTarget->End();

	JASSERT(PostProcessOutput);
	if (PostProcessOutput)
		PostProcessOutput.get()->RenderTarget = pCurrentRenderTarget.get();

	return result;

}

//////////////////////////////////////////////////////////////////////////
// jPostProcess_BloomThreshold
void jPostProcess_BloomThreshold::Setup()
{
	__super::Setup();
	Shader = jShader::GetShader("BloomThreshold");
}

bool jPostProcess_BloomThreshold::Do(const jCamera* camera) const
{
	JASSERT(Shader);

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);

	auto fullscreenQuad = GetFullscreenQuad();
	BindInputs(fullscreenQuad);
	camera->BindCamera(Shader);

	g_rhi->SetShader(Shader);
	g_rhi->SetUniformbuffer("BloomThreshold", jShadowAppSettingProperties::GetInstance().BloomThreshold, Shader);
	g_rhi->SetUniformbuffer("AutoExposureKeyValue", jShadowAppSettingProperties::GetInstance().AutoExposureKeyValue, Shader);

	fullscreenQuad->Draw(camera, Shader, {});
	UnbindInputs(fullscreenQuad);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// jPostProcess_Scale
void jPostProcess_Scale::Setup()
{
	__super::Setup();
	Shader = jShader::GetShader("Scale");
}

bool jPostProcess_Scale::Do(const jCamera* camera) const
{
	JASSERT(Shader);

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);

	auto fullscreenQuad = GetFullscreenQuad();
	BindInputs(fullscreenQuad);
	camera->BindCamera(Shader);

	g_rhi->SetShader(Shader);

	fullscreenQuad->Draw(camera, Shader, {});
	UnbindInputs(fullscreenQuad);

	return true;
}

//////////////////////////////////////////////////////////////////////////
// jPostProcess_GaussianBlurH
void jPostProcess_GaussianBlurH::Setup()
{
	__super::Setup();
	Shader = jShader::GetShader("GaussianBlurH");
}

bool jPostProcess_GaussianBlurH::Do(const jCamera* camera) const
{
	JASSERT(Shader);

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);

	auto fullscreenQuad = GetFullscreenQuad();
	BindInputs(fullscreenQuad);
	camera->BindCamera(Shader);

	g_rhi->SetShader(Shader);
	g_rhi->SetUniformbuffer("BloomBlurSigma", jShadowAppSettingProperties::GetInstance().BloomBlurSigma, Shader);
	
	Vector2 renderTargetSize;
	auto pCurrentRenderTarget = (PostProcessOutput ? PostProcessOutput.get()->RenderTarget : nullptr);
	if (pCurrentRenderTarget)
		renderTargetSize = Vector2(static_cast<float>(pCurrentRenderTarget->Info.Width), static_cast<float>(pCurrentRenderTarget->Info.Height));
	else
		renderTargetSize = Vector2(SCR_WIDTH, SCR_HEIGHT);
	g_rhi->SetUniformbuffer("RenderTargetSize", renderTargetSize, Shader);

	fullscreenQuad->Draw(camera, Shader, {});
	UnbindInputs(fullscreenQuad);

	return true;
}

void jPostProcess_GaussianBlurH::BindInputs(jFullscreenQuadPrimitive* fsQuad) const
{
	int index = 0;
	for (auto it = PostProcessInputList.begin(); PostProcessInputList.end() != it; ++it, ++index)
	{
		const auto& input = it->Input;
		auto texture = input.expired() ? nullptr : input.lock()->RenderTarget->GetTexture();
		if (texture)
		{
			texture->Magnification = ETextureFilter::NEAREST;
			texture->Minification = ETextureFilter::NEAREST;
		}
		fsQuad->SetTexture(index, texture, it->SamplerState.get());
	}
}

void jPostProcess_GaussianBlurH::UnbindInputs(jFullscreenQuadPrimitive* fsQuad) const
{
	int index = 0;
	for (auto it = PostProcessInputList.begin(); PostProcessInputList.end() != it; ++it, ++index)
	{
		const auto& input = it->Input;
		if (!input.expired())
		{
			auto rt = input.lock()->RenderTarget;
			auto texture = rt->GetTexture();
			texture->Magnification = rt->Info.Magnification;
			texture->Minification = rt->Info.Minification;
		}
		fsQuad->SetTexture(index, nullptr, nullptr);
	}
}

//////////////////////////////////////////////////////////////////////////
// jPostProcess_GaussianBlurV
void jPostProcess_GaussianBlurV::Setup()
{
	__super::Setup();
	Shader = jShader::GetShader("GaussianBlurV");
}

bool jPostProcess_GaussianBlurV::Do(const jCamera* camera) const
{
	JASSERT(Shader);

	g_rhi->SetClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	g_rhi->SetClear(ERenderBufferType::COLOR | ERenderBufferType::DEPTH);

	auto fullscreenQuad = GetFullscreenQuad();
	BindInputs(fullscreenQuad);
	camera->BindCamera(Shader);

	g_rhi->SetShader(Shader);
	g_rhi->SetUniformbuffer("BloomBlurSigma", jShadowAppSettingProperties::GetInstance().BloomBlurSigma, Shader);

	Vector2 renderTargetSize;
	auto pCurrentRenderTarget = (PostProcessOutput ? PostProcessOutput.get()->RenderTarget : nullptr);
	if (pCurrentRenderTarget)
		renderTargetSize = Vector2(static_cast<float>(pCurrentRenderTarget->Info.Width), static_cast<float>(pCurrentRenderTarget->Info.Height));
	else
		renderTargetSize = Vector2(SCR_WIDTH, SCR_HEIGHT);
	g_rhi->SetUniformbuffer("RenderTargetSize", renderTargetSize, Shader);
	fullscreenQuad->Draw(camera, Shader, {});
	UnbindInputs(fullscreenQuad);

	return true;
}

void jPostProcess_GaussianBlurV::BindInputs(jFullscreenQuadPrimitive* fsQuad) const
{
	int index = 0;
	for (auto it = PostProcessInputList.begin(); PostProcessInputList.end() != it; ++it, ++index)
	{
		const auto& input = it->Input;
		auto texture = input.expired() ? nullptr : input.lock()->RenderTarget->GetTexture();
		if (texture)
		{
			texture->Magnification = ETextureFilter::NEAREST;
			texture->Minification = ETextureFilter::NEAREST;
		}
		fsQuad->SetTexture(index, texture, it->SamplerState.get());
	}
}

void jPostProcess_GaussianBlurV::UnbindInputs(jFullscreenQuadPrimitive* fsQuad) const
{
	int index = 0;
	for (auto it = PostProcessInputList.begin(); PostProcessInputList.end() != it; ++it, ++index)
	{
		const auto& input = it->Input;
		if (!input.expired())
		{
			auto rt = input.lock()->RenderTarget;
			auto texture = rt->GetTexture();
			texture->Magnification = rt->Info.Magnification;
			texture->Minification = rt->Info.Minification;
		}
		fsQuad->SetTexture(index, nullptr, nullptr);
	}
}