#pragma once

struct jRenderTarget;
struct jShader;
class jCamera;
class jObject;
class jLight;
struct IShaderStorageBufferObject;
class jFullscreenQuadPrimitive;
class jGBuffer;

struct jPostProcessInOutput : public std::enable_shared_from_this<jPostProcessInOutput>
{
	jRenderTarget* RenderTarget = nullptr;

};

//////////////////////////////////////////////////////////////////////////
class IPostprocess
{
public:
	IPostprocess() = default;
	IPostprocess(const std::string& name)
		: Name(name)
	{ }
	virtual ~IPostprocess() {}
	std::string Name;

	struct jInputData
	{
		std::weak_ptr<jPostProcessInOutput> Input;
		std::shared_ptr<jSamplerState> SamplerState;
	};

	std::list<jInputData> PostProcessInputList;
	std::shared_ptr<jPostProcessInOutput> PostProcessOutput;

	virtual void Setup();
	virtual void TearDown() { }

	virtual bool Process(const jCamera* camera) const;

	virtual std::weak_ptr<jPostProcessInOutput> GetPostProcessOutput() const;
	virtual void AddInput(const std::weak_ptr<jPostProcessInOutput>& input, const std::shared_ptr<jSamplerState>& samplerState = nullptr);
	virtual void SetOutput(const std::shared_ptr<jPostProcessInOutput>& output);
	virtual void ClearInputs();
	virtual void ClearOutputs();
	virtual void ClearInOutputs();

protected:
	virtual bool Do(const jCamera* camera) const = 0;

	jFullscreenQuadPrimitive* GetFullscreenQuad() const;
	virtual void BindInputs(jFullscreenQuadPrimitive* fsQuad) const;
	virtual void UnbindInputs(jFullscreenQuadPrimitive* fsQuad) const;
	virtual void Draw(const jCamera* camera, const jShader* shader, const std::list<const jLight*>& lights) const;
};

//////////////////////////////////////////////////////////////////////////
class jPostprocessChain
{
public:
	jPostprocessChain() = default;

	void AddNewPostprocess(IPostprocess* postprocess);
	void AddNewPostprocess(IPostprocess* postprocess, const std::weak_ptr<jPostProcessInOutput>& input);
	void ReleaseAllPostprocesses();

	bool Process(const jCamera* camera) const;

private:
	std::list<IPostprocess*> PostProcesses;
};

//////////////////////////////////////////////////////////////////////////
// DeepShadowMap postprocess
class jPostProcess_DeepShadowMap : public IPostprocess
{
public:
	struct jSSBO_DeepShadowMap
	{
		IShaderStorageBufferObject* StartElementBuf = nullptr;
		IShaderStorageBufferObject* LinkedListEntryDepthAlphaNext = nullptr;
		IShaderStorageBufferObject* LinkedListEntryNeighbors = nullptr;

		void Bind(const jShader* shader) const;
	};

	jPostProcess_DeepShadowMap() = default;
	jPostProcess_DeepShadowMap(const std::string& name, const jSSBO_DeepShadowMap& ssbos, const jGBuffer* gBuffer)
		: IPostprocess(name), SSBOs(ssbos), GBuffer(gBuffer)
	{ }

	virtual bool Do(const jCamera* camera) const override;

private:
	jSSBO_DeepShadowMap SSBOs;
	const jGBuffer* GBuffer = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// DeepShadowMap anti aliasing postprocess
class jPostProcess_AA_DeepShadowAddition : public IPostprocess
{
public:
	using IPostprocess::IPostprocess;

	virtual void Setup() override;
	virtual bool Do(const jCamera* camera) const override;

private:
	const jShader* Shader = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// Blur
class jPostProcess_Blur : public IPostprocess
{
public:
	jPostProcess_Blur() = default;
	jPostProcess_Blur(const std::string& name, bool isVertical)
		: IPostprocess(name), IsVertical(isVertical)
	{ }

	virtual bool Do(const jCamera* camera) const override;

	float MaxDist = FLT_MAX;
	bool IsVertical = false;
	bool OmniDirectional = false;
};

//////////////////////////////////////////////////////////////////////////
// Tonemap
class jPostProcess_Tonemap : public IPostprocess
{
public:
	using IPostprocess::IPostprocess;

	virtual void Setup() override;
	virtual bool Do(const jCamera* camera) const override;

private:
	const jShader* Shader = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// LuminanceMapGeneration
class jPostProcess_LuminanceMapGeneration : public IPostprocess
{
public:
	using IPostprocess::IPostprocess;

	virtual bool Process(const jCamera* camera) const override;
	virtual void Setup() override;
	virtual bool Do(const jCamera* camera) const override;

private:
	const jShader* Shader = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// AdaptiveLuminance
class jPostProcess_AdaptiveLuminance : public IPostprocess
{
public:
	jPostProcess_AdaptiveLuminance()
	{ }
	jPostProcess_AdaptiveLuminance(const std::string& name)
		: IPostprocess(name)
	{ }

	virtual void Setup() override;
	virtual bool Do(const jCamera* camera) const override;
	virtual bool Process(const jCamera* camera) const override;
	virtual void BindInputs(jFullscreenQuadPrimitive* fsQuad) const override;
	virtual void UnbindInputs(jFullscreenQuadPrimitive* fsQuad) const override;

	static int32 s_index;
	static int32 UpdateLuminanceIndex()
	{		
		s_index = static_cast<int32>(!s_index);
		return s_index;			
	}

private:
	const jShader* Shader = nullptr;
	std::shared_ptr<jRenderTarget> LastLumianceRenderTarget[2];
};

//////////////////////////////////////////////////////////////////////////
// BloomThreshold
class jPostProcess_BloomThreshold : public IPostprocess
{
public:
	using IPostprocess::IPostprocess;

	virtual void Setup() override;
	virtual bool Do(const jCamera* camera) const override;

private:
	const jShader* Shader = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// Scale
class jPostProcess_Scale : public IPostprocess
{
public:
	using IPostprocess::IPostprocess;

	virtual void Setup() override;
	virtual bool Do(const jCamera* camera) const override;

private:
	const jShader* Shader = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// GaussianBlurH
class jPostProcess_GaussianBlurH : public IPostprocess
{
public:
	using IPostprocess::IPostprocess;

	virtual void Setup() override;
	virtual bool Do(const jCamera* camera) const override;
	virtual void BindInputs(jFullscreenQuadPrimitive* fsQuad) const override;
	virtual void UnbindInputs(jFullscreenQuadPrimitive* fsQuad) const override;

private:
	const jShader* Shader = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// GaussianBlurV
class jPostProcess_GaussianBlurV : public IPostprocess
{
public:
	using IPostprocess::IPostprocess;

	virtual void Setup() override;
	virtual bool Do(const jCamera* camera) const override;
	virtual void BindInputs(jFullscreenQuadPrimitive* fsQuad) const override;
	virtual void UnbindInputs(jFullscreenQuadPrimitive* fsQuad) const override;

private:
	const jShader* Shader = nullptr;
};

