#pragma once

#include "jRHI.h"
#include "jRHIType.h"
#include "jShader.h"

struct jVertexStream_OpenGL
{
	jName Name;
	unsigned int BufferID = 0;
	unsigned int Count;
	EBufferType BufferType = EBufferType::STATIC;
	EBufferElementType ElementType = EBufferElementType::BYTE;
	bool Normalized = false;
	int32 Stride = 0;
	size_t Offset = 0;
	int32 InstanceDivisor = 0;
};

struct jVertexBuffer_OpenGL : public jVertexBuffer
{
	uint32 VAO = 0;
	std::vector<jVertexStream_OpenGL> Streams;

	virtual void Bind(const jShader* shader) const override;
};

struct jIndexBuffer_OpenGL : public jIndexBuffer
{
	unsigned int BufferID = 0;

	virtual void Bind(const jShader* shader) const override;
};

struct jTexture_OpenGL : public jTexture
{
	uint32 TextureID = 0;
};

struct jRenderTarget_OpenGL : public jRenderTarget
{
	std::vector<uint32> fbos;
	std::vector<uint32> rbos;
	std::vector<uint32> drawBuffers;

	uint32 mrt_fbo = 0;
	uint32 mrt_rbo = 0;
	std::vector<uint32> mrt_drawBuffers;

	virtual bool Begin(int index = 0, bool mrt = false) const override;
	virtual void End() const override;
	virtual bool SetDepthAttachment(const std::shared_ptr<jTexture>& InDepth) override;
	virtual void SetDepthMipLevel(int32 InLevel) override;
};

struct jUniformBufferBlock_OpenGL : public IUniformBufferBlock
{
	using IUniformBufferBlock::IUniformBufferBlock;
	using IUniformBufferBlock::UpdateBufferData;
	virtual ~jUniformBufferBlock_OpenGL()
	{
		glDeleteBuffers(1, &UBO);
	}

	uint32 BindingPoint = -1;
	uint32 UBO = -1;
	virtual void Init() override;
	virtual void Bind(const jShader* shader) const override;
	virtual void UpdateBufferData(const void* newData, size_t size) override;
	virtual void ClearBuffer(int32 clearValue) override;
};

struct jShaderStorageBufferObject_OpenGL : public IShaderStorageBufferObject
{
	using IShaderStorageBufferObject::IShaderStorageBufferObject;
	using IShaderStorageBufferObject::GetBufferData;
	using IShaderStorageBufferObject::UpdateBufferData;

	virtual ~jShaderStorageBufferObject_OpenGL()
	{
		glDeleteBuffers(1, &SSBO);
	}

	uint32 BindingPoint = -1;
	uint32 SSBO = -1;
	virtual void Init() override;
	virtual void Bind(const jShader* shader) const override;
	virtual void UpdateBufferData(void* newData, size_t size) override;
	virtual void ClearBuffer(int32 clearValue) override;
	virtual void GetBufferData(void* newData, size_t size) override;
};

struct jAtomicCounterBuffer_OpenGL : public IAtomicCounterBuffer
{
	using IAtomicCounterBuffer::IAtomicCounterBuffer;
	virtual ~jAtomicCounterBuffer_OpenGL()
	{
		glDeleteBuffers(1, &ACBO);
	}

	uint32 ACBO = -1;
	virtual void Init() override;
	virtual void Bind(const jShader* shader) const override;
	virtual void UpdateBufferData(void* newData, size_t size) override;
	using IAtomicCounterBuffer::GetBufferData;
	virtual void GetBufferData(void* newData, size_t size) override;
	virtual void ClearBuffer(int32 clearValue) override;
};

struct jTransformFeedbackBuffer_OpenGL : public ITransformFeedbackBuffer
{
	using ITransformFeedbackBuffer::ITransformFeedbackBuffer;
	virtual ~jTransformFeedbackBuffer_OpenGL()
	{
		glDeleteBuffers(1, &TFBO);
	}

	uint32 TFBO = -1;
	int32 DepthCheck = 0;
	virtual void Init() override;
	virtual void Bind(const jShader* shader) const override;
	virtual void UpdateBufferData(void* newData, size_t size) override;
	using ITransformFeedbackBuffer::GetBufferData;
	virtual void GetBufferData(void* newData, size_t size) override;
	virtual void ClearBuffer(int32 clearValue) override;
	virtual void UpdateVaryingsToShader(const std::vector<std::string>& varyings, const jShader* shader) override;
	virtual void ClearVaryingsToShader(const jShader* shader) override;
	virtual void Begin(EPrimitiveType type) override;
	virtual void End() override;
	virtual void Pause() override;

private:
	void LinkProgram(const jShader* shader) const;
};


struct jQueryTime_OpenGL : public jQueryTime
{
	virtual ~jQueryTime_OpenGL() {}

	uint32 QueryId = 0;
};

struct jQueryPrimitiveGenerated_OpenGL : public jQueryPrimitiveGenerated
{
	virtual ~jQueryPrimitiveGenerated_OpenGL() {}

	uint32 QueryId = 0;
};

struct jSamplerState_OpenGL : public jSamplerState
{
	using jSamplerState::jSamplerState;
	virtual ~jSamplerState_OpenGL() {}
	uint32 SamplerId = 0;
};

class jRHI_OpenGL : public jRHI
{
public:
	jRHI_OpenGL();
	~jRHI_OpenGL();

	virtual jSamplerState* CreateSamplerState(const jSamplerStateInfo& info) const override;
	virtual void ReleaseSamplerState(jSamplerState* samplerState) const override;
	virtual void BindSamplerState(int32 index, const jSamplerState* samplerState) const override;
	virtual jVertexBuffer* CreateVertexBuffer(const std::shared_ptr<jVertexStreamData>& streamData) const override;
	virtual jIndexBuffer* CreateIndexBuffer(const std::shared_ptr<jIndexStreamData>& streamData) const override;
	virtual void BindVertexBuffer(const jVertexBuffer* vb, const jShader* shader) const override;
	virtual void BindIndexBuffer(const jIndexBuffer* ib, const jShader* shader) const override;
	virtual void MapBufferdata(IBuffer* buffer) const override;
	virtual void DrawArrays(EPrimitiveType type, int vertStartIndex, int vertCount) const override;
	virtual void DrawArraysInstanced(EPrimitiveType type, int32 vertStartIndex, int32 vertCount, int32 instanceCount) const override;
	virtual void DrawElements(EPrimitiveType type, int32 elementSize, int32 startIndex, int32 count) const override;
	virtual void DrawElementsInstanced(EPrimitiveType type, int32 elementSize, int32 startIndex, int32 count, int32 instanceCount) const override;
	virtual void DrawElementsBaseVertex(EPrimitiveType type, int elementSize, int32 startIndex, int32 count, int32 baseVertexIndex) const override;
	virtual void DrawElementsInstancedBaseVertex(EPrimitiveType type, int elementSize, int32 startIndex, int32 count, int32 baseVertexIndex, int32 instanceCount) const override;
	virtual void DispatchCompute(uint32 numGroupsX, uint32 numGroupsY, uint32 numGroupsZ) const override;
	virtual void EnableDepthBias(bool enable, EPolygonMode polygonMode = EPolygonMode::FILL) const override;
	virtual void SetDepthBias(float constant, float slope) const override;
	virtual void SetClear(ERenderBufferType typeBit) const override;
	virtual void SetClearColor(float r, float g, float b, float a) const override;
	virtual void SetClearColor(Vector4 rgba) const override;
	virtual void SetClearBuffer(ERenderBufferType typeBit, const float* value, int32 bufferIndex) const override;
	virtual void SetClearBuffer(ERenderBufferType typeBit, const int32* value, int32 bufferIndex) const override;
	virtual void SetShader(const jShader* shader) const override;
	virtual jShader* CreateShader(const jShaderInfo& shaderInfo) const override;
	virtual bool CreateShader(jShader* OutShader, const jShaderInfo& shaderInfo) const override;
	virtual void ReleaseShader(jShader* shader) const override;
	virtual jTexture* CreateNullTexture() const override;
	virtual jTexture* CreateTextureFromData(void* data, int32 width, int32 height, bool sRGB
		, EFormatType dataType = EFormatType::UNSIGNED_BYTE, ETextureFormat textureFormat = ETextureFormat::RGBA, bool createMipmap = false) const override;
	virtual jTexture* CreateCubeTextureFromData(std::vector<void*> faces, int32 width, int32 height, bool sRGB
		, EFormatType dataType = EFormatType::UNSIGNED_BYTE, ETextureFormat textureFormat = ETextureFormat::RGBA, bool createMipmap = false) const override;

	virtual bool SetUniformbuffer(const jName& name, const Matrix& InData, const jShader* InShader) const override;
	virtual bool SetUniformbuffer(const jName& name, const int InData, const jShader* InShader) const override;
	virtual bool SetUniformbuffer(const jName& name, const uint32 InData, const jShader* InShader) const override;
	virtual bool SetUniformbuffer(const jName& name, const float InData, const jShader* InShader) const override;
	virtual bool SetUniformbuffer(const jName& name, const Vector2& InData, const jShader* InShader) const override;
	virtual bool SetUniformbuffer(const jName& name, const Vector& InData, const jShader* InShader) const override;
	virtual bool SetUniformbuffer(const jName& name, const Vector4& InData, const jShader* InShader) const override;
	virtual bool SetUniformbuffer(const jName& name, const Vector2i& InData, const jShader* InShader) const override;
	virtual bool SetUniformbuffer(const jName& name, const Vector3i& InData, const jShader* InShader) const override;
	virtual bool SetUniformbuffer(const jName& name, const Vector4i& InData, const jShader* InShader) const override;
	virtual bool GetUniformbuffer(Matrix& outResult, const jName& name, const jShader* shader) const override;
	virtual bool GetUniformbuffer(int& outResult, const jName& name, const jShader* shader) const override;
	virtual bool GetUniformbuffer(uint32& outResult, const jName& name, const jShader* shader) const override;
	virtual bool GetUniformbuffer(float& outResult, const jName& name, const jShader* shader) const override;
	virtual bool GetUniformbuffer(Vector2& outResult, const jName& name, const jShader* shader) const override;
	virtual bool GetUniformbuffer(Vector& outResult, const jName& name, const jShader* shader) const override;
	virtual bool GetUniformbuffer(Vector4& outResult, const jName& name, const jShader* shader) const override;
	virtual bool GetUniformbuffer(Vector2i& outResult, const jName& name, const jShader* shader) const override;
	virtual bool GetUniformbuffer(Vector3i& outResult, const jName& name, const jShader* shader) const override;
	virtual bool GetUniformbuffer(Vector4i& outResult, const jName& name, const jShader* shader) const override;

	virtual int32 SetMatetrial(const jMaterialData* materialData, const jShader* shader, int32 baseBindingIndex = 0) const override;
	virtual void SetTexture(int32 index, const jTexture* texture) const override;
	virtual void SetTextureFilter(ETextureType type, ETextureFilterTarget target, ETextureFilter filter) const override;
	virtual void EnableCullFace(bool enable) const override;
	virtual void SetFrontFace(EFrontFace frontFace) const override;
	virtual void EnableCullMode(ECullMode cullMode) const override;
	virtual jRenderTarget* CreateRenderTarget(const jRenderTargetInfo& info) const override;
	virtual void EnableDepthTest(bool enable) const override;
	virtual void SetRenderTarget(const jRenderTarget* rt, int32 index = 0, bool mrt = false) const override;
	virtual void SetDrawBuffers(const std::initializer_list<EDrawBufferType>& list) const override;
	virtual void UpdateVertexBuffer(jVertexBuffer* vb, const std::shared_ptr<jVertexStreamData>& streamData) const override;
	virtual void UpdateVertexBuffer(jVertexBuffer* vb, IStreamParam* streamParam, int32 streamParamIndex) const override;
	virtual void EnableBlend(bool enable) const override;
	virtual void SetBlendFunc(EBlendSrc src, EBlendDest dest) const override;
	virtual void SetBlendFuncRT(EBlendSrc src, EBlendDest dest, int32 rtIndex = 0) const override;
	virtual void SetBlendEquation(EBlendEquation func) const override;
	virtual void SetBlendEquation(EBlendEquation func, int32 rtIndex) const override;
	virtual void SetBlendColor(float r, float g, float b, float a) const override;
	virtual void EnableStencil(bool enable) const override;
	virtual void SetStencilOpSeparate(EFace face, EStencilOp sFail, EStencilOp dpFail, EStencilOp dpPass) const override;
	virtual void SetStencilFunc(EComparisonFunc func, int32 ref, uint32 mask) const override;
	virtual void SetDepthFunc(EComparisonFunc func) const override;
	virtual void SetDepthMask(bool enable) const override;
	virtual void SetColorMask(bool r, bool g, bool b, bool a) const override;
	virtual IUniformBufferBlock* CreateUniformBufferBlock(const char* blockname) const override;
	virtual void EnableSRGB(bool enable) const override;
	virtual IShaderStorageBufferObject* CreateShaderStorageBufferObject(const char* blockname) const override;
	virtual IAtomicCounterBuffer* CreateAtomicCounterBuffer(const char* name, int32 bindingPoint) const override;
	virtual ITransformFeedbackBuffer* CreateTransformFeedbackBuffer(const char* name) const override;
	virtual void SetViewport(int32 x, int32 y, int32 width, int32 height) const override;
	virtual void SetViewport(const jViewport& viewport) const override;
	virtual void SetViewportIndexed(int32 index, float x, float y, float width, float height) const override;
	virtual void SetViewportIndexed(int32 index, const jViewport& viewport) const override;
	virtual void SetViewportIndexedArray(int32 startIndex, int32 count, const jViewport* viewports) const override;
	virtual void EnableDepthClip(bool enable) const override;
	virtual void BeginDebugEvent(const char* name) const override;
	virtual void EndDebugEvent() const override;
	virtual void GenerateMips(const jTexture* texture) const override;
	virtual jQueryTime* CreateQueryTime() const override;
	virtual void ReleaseQueryTime(jQueryTime* queryTime) const override;
	virtual void QueryTimeStamp(const jQueryTime* queryTimeStamp) const override;
	virtual bool IsQueryTimeStampResult(const jQueryTime* queryTimeStamp, bool isWaitUntilAvailable) const override;
	virtual void GetQueryTimeStampResult(jQueryTime* queryTimeStamp) const override;
	virtual void BeginQueryTimeElapsed(const jQueryTime* queryTimeElpased) const override;
	virtual void EndQueryTimeElapsed(const jQueryTime* queryTimeElpased) const override;
	virtual void EnableWireframe(bool enable) const override;
	virtual void SetImageTexture(int32 index, const jTexture* texture, EImageTextureAccessType type) const override;
	virtual void SetPolygonMode(EFace face, EPolygonMode mode = EPolygonMode::FILL) override;
	virtual jQueryPrimitiveGenerated* CreateQueryPrimitiveGenerated() const override;
	virtual void ReleaseQueryPrimitiveGenerated(jQueryPrimitiveGenerated* query) const override;
	virtual void BeginQueryPrimitiveGenerated(const jQueryPrimitiveGenerated* query) const override;
	virtual void EndQueryPrimitiveGenerated() const override;
	virtual void GetQueryPrimitiveGeneratedResult(jQueryPrimitiveGenerated* query) const override;
	virtual void EnableRasterizerDiscard(bool enable) const override;
	virtual void SetTextureMipmapLevelLimit(ETextureType type, int32 baseLevel, int32 maxLevel) const override;
	virtual void EnableMultisample(bool enable) const override;
	virtual void SetCubeMapSeamless(bool enable) const override;
	virtual void SetLineWidth(float width) const override;

	//////////////////////////////////////////////////////////////////////////
	int32 GetUniformLocation(uint32 InProgram, const char* name) const;
	int32 GetAttributeLocation(uint32 InProgram, const char* name) const;
};

extern jRHI_OpenGL* g_rhi_gl;

//////////////////////////////////////////////////////////////////////////
#define USE_CACHED_LOCATION_HASH 0
#define USE_CACHED_LOCATION_ARRAY 1		// The way using an array is fastest

// UniformBuffer location 을 얻어오는 functor, TTrayGetLocation의 template type으로 사용
struct jUniformbufferLocationGetter
{
	FORCEINLINE static int32 GetLocation(uint32 program, const jName& name)
	{
		return g_rhi_gl->GetUniformLocation(program, name.ToStr());
	}
};

// Attribute location 을 얻어오는 functor, TTrayGetLocation의 template type으로 사용
struct jAttributeLocationGatter
{
	FORCEINLINE static int32 GetLocation(uint32 program, const jName& name)
	{
		return g_rhi_gl->GetAttributeLocation(program, name.ToStr());
	}
};

// Shader 의 Location 정보를 얻어오고, 캐싱해줌.
template <typename T>
struct TTryGetLocation
{
#if USE_CACHED_LOCATION_HASH
	mutable std::unordered_map<uint32, int32> NameLocationMap;		// <jNameHash, location>
#elif USE_CACHED_LOCATION_ARRAY
	struct jLocationInfo
	{
		uint32 hash = -1;
		int32 location = -1;
	};
	mutable std::vector<jLocationInfo> locations;
#endif

	FORCEINLINE int32 TryGetLocation(uint32 program, const jName& name) const
	{
#if USE_CACHED_LOCATION_HASH
		const auto it_find = NameLocationMap.find(name.GetNameHash());
		const bool found = (it_find != NameLocationMap.end());
		if (found)
		{
			return it_find->second;
		}

		const int32 Location = T::GetLocation(program, name);
		NameLocationMap[name.GetNameHash()] = Location;

		return Location;
#elif USE_CACHED_LOCATION_ARRAY
		const auto hash = name.GetNameHash();
		const size_t size = locations.size();
		for (size_t i = 0; i < size; ++i)
		{
			if (locations[i].hash == hash)
			{
				return locations[i].location;
			}
		}

		const int32 Location = T::GetLocation(program, name);
		locations.push_back({ hash , Location });
		return Location;
#else
		return T::GetLocation(program, name);
#endif
	}

	FORCEINLINE void ClearLocation()
	{
#if USE_CACHED_LOCATION_HASH
		NameLocationMap.clear();
#elif USE_CACHED_LOCATION_ARRAY
		locations.clear();
#else
#endif
	}
};

using jTryGetUniformLocation = TTryGetLocation<jUniformbufferLocationGetter>;
using jTryGetAttributeLocation = TTryGetLocation<jAttributeLocationGatter>;

struct jShader_OpenGL : public jShader
{
	uint32 program = 0;

	// 이 Shader 와 연관된 Location 정보를 얻어오고, 캐싱해줌.
	jTryGetUniformLocation UniformLocationGatter;
	jTryGetAttributeLocation AttributeLocationGatter;

	FORCEINLINE int32 TryGetUniformLocation(const jName& name) const
	{
		return UniformLocationGatter.TryGetLocation(program, name);
	}
	FORCEINLINE int32 TryGetAttributeLocation(const jName& name) const
	{
		return AttributeLocationGatter.TryGetLocation(program, name);
	}
	FORCEINLINE void ClearAllLocations()
	{
		UniformLocationGatter.ClearLocation();
		AttributeLocationGatter.ClearLocation();
	}
};
