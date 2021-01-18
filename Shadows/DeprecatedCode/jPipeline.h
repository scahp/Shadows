#pragma once
#include "jRHI.h"

class jObject;
class jCamera;
class jLight;
class jGBuffer;
class IShadowVolume;

// 고민해볼 점
// 하나의 픽셀을 그려내는 방식을 정의하는 것은 렌더링 파이프라인으로 인해 결정 된다.
// 그러므로 사용하는 쉐이더 + SSBO 와 Uniforms 들을 모두 묶어서 하나의 파이프라인을 구성하도록 하자.
// 디퍼드렌더링의 ShadowPass, RenderPass, ComputePass 에서 어떤 파이프라인을 쓰기로 결정했는지 설정해줄 수 있도록 하자.
// * 현재 파이프라인에는 메시의 타입과 쉐이더의 Vertex Input Stream 의 타입은 고려하지 않고 있다. 매칭되지 않은 Input은 무시된다고 가정하고 있음.
//   하지만 이 부분도 매칭을 시켜줄수있도록 해야할것 같다. 또한 쉐이더와 Mesh의 Vertex Input Stream Type을 서로 확인해 매칭이 될수있는지 확인할 수 있는지 알아보자.
//   혹은 그게 안된다면 Mesh 타입을 조사해서 Shader의 Vertex Input Binding 수를 변경하거나 반대의 상황(Vertex Input Binding 수를 확인하여 Mesh의 Vertex Input Stream 수 조정
//    , 단 가능한 모든 Vertex Input Stream과 Shader의 Vertex Input Binding은 고정)이 가능한가 알아보자.

struct jDeepShadowMapBuffers
{
	static constexpr size_t LinkedlistDepth = 50;
	static constexpr size_t LinkedListDepthSize = SM_LINKED_LIST_WIDTH * SM_LINKED_LIST_HEIGHT * LinkedlistDepth;

	~jDeepShadowMapBuffers()
	{
		delete AtomicBuffer;
		delete StartElementBuf;
		delete LinkedListEntryDepthAlphaNext;
		delete LinkedListEntryNeighbors;
	}

	IAtomicCounterBuffer* AtomicBuffer = nullptr;
	IShaderStorageBufferObject* StartElementBuf = nullptr;
	IShaderStorageBufferObject* LinkedListEntryDepthAlphaNext = nullptr;
	IShaderStorageBufferObject* LinkedListEntryNeighbors = nullptr;
};


//////////////////////////////////////////////////////////////////////////
// class IPipeline

// jPipelineContext
struct jPipelineContext
{
	static const std::list<jObject*> emptyObjectList;
	//static const std::list<const jLight*> emptyLightList;
	jPipelineContext()
		: Objects(emptyObjectList)// , Lights(emptyLightList)
	{ }
	jPipelineContext(const jRenderTarget* defaultRenderTarget, const std::list<jObject*>& objects, const jCamera* camera, const std::list<const jLight*>& lights)
		: DefaultRenderTarget(defaultRenderTarget), Objects(objects), Camera(camera), Lights(lights)
	{ }

	const jRenderTarget* DefaultRenderTarget = nullptr;
	const std::list<jObject*>& Objects;
	const jCamera* Camera = nullptr;
	const std::list<const jLight*> Lights;
};

class IPipeline
{
public:
	static std::unordered_map<std::string, IPipeline*> s_pipelinesNameMap;
	static std::unordered_set<IPipeline*> s_pipelinesNameSet;

	static void AddPipeline(const std::string& name, IPipeline* newPipeline);
	static IPipeline* GetPipeline(const std::string& name);
	static void SetupPipelines();
	static void TeardownPipelines();

	IPipeline() = default;
	virtual ~IPipeline() { }

	virtual void Setup() {}
	virtual void Teardown() {}

	virtual void Do(const jPipelineContext& pipelineContext) const = 0;
	FORCEINLINE void SetName(const std::string& name) { Name = name; }

protected:
	std::string Name;
	jShader* Shader = nullptr;
	Vector4 ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	ERenderBufferType ClearType = ERenderBufferType::NONE;
	bool EnableDepthTest = true;
	EComparisonFunc DepthStencilFunc = EComparisonFunc::LESS;
	bool EnableBlend = false;
	EBlendSrc BlendSrc = EBlendSrc::ONE;
	EBlendDest BlendDest = EBlendDest::ONE_MINUS_SRC_ALPHA;
	bool EnableDepthBias = false;
	float DepthSlopeBias = 1.0f;
	float DepthConstantBias = 1.0f;
	std::vector<IBuffer*> Buffers;
	bool EnableClear = true;
};

#define CREATE_PIPELINE_WITH_SETUP(PipelineClass, ...) \
[this]()\
{ \
auto newPipeline = new PipelineClass(__VA_ARGS__); \
newPipeline->SetName(#PipelineClass); \
newPipeline->Setup(); \
return newPipeline; \
}()

#define ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(RenderPassCont, PipelineClass, ...) \
RenderPassCont.push_back(CREATE_PIPELINE_WITH_SETUP(PipelineClass, __VA_ARGS__))

#define ADD_PIPELINE_AT_RENDERPASS(RenderPassCont, PipelineName) \
{ auto pipeline = IPipeline::GetPipeline(PipelineName); \
JASSERT(pipeline);\
RenderPassCont.push_back(pipeline); }

//////////////////////////////////////////////////////////////////////////
// jRenderPipeline
class jRenderPipeline : public IPipeline
{
public:
	virtual void Do(const jPipelineContext& pipelineContext) const override;
	virtual void Draw(const jPipelineContext& pipelineContext) const;
	virtual void Draw(const jPipelineContext& pipelineContext, const jShader* shader) const;
};


class jDeferredGeometryPipeline : public jRenderPipeline
{
public:
	jDeferredGeometryPipeline() = default;
	jDeferredGeometryPipeline(const jGBuffer* gBuffer)
		: GBuffer(gBuffer)
	{ }

	virtual void Setup() override;

	virtual void Draw(const jPipelineContext& pipelineContext, const jShader* shader) const override;

private:
	const jGBuffer* GBuffer = nullptr;
};

class jDeepShadowMap_ShadowPass_Pipeline : public jRenderPipeline
{
public:
	jDeepShadowMap_ShadowPass_Pipeline() = default;
	jDeepShadowMap_ShadowPass_Pipeline(const jDeepShadowMapBuffers& buffers)
		: DeepShadowMapBuffers(buffers)
	{ }

	virtual void Setup() override;
	virtual void Draw(const jPipelineContext& pipelineContext, const jShader* shader) const override;

private:
	jDeepShadowMapBuffers DeepShadowMapBuffers;
};

//////////////////////////////////////////////////////////////////////////
// jForward_ShadowMapGen_Pipeline
class jForward_ShadowMapGen_Pipeline : public jRenderPipeline
{
public:
	jForward_ShadowMapGen_Pipeline(const char* directionalLightShaderName, const char* omniDirectionalLightShaderName)
		: DirectionalLightShaderName(directionalLightShaderName), OmniDirectionalLightShaderName(omniDirectionalLightShaderName)
	{ }

	virtual void Setup() override;
	virtual void Do(const jPipelineContext& pipelineContext) const override;

	jShader* ShadowGenShader = nullptr;
	jShader* OmniShadowGenShader = nullptr;

	const char* DirectionalLightShaderName = nullptr;
	const char* OmniDirectionalLightShaderName = nullptr;
};

#define ADD_FORWARD_SHADOWMAP_GEN_PIPELINE(Name, DirectionalLightShaderName, OmniDirectionalLightShaderName) \
IPipeline::AddPipeline(#Name, new jForward_ShadowMapGen_Pipeline(DirectionalLightShaderName, OmniDirectionalLightShaderName))

//////////////////////////////////////////////////////////////////////////
// jForward_ShadowMapGen_CSM_SSM_Pipeline
class jForward_ShadowMapGen_CSM_SSM_Pipeline : public jRenderPipeline
{
public:
	jForward_ShadowMapGen_CSM_SSM_Pipeline(const char* directionalLightShaderName, const char* omniDirectionalLightShaderName)
		: DirectionalLightShaderName(directionalLightShaderName), OmniDirectionalLightShaderName(omniDirectionalLightShaderName)
	{}

	virtual void Setup() override;
	virtual void Do(const jPipelineContext& pipelineContext) const override;

	jShader* ShadowGenShader = nullptr;
	jShader* OmniShadowGenShader = nullptr;

	const char* DirectionalLightShaderName = nullptr;
	const char* OmniDirectionalLightShaderName = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// jForward_Shadow_Pipeline
class jForward_Shadow_Pipeline : public jRenderPipeline
{
public:
	jForward_Shadow_Pipeline(const char* shaderName)
		: ShaderName(shaderName)
	{}

	virtual void Setup() override;
	virtual void Do(const jPipelineContext& pipelineContext) const override;

	const char* ShaderName = nullptr;
};

#define ADD_FORWARD_SHADOW_PIPELINE(Name, ShaderName) \
IPipeline::AddPipeline(#Name, new jForward_Shadow_Pipeline(ShaderName))

class jForwardShadowMap_Blur_Pipeline : public jRenderPipeline
{
public:
	jForwardShadowMap_Blur_Pipeline() = default;

	virtual void Setup() override;
	virtual void Draw(const jPipelineContext& pipelineContext, const jShader* shader) const override;

	std::shared_ptr<class jPostProcess_Blur> PostProcessBlur;
	std::shared_ptr<struct jPostProcessInOutput> PostProcessInput;
	std::shared_ptr<struct jPostProcessInOutput> PostProcessOutput;
};

//////////////////////////////////////////////////////////////////////////
// jComputePipeline
class jComputePipeline : public IPipeline
{
public:
	virtual void Setup() {}
	virtual void Teardown() {}

	virtual void Do(const jPipelineContext& pipelineContext) const override;
	virtual void Dispatch() const;

	uint32 NumGroupsX = 0;
	uint32 NumGroupsY = 0;
	uint32 NumGroupsZ = 0;
	jShader* Shader = nullptr;
};

class jDeepShadowMap_Sort_ComputePipeline : public jComputePipeline
{
public:
	jDeepShadowMap_Sort_ComputePipeline() = default;
	jDeepShadowMap_Sort_ComputePipeline(const jDeepShadowMapBuffers& buffers)
		: DeepShadowMapBuffers(buffers)
	{ }

	virtual void Setup();
	virtual void Teardown();

	jUniformBuffer<int>* ShadowMapWidthUniform = nullptr;
	jUniformBuffer<int>* ShadowMapHeightUniform = nullptr;
	jDeepShadowMapBuffers DeepShadowMapBuffers;
};

class jDeepShadowMap_Link_ComputePipeline : public jComputePipeline
{
public:
	jDeepShadowMap_Link_ComputePipeline() = default;
	jDeepShadowMap_Link_ComputePipeline(const jDeepShadowMapBuffers& buffers)
		: DeepShadowMapBuffers(buffers)
	{ }

	virtual void Setup();
	virtual void Teardown();

	jUniformBuffer<int>* ShadowMapWidthUniform = nullptr;
	jUniformBuffer<int>* ShadowMapHeightUniform = nullptr;
	jDeepShadowMapBuffers DeepShadowMapBuffers;
};

//////////////////////////////////////////////////////////////////////////
// jPipelineSet

enum class EPipelineSetType
{
	None = 0,
	DeepShadowMap,
	Forward,
	Max,
};

class jPipelineSet
{
public:
	virtual ~jPipelineSet() {}
	std::list<const IPipeline*> ShadowPrePass;
	std::list<const IPipeline*> RenderPass;
	std::list<const IPipeline*> DebugRenderPass;
	std::list<const IPipeline*> BoundVolumeRenderPass;
	std::list<const IPipeline*> PostRenderPass;
	std::list<const IPipeline*> UIPass;
	std::list<const IPipeline*> DebugUIPass;

	virtual EPipelineSetType GetType() const { return EPipelineSetType::None; }
	virtual void Setup() {}
	virtual void Teardown() {}
};

//////////////////////////////////////////////////////////////////////////
// jDeferredDeepShadowMapPipelineSet
class jDeferredDeepShadowMapPipelineSet : public jPipelineSet
{
public:
	jDeferredDeepShadowMapPipelineSet(const jGBuffer* gBuffer)
		: GBuffer(gBuffer)
	{ }

	virtual EPipelineSetType GetType() const { return EPipelineSetType::DeepShadowMap; }
	virtual void Setup() override;

	const jGBuffer* GBuffer = nullptr;
	jDeepShadowMapBuffers DeepShadowMapBuffers;
};

#define START_CREATE_PIPELINE_SET_INFO(Name, Prefix, PipelineSetType) \
class j##Prefix##PipelineSet_##Name : public jPipelineSet \
{ \
public: \
	virtual EPipelineSetType GetType() const override { return PipelineSetType; } \
	virtual void Setup() override {

#define START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(Name, Prefix, PipelineSetType) \
START_CREATE_PIPELINE_SET_INFO(Name, Prefix, PipelineSetType) \
	ADD_PIPELINE_AT_RENDERPASS(DebugRenderPass, "Forward_DebugObject_Pipeline");\
	ADD_PIPELINE_AT_RENDERPASS(BoundVolumeRenderPass, "Forward_BoundVolume_Pipeline");\
	ADD_PIPELINE_AT_RENDERPASS(DebugUIPass, "Forward_UI_Pipeline");

#define END_CREATE_PIPELINE_SET_INFO() } };

//////////////////////////////////////////////////////////////////////////
// jForwardPipelineSet_SSM
START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(SSM, Forward, EPipelineSetType::Forward)
	ADD_PIPELINE_AT_RENDERPASS(ShadowPrePass, "Forward_ShadowMapGen_SSM_Pipeline");
	ADD_PIPELINE_AT_RENDERPASS(RenderPass, "Forward_SSM_Pipeline");
END_CREATE_PIPELINE_SET_INFO()

// jForwardPipelineSet_SSM_PCF
START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(SSM_PCF, Forward, EPipelineSetType::Forward)
	ADD_PIPELINE_AT_RENDERPASS(ShadowPrePass, "Forward_ShadowMapGen_SSM_Pipeline");
	ADD_PIPELINE_AT_RENDERPASS(RenderPass, "Forward_SSM_PCF_Pipeline");
END_CREATE_PIPELINE_SET_INFO()

// jForwardPipelineSet_SSM_PCSS
START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(SSM_PCSS, Forward, EPipelineSetType::Forward)
	ADD_PIPELINE_AT_RENDERPASS(ShadowPrePass, "Forward_ShadowMapGen_SSM_Pipeline");
	ADD_PIPELINE_AT_RENDERPASS(RenderPass, "Forward_SSM_PCSS_Pipeline");
END_CREATE_PIPELINE_SET_INFO()

// jForwardPipelineSet_SSM_PCF_Poisson
START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(SSM_PCF_Poisson, Forward, EPipelineSetType::Forward)
	ADD_PIPELINE_AT_RENDERPASS(ShadowPrePass, "Forward_ShadowMapGen_SSM_Pipeline");
	ADD_PIPELINE_AT_RENDERPASS(RenderPass, "Forward_SSM_PCF_Poisson_Pipeline");
END_CREATE_PIPELINE_SET_INFO()

// jForwardPipelineSet_SSM_PCSS_Poisson
START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(SSM_PCSS_Poisson, Forward, EPipelineSetType::Forward)
	ADD_PIPELINE_AT_RENDERPASS(ShadowPrePass, "Forward_ShadowMapGen_SSM_Pipeline");
	ADD_PIPELINE_AT_RENDERPASS(RenderPass, "Forward_SSM_PCSS_Poisson_Pipeline");
END_CREATE_PIPELINE_SET_INFO()

// jForwardPipelineSet_VSM
START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(VSM, Forward, EPipelineSetType::Forward)
	ADD_PIPELINE_AT_RENDERPASS(ShadowPrePass, "Forward_ShadowMapGen_VSM_Pipeline");
	ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(ShadowPrePass, jForwardShadowMap_Blur_Pipeline);
	ADD_PIPELINE_AT_RENDERPASS(RenderPass, "Forward_VSM_Pipeline");
END_CREATE_PIPELINE_SET_INFO()

// jForwardPipelineSet_VSM
START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(ESM, Forward, EPipelineSetType::Forward)
	ADD_PIPELINE_AT_RENDERPASS(ShadowPrePass, "Forward_ShadowMapGen_ESM_Pipeline");
	ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(ShadowPrePass, jForwardShadowMap_Blur_Pipeline);
	ADD_PIPELINE_AT_RENDERPASS(RenderPass, "Forward_ESM_Pipeline");
END_CREATE_PIPELINE_SET_INFO()

// jForwardPipelineSet_EVSM
START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(EVSM, Forward, EPipelineSetType::Forward)
	ADD_PIPELINE_AT_RENDERPASS(ShadowPrePass, "Forward_ShadowMapGen_EVSM_Pipeline");
	ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(ShadowPrePass, jForwardShadowMap_Blur_Pipeline);
	ADD_PIPELINE_AT_RENDERPASS(RenderPass, "Forward_EVSM_Pipeline");
END_CREATE_PIPELINE_SET_INFO()

// jForwardPipelineSet_CSM_SSM
START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(CSM_SSM, Forward, EPipelineSetType::Forward)
	ADD_PIPELINE_AT_RENDERPASS(ShadowPrePass, "Forward_ShadowMapGen_CSM_SSM_Pipeline");
	ADD_PIPELINE_AT_RENDERPASS(RenderPass, "Forward_CSM_SSM_Pipeline");
END_CREATE_PIPELINE_SET_INFO()

#define CREATE_PIPELINE_SET_WITH_SETUP(PipelineSetClass, ...) \
[this]()\
{ \
auto newPipeline = new PipelineSetClass(__VA_ARGS__); \
newPipeline->Setup(); \
return newPipeline; \
}()


//////////////////////////////////////////////////////////////////////////
// jForward_DebugObject_Pipeline
class jForward_DebugObject_Pipeline : public jRenderPipeline
{
public:
	jForward_DebugObject_Pipeline(const char* shaderName)
		: ShaderName(shaderName)
	{}

	virtual void Setup() override;
	const char* ShaderName = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// jForward_UIObject_Pipeline
class jForward_UIObject_Pipeline : public jRenderPipeline
{
public:
	jForward_UIObject_Pipeline(const char* shaderName)
		: ShaderName(shaderName)
	{}

	virtual void Setup() override;
	const char* ShaderName = nullptr;
};

//////////////////////////////////////////////////////////////////////////
// jForward_ShadowVolume_Pipeline
class jForward_ShadowVolume_Pipeline : public jRenderPipeline
{
public:
	using jRenderPipeline::jRenderPipeline;
	
	virtual void Setup() override;
	bool CanSkipShadowObject(const IShadowVolume* shadowVolume, const jCamera* camera, const jObject* object
		, const Vector& lightPosOrDirection, bool isOmniDirectional, const jLight* light) const;
	virtual void Do(const jPipelineContext& pipelineContext) const override;
};


// jForwardPipelineSet_ShadowVolume
START_CREATE_PIPELINE_SET_INFO_WITH_DEBUG_RENDER(ShadowVolume, Forward, EPipelineSetType::Forward)
	ADD_PIPELINE_WITH_CREATE_AND_SETUP_AT_RENDERPASS(RenderPass, jForward_ShadowVolume_Pipeline);
END_CREATE_PIPELINE_SET_INFO()

