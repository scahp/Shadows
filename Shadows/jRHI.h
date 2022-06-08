#pragma once
#include "jRHIType.h"
#include "Math\Matrix.h"
#include "Core\jName.h"

extern class jRHI* g_rhi;

struct jShader;
struct jShaderInfo;
struct jSamplerState;

struct IBuffer
{
	virtual ~IBuffer() {}

	virtual jName GetName() const { return jName::Invalid; }
	virtual void Bind(const jShader* shader) const = 0;
};

struct jVertexBuffer : public IBuffer
{
	std::weak_ptr<jVertexStreamData> VertexStreamData;

	virtual void Bind(const jShader* shader) const {}
};

struct jIndexBuffer : public IBuffer
{
	std::weak_ptr<jIndexStreamData> IndexStreamData;

	virtual void Bind(const jShader* shader) const {}
};

struct jTexture
{
	static int32 GetMipLevels(int32 InWidth, int32 InHeight)
	{
		return 1 + (int32)floorf(log2f(fmaxf((float)InWidth, (float)InHeight)));
	}
	bool sRGB = false;
	ETextureType Type = ETextureType::MAX;
	ETextureFormat ColorBufferType = ETextureFormat::RGB;
	EFormatType ColorPixelType = EFormatType::BYTE;
	EDepthBufferType DepthBufferType = EDepthBufferType::NONE;

	int32 Width = 0;
	int32 Height = 0;

	ETextureFilter Minification = ETextureFilter::NEAREST;
	ETextureFilter Magnification = ETextureFilter::NEAREST;
};

struct jViewport
{
	jViewport() = default;
	jViewport(int32 x, int32 y, int32 width, int32 height)
		: X(static_cast<float>(x)), Y(static_cast<float>(y))
		, Width(static_cast<float>(width)), Height(static_cast<float>(height)) 
	{}
	jViewport(float x, float y, float width, float height) : X(x), Y(y), Width(width), Height(height) {}

	float X = 0.0f;
	float Y = 0.0f;
	float Width = 0.0f;
	float Height = 0.0f;
};

enum class EUniformType
{
	NONE = 0,
	MATRIX,
	BOOL,
	INT,
	FLOAT,
	VECTOR2,
	VECTOR3,
	VECTOR4,
	VECTOR2I,
	VECTOR3I,
	VECTOR4I, 
	MAX,
};

template <typename T> struct ConvertUniformType { };
template <> struct ConvertUniformType<Matrix> { operator EUniformType () { return EUniformType::MATRIX; } };
template <> struct ConvertUniformType<bool> { operator EUniformType () { return EUniformType::BOOL; } };
template <> struct ConvertUniformType<int> { operator EUniformType () { return EUniformType::INT; } };
template <> struct ConvertUniformType<float> { operator EUniformType () { return EUniformType::FLOAT; } };
template <> struct ConvertUniformType<Vector2> { operator EUniformType () { return EUniformType::VECTOR2; } };
template <> struct ConvertUniformType<Vector> { operator EUniformType () { return EUniformType::VECTOR3; } };
template <> struct ConvertUniformType<Vector4> { operator EUniformType () { return EUniformType::VECTOR4; } };

struct IUniformBuffer : public IBuffer
{
	IUniformBuffer() = default;
	IUniformBuffer(const jName& name)
		: Name(name)
	{}
	virtual ~IUniformBuffer() {}
	jName Name;

	virtual jName GetName() const { return Name; }
	virtual EUniformType GetType() const { return EUniformType::NONE; }
	virtual void SetUniformbuffer(const jShader* /*InShader*/) const {}
	virtual void Bind(const jShader* shader) const override;
};

template <typename T>
struct jUniformBuffer : public IUniformBuffer
{
	T Data;
};

static int32 GetBindPoint()
{
	static int32 s_index = 0;
	return s_index++;
}

struct IUniformBufferBlock : public IBuffer
{
	IUniformBufferBlock() = default;
	IUniformBufferBlock(const std::string& name)
		: Name(name)
	{}
	virtual ~IUniformBufferBlock() {}

	std::string Name;
	size_t Size = 0;

	virtual void Init() = 0;
	virtual void Bind(const jShader* shader) const override { }
	virtual void UpdateBufferData(const void* newData, size_t size) = 0;
	virtual void ClearBuffer(int32 clearValue = 0) = 0;
};

struct IShaderStorageBufferObject : public IBuffer
{
	IShaderStorageBufferObject() = default;
	IShaderStorageBufferObject(const std::string& name)
		: Name(name)
	{}
	virtual ~IShaderStorageBufferObject() {}

	std::string Name;
	size_t Size = 0;

	virtual void Init() = 0;
	virtual void Bind(const jShader* shader) const override {}
	virtual void UpdateBufferData(void* newData, size_t size) = 0;
	virtual void GetBufferData(void* newData, size_t size) = 0;
	virtual void ClearBuffer(int32 clearValue) = 0;

	template <typename T>
	void GetBufferData(typename std::remove_reference<T>::type& out)
	{
		GetBufferData(&out, sizeof(T));
	}
};

struct IAtomicCounterBuffer : public IBuffer
{
	IAtomicCounterBuffer() = default;
	IAtomicCounterBuffer(const std::string& name, uint32 bindingPoint)
		: Name(name), BindingPoint(bindingPoint)
	{}
	virtual ~IAtomicCounterBuffer() {}

	std::string Name;
	size_t Size = 0;
	uint32 BindingPoint = -1;

	virtual void Init() = 0;
	virtual void Bind(const jShader* shader) const override {}
	virtual void UpdateBufferData(void* newData, size_t size) = 0;
	virtual void GetBufferData(void* newData, size_t size) = 0;
	virtual void ClearBuffer(int32 clearValue) = 0;

	template <typename T>
	void GetBufferData(typename std::remove_reference<T>::type& out)
	{
		GetBufferData(&out, sizeof(T));
	}
};

struct ITransformFeedbackBuffer : public IBuffer
{
	ITransformFeedbackBuffer() = default;
	ITransformFeedbackBuffer(const std::string& name)
		: Name(name)
	{}
	virtual ~ITransformFeedbackBuffer() {}

	std::string Name;
	size_t Size = 0;
	std::vector<std::string> Varyings;

	virtual void Init() = 0;
	virtual void Bind(const jShader* shader) const override {}
	virtual void UpdateBufferData(void* newData, size_t size) = 0;
	virtual void GetBufferData(void* newData, size_t size) = 0;
	virtual void ClearBuffer(int32 clearValue) = 0;
	virtual void UpdateVaryingsToShader(const std::vector<std::string>& varyings, const jShader* shader) = 0;
	virtual void ClearVaryingsToShader(const jShader* shader) = 0;
	virtual void Begin(EPrimitiveType type) = 0;
	virtual void End() = 0;
	virtual void Pause() = 0;

	template <typename T>
	void GetBufferData(typename std::remove_reference<T>::type& out)
	{
		GetBufferData(&out, sizeof(T));
	}
};

struct jMaterialParam
{
	jMaterialParam() = default;
	jMaterialParam(const jName& name, const jTexture* texture, const jSamplerState* samplerstate)
		: Name(name), Texture(texture), SamplerState(samplerstate)
	{}
	virtual ~jMaterialParam() {}

	jName Name;
	const jTexture* Texture = nullptr;
	const jSamplerState* SamplerState = nullptr;
};

struct jMaterialData
{
	~jMaterialData()
	{
		Clear();
	}

	void AddMaterialParam(const jName& name, const jTexture* texture, const jSamplerState* samplerState = nullptr);
    void SetMaterialParam(int32 index, const jName& name, const jTexture* texture, const jSamplerState* samplerState = nullptr);
	void SetMaterialParam(int32 index, const jName& name);
	void SetMaterialParam(int32 index, const jTexture* texture);
	void SetMaterialParam(int32 index, const jSamplerState* samplerState);
	void Clear() { Params.clear(); }

	std::vector<jMaterialParam> Params;
};

class jMaterial
{
public:
	jMaterial() {}
	~jMaterial() {}
};

//////////////////////////////////////////////////////////////////////////

struct jRenderTargetInfo
{
	jRenderTargetInfo() = default;
	jRenderTargetInfo(ETextureType textureType, ETextureFormat internalFormat, ETextureFormat format, EFormatType formatType, EDepthBufferType depthBufferType
		, int32 width, int32 height, int32 textureCount = 1, ETextureFilter magnification = ETextureFilter::LINEAR
		, ETextureFilter minification = ETextureFilter::LINEAR, bool isGenerateMipmap = false, bool isGenerateMipmapDepth = false, int32 sampleCount = 1)
		: TextureType(textureType), InternalFormat(internalFormat), Format(format), FormatType(formatType), DepthBufferType(depthBufferType)
		, Width(width), Height(height), TextureCount(textureCount), Magnification(magnification), Minification(minification), IsGenerateMipmap(isGenerateMipmap)
		, IsGenerateMipmapDepth(isGenerateMipmapDepth), SampleCount(sampleCount)
	{}

	size_t GetHash() const
	{
		size_t result = 0;
		hash_combine(result, TextureType);
		hash_combine(result, InternalFormat);
		hash_combine(result, Format);
		hash_combine(result, FormatType);
		hash_combine(result, DepthBufferType);
		hash_combine(result, Width);
		hash_combine(result, Height);
		hash_combine(result, TextureCount);
		hash_combine(result, Magnification);
		hash_combine(result, Minification);
		hash_combine(result, IsGenerateMipmap);
		hash_combine(result, IsGenerateMipmapDepth);
		hash_combine(result, SampleCount);
		return result;
	}

	ETextureType TextureType = ETextureType::TEXTURE_2D;
	ETextureFormat InternalFormat = ETextureFormat::RGB;
	ETextureFormat Format = ETextureFormat::RGB;
	EFormatType FormatType = EFormatType::BYTE;
	EDepthBufferType DepthBufferType = EDepthBufferType::DEPTH16;
	int32 Width = 0;
	int32 Height = 0;
	int32 TextureCount = 1;
	ETextureFilter Magnification = ETextureFilter::LINEAR;
	ETextureFilter Minification = ETextureFilter::LINEAR;
	bool IsGenerateMipmap = false;
	bool IsGenerateMipmapDepth = false;
	int32 SampleCount = 1;
};

struct jRenderTarget : public std::enable_shared_from_this<jRenderTarget>
{
	virtual ~jRenderTarget() {}

	virtual jTexture* GetTexture(int32 index = 0) const { return Textures[index].get(); }
	virtual jTexture* GetTextureDepth(int32 index = 0) const { return TextureDepth.get(); }
	virtual ETextureType GetTextureType() const { return Info.TextureType; }
	virtual bool SetDepthAttachment(const std::shared_ptr<jTexture>& InDepth) { TextureDepth = InDepth; return true; }
	virtual void SetDepthMipLevel(int32 InLevel) {}

	virtual bool Begin(int index = 0, bool mrt = false) const { return true; };
	virtual void End() const {}

	jRenderTargetInfo Info;
	std::vector<std::shared_ptr<jTexture> > Textures;
	std::shared_ptr<jTexture> TextureDepth;
};

struct jQueryTime
{
	virtual ~jQueryTime() {}

	uint64 TimeStamp = 0;
};

struct jQueryPrimitiveGenerated
{
	virtual ~jQueryPrimitiveGenerated() {}

	uint64 NumOfGeneratedPrimitives = 0;

	void Begin() const;
	void End() const;
	uint64 GetResult();
};

struct jSamplerStateInfo
{
	size_t GetHash() const
	{
		size_t result = 0;
		hash_combine(result, Minification);
		hash_combine(result, Magnification);
		hash_combine(result, AddressU);
		hash_combine(result, AddressV);
		hash_combine(result, AddressW);
		hash_combine(result, MipLODBias);
		hash_combine(result, MaxAnisotropy);
		hash_combine(result, BorderColor.x);
		hash_combine(result, BorderColor.y);
		hash_combine(result, BorderColor.z);
		hash_combine(result, BorderColor.w);
		hash_combine(result, MinLOD);
		hash_combine(result, MaxLOD);
		return result;
	}

	ETextureFilter Minification = ETextureFilter::NEAREST;
	ETextureFilter Magnification = ETextureFilter::NEAREST;
	ETextureAddressMode AddressU = ETextureAddressMode::CLAMP_TO_EDGE;
	ETextureAddressMode AddressV = ETextureAddressMode::CLAMP_TO_EDGE;
	ETextureAddressMode AddressW = ETextureAddressMode::CLAMP_TO_EDGE;
	float MipLODBias = 0.0f;
	float MaxAnisotropy = 1.0f;			// if you anisotropy filtering tuned on, set this variable greater than 1.
	ETextureComparisonMode TextureComparisonMode = ETextureComparisonMode::NONE;
	EComparisonFunc ComparisonFunc = EComparisonFunc::LESS;
	Vector4 BorderColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);
	float MinLOD = -FLT_MAX;
	float MaxLOD = FLT_MAX;
};

struct jSamplerState : public std::enable_shared_from_this<jSamplerState>
{
	jSamplerState(const jSamplerStateInfo& info)
		: Info(info)
	{ }
	virtual ~jSamplerState() {}
	const jSamplerStateInfo Info;
};

class jRHI
{
public:
	jRHI();
	virtual ~jRHI() {}

	virtual jSamplerState* CreateSamplerState(const jSamplerStateInfo& info) const { return nullptr; }
	virtual void ReleaseSamplerState(jSamplerState* samplerState) const {}
	virtual void BindSamplerState(int32 index, const jSamplerState* samplerState) const {}
	virtual void SetClear(ERenderBufferType typeBit) const {}
	virtual void SetClearColor(float r, float g, float b, float a) const {}
	virtual void SetClearColor(Vector4 rgba) const {}
	virtual void SetClearBuffer(ERenderBufferType typeBit, const float* value, int32 bufferIndex) const {}
	virtual void SetClearBuffer(ERenderBufferType typeBit, const int32* value, int32 bufferIndex) const {}
	virtual void SetRenderTarget(const jRenderTarget* rt, int32 index = 0, bool mrt = false) const {}
	virtual void SetDrawBuffers(const std::initializer_list<EDrawBufferType>& list) const {}
	virtual jVertexBuffer* CreateVertexBuffer(const std::shared_ptr<jVertexStreamData>& streamData) const { return nullptr; }
	virtual jIndexBuffer* CreateIndexBuffer(const std::shared_ptr<jIndexStreamData>& streamData) const { return nullptr; }
	virtual void UpdateVertexBuffer(jVertexBuffer* vb, const std::shared_ptr<jVertexStreamData>& streamData) const {}
	virtual void UpdateVertexBuffer(jVertexBuffer* vb, IStreamParam* streamParam, int32 streamParamIndex) const {}
	virtual void BindVertexBuffer(const jVertexBuffer* vb, const jShader* shader) const {}
	virtual void BindIndexBuffer(const jIndexBuffer* ib, const jShader* shader) const {}
	virtual void MapBufferdata(IBuffer* buffer) const;
	virtual void SetTextureFilter(ETextureType type, ETextureFilterTarget target, ETextureFilter filter) const {}
	virtual void SetTextureWrap(int flag) const {}
	virtual void SetTexture(int32 index, const jTexture* texture) const {}
	virtual void DrawArrays(EPrimitiveType type, int32 vertStartIndex, int32 vertCount) const {}
	virtual void DrawArraysInstanced(EPrimitiveType type, int32 vertStartIndex, int32 vertCount, int32 instanceCount) const {}
	virtual void DrawElements(EPrimitiveType type, int32 elementSize, int32 startIndex, int32 count) const {}
	virtual void DrawElementsInstanced(EPrimitiveType type, int32 elementSize, int32 startIndex, int32 count, int32 instanceCount) const {}
	virtual void DrawElementsBaseVertex(EPrimitiveType type, int elementSize, int32 startIndex, int32 count, int32 baseVertexIndex) const {}
	virtual void DrawElementsInstancedBaseVertex(EPrimitiveType type, int elementSize, int32 startIndex, int32 count, int32 baseVertexIndex, int32 instanceCount) const {}
	virtual void DispatchCompute(uint32 numGroupsX, uint32 numGroupsY, uint32 numGroupsZ) const {}
	virtual void EnableDepthBias(bool enable, EPolygonMode polygonMode = EPolygonMode::FILL) const {}
	virtual void SetDepthBias(float constant, float slope) const {}
	virtual void SetShader(const jShader* shader) const {}
	virtual jShader* CreateShader(const jShaderInfo& shaderInfo) const { return nullptr; }
	virtual bool CreateShader(jShader* OutShader, const jShaderInfo& shaderInfo) const { return false;  }
	virtual void ReleaseShader(jShader* shader) const {}
	virtual void SetViewport(int32 x, int32 y, int32 width, int32 height) const {}
	virtual void SetViewport(const jViewport& viewport) const {}
	virtual void SetViewportIndexed(int32 index, float x, float y, float width, float height) const {}
	virtual void SetViewportIndexed(int32 index, const jViewport& viewport) const {}
	virtual void SetViewportIndexedArray(int32 startIndex, int32 count, const jViewport* viewports) const {}
	virtual bool SetUniformbuffer(const jName& name, const Matrix& InData, const jShader* InShader) const { return false; }
	virtual bool SetUniformbuffer(const jName& name, const int InData, const jShader* InShader) const { return false; }
	virtual bool SetUniformbuffer(const jName& name, const uint32 InData, const jShader* InShader) const { return false; }
	FORCEINLINE virtual bool SetUniformbuffer(jName name, const bool InData, const jShader* InShader) const { return SetUniformbuffer(name, (int32)InData, InShader); }
	virtual bool SetUniformbuffer(const jName& name, const float InData, const jShader* InShader) const { return false; }
	virtual bool SetUniformbuffer(const jName& name, const Vector2& InData, const jShader* InShader) const { return false; }
	virtual bool SetUniformbuffer(const jName& name, const Vector& InData, const jShader* InShader) const { return false; }
	virtual bool SetUniformbuffer(const jName& name, const Vector4& InData, const jShader* InShader) const { return false; }
	virtual bool SetUniformbuffer(const jName& name, const Vector2i& InData, const jShader* InShader) const { return false; }
	virtual bool SetUniformbuffer(const jName& name, const Vector3i& InData, const jShader* InShader) const { return false; }
	virtual bool SetUniformbuffer(const jName& name, const Vector4i& InData, const jShader* InShader) const { return false; }
	virtual bool GetUniformbuffer(Matrix& outResult, const jName& name, const jShader* shader) const { return false; }
	virtual bool GetUniformbuffer(int& outResult, const jName& name, const jShader* shader) const { return false; }
	virtual bool GetUniformbuffer(uint32& outResult, const jName& name, const jShader* shader) const { return false; }
	virtual bool GetUniformbuffer(float& outResult, const jName& name, const jShader* shader) const { return false; }
	virtual bool GetUniformbuffer(Vector2& outResult, const jName& name, const jShader* shader) const { return false; }
	virtual bool GetUniformbuffer(Vector& outResult, const jName& name, const jShader* shader) const { return false; }
	virtual bool GetUniformbuffer(Vector4& outResult, const jName& name, const jShader* shader) const { return false; }
	virtual bool GetUniformbuffer(Vector2i& outResult, const jName& name, const jShader* shader) const { return false; }
	virtual bool GetUniformbuffer(Vector3i& outResult, const jName& name, const jShader* shader) const { return false; }
	virtual bool GetUniformbuffer(Vector4i& outResult, const jName& name, const jShader* shader) const { return false; }
	virtual jTexture* CreateNullTexture() const { return nullptr; }
	virtual jTexture* CreateTextureFromData(void* data, int32 width, int32 height, bool sRGB
		, EFormatType dataType = EFormatType::UNSIGNED_BYTE, ETextureFormat textureFormat = ETextureFormat::RGBA, bool createMipmap = false) const { return nullptr; }
	virtual jTexture* CreateCubeTextureFromData(std::vector<void*> faces, int32 width, int32 height, bool sRGB
		, EFormatType dataType = EFormatType::UNSIGNED_BYTE, ETextureFormat textureFormat = ETextureFormat::RGBA, bool createMipmap = false) const { return nullptr; }
	virtual int32 SetMatetrial(const jMaterialData* materialData, const jShader* shader, int32 baseBindingIndex = 0) const { return baseBindingIndex; }
	virtual void EnableCullFace(bool enable) const {}
	virtual void SetFrontFace(EFrontFace frontFace) const {}
	virtual void EnableCullMode(ECullMode cullMode) const {}
	virtual jRenderTarget* CreateRenderTarget(const jRenderTargetInfo& info) const { return nullptr; }
	virtual void EnableDepthTest(bool enable) const {}
	virtual void EnableBlend(bool enable) const {}
	virtual void SetBlendFunc(EBlendSrc src, EBlendDest dest) const {}
	virtual void SetBlendFuncRT(EBlendSrc src, EBlendDest dest, int32 rtIndex = 0) const {}
	virtual void SetBlendEquation(EBlendEquation func) const {}
	virtual void SetBlendEquation(EBlendEquation func, int32 rtIndex) const {}
	virtual void SetBlendColor(float r, float g, float b, float a) const {}
	virtual void EnableStencil(bool enable) const {}
	virtual void SetStencilOpSeparate(EFace face, EStencilOp sFail, EStencilOp dpFail, EStencilOp dpPass) const {}
	virtual void SetStencilFunc(EComparisonFunc func, int32 ref, uint32 mask) const {}
	virtual void SetDepthFunc(EComparisonFunc func) const {}
	virtual void SetDepthMask(bool enable) const {}
	virtual void SetColorMask(bool r, bool g, bool b, bool a) const {}
	virtual IUniformBufferBlock* CreateUniformBufferBlock(const char* blockname) const { return nullptr; }
	virtual IShaderStorageBufferObject* CreateShaderStorageBufferObject(const char* blockname) const { return nullptr; }
	virtual IAtomicCounterBuffer* CreateAtomicCounterBuffer(const char* name, int32 bindingPoint) const { return nullptr; }
	virtual ITransformFeedbackBuffer* CreateTransformFeedbackBuffer(const char* name) const { return nullptr; }
	virtual void EnableSRGB(bool enable) const {  }
	virtual void EnableDepthClip(bool enable) const {  }
	virtual void BeginDebugEvent(const char* name) const {}
	virtual void EndDebugEvent() const {}
	virtual void GenerateMips(const jTexture* texture) const {}
	virtual jQueryTime* CreateQueryTime() const { return nullptr;  }
	virtual void ReleaseQueryTime(jQueryTime* queryTime) const {}
	virtual void QueryTimeStamp(const jQueryTime* queryTimeStamp) const {}
	virtual bool IsQueryTimeStampResult(const jQueryTime* queryTimeStamp, bool isWaitUntilAvailable) const { return false; }
	virtual void GetQueryTimeStampResult(jQueryTime* queryTimeStamp) const {}
	virtual void BeginQueryTimeElapsed(const jQueryTime* queryTimeElpased) const {}
	virtual void EndQueryTimeElapsed(const jQueryTime* queryTimeElpased) const {}
	virtual void EnableWireframe(bool enable) const {}
	virtual void SetImageTexture(int32 index, const jTexture* texture, EImageTextureAccessType type) const {}
	virtual void SetPolygonMode(EFace face, EPolygonMode mode = EPolygonMode::FILL) {}
	virtual jQueryPrimitiveGenerated* CreateQueryPrimitiveGenerated() const { return nullptr; }
	virtual void ReleaseQueryPrimitiveGenerated(jQueryPrimitiveGenerated* query) const {}
	virtual void BeginQueryPrimitiveGenerated(const jQueryPrimitiveGenerated* query) const {}
	virtual void EndQueryPrimitiveGenerated() const {}
	virtual void GetQueryPrimitiveGeneratedResult(jQueryPrimitiveGenerated* query) const {}
	virtual void EnableRasterizerDiscard(bool enable) const {}
	virtual void SetTextureMipmapLevelLimit(ETextureType type, int32 baseLevel, int32 maxLevel) const {}
	virtual void EnableMultisample(bool enable) const {}
	virtual void SetCubeMapSeamless(bool enable) const {}
	virtual void SetLineWidth(float width) const {}
};

// Not thred safe
#define SET_UNIFORM_BUFFER_STATIC(Name, CurrentData, Shader) \
{\
	static jUniformBuffer<typename std::decay<decltype(CurrentData)>::type> temp(jName(Name), CurrentData);\
	temp.Data = CurrentData;\
	temp.SetUniformbuffer(Shader);\
}

// Not thread safe
#define GET_UNIFORM_BUFFER_STATIC(ResultData, Name, Shader) \
{\
	static jName tempName(Name);\
	EUniformType UniformType = ConvertUniformType<typename std::decay<decltype(ResultData)>::type>();\
	JASSERT(UniformType != EUniformType::NONE);\
	g_rhi->GetUniformbuffer(ResultData, tempName, Shader);\
}

struct jScopeDebugEvent final
{
	jScopeDebugEvent(const jRHI* rhi, const char* name)
		: RHI(rhi)
	{
		JASSERT(RHI);
		RHI->BeginDebugEvent(name);
	}
	~jScopeDebugEvent()
	{
		RHI->EndDebugEvent();
	}

	const jRHI* RHI = nullptr;
};
#define SCOPE_DEBUG_EVENT(rhi, name) jScopeDebugEvent scope_debug_event(rhi, name);

//////////////////////////////////////////////////////////////////////////
#define DECLARE_UNIFORMBUFFER(EnumType, DataType) \
template <> struct jUniformBuffer<DataType> : public IUniformBuffer\
{\
	jUniformBuffer() = default;\
	jUniformBuffer(const jName& name, const DataType& data)\
		: IUniformBuffer(name), Data(data)\
	{}\
	static constexpr EUniformType Type = EnumType;\
	FORCEINLINE virtual EUniformType GetType() const { return Type; }\
	FORCEINLINE virtual void SetUniformbuffer(const jShader* InShader) const { g_rhi->SetUniformbuffer(Name, Data, InShader); }\
	DataType Data;\
};

DECLARE_UNIFORMBUFFER(EUniformType::MATRIX, Matrix);
DECLARE_UNIFORMBUFFER(EUniformType::BOOL, bool);
DECLARE_UNIFORMBUFFER(EUniformType::INT, int);
DECLARE_UNIFORMBUFFER(EUniformType::FLOAT, float);
DECLARE_UNIFORMBUFFER(EUniformType::VECTOR2, Vector2);
DECLARE_UNIFORMBUFFER(EUniformType::VECTOR3, Vector);
DECLARE_UNIFORMBUFFER(EUniformType::VECTOR4, Vector4);
DECLARE_UNIFORMBUFFER(EUniformType::VECTOR2I, Vector2i);
DECLARE_UNIFORMBUFFER(EUniformType::VECTOR3I, Vector3i);
DECLARE_UNIFORMBUFFER(EUniformType::VECTOR4I, Vector4i);

jName GetCommonTextureName(int32 index);
jName GetCommonTextureSRGBName(int32 index);