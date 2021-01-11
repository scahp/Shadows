#pragma once

#include "jRHI.h"
#include "jRHIType.h"
#include "jShader.h"

struct jVertexStream_OpenGL
{
	std::string Name;
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

struct jShader_OpenGL : public jShader
{
	unsigned int program = 0;
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

struct jQueryTime_OpenGL : public jQueryTime
{
	virtual ~jQueryTime_OpenGL() {}

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
	virtual void EnableDepthBias(bool enable) const override;
	virtual void SetDepthBias(float constant, float slope) const override;
	virtual void SetClear(ERenderBufferType typeBit) const override;
	virtual void SetClearColor(float r, float g, float b, float a) const override;
	virtual void SetClearColor(Vector4 rgba) const override;
	virtual void SetShader(const jShader* shader) const override;
	virtual jShader* CreateShader(const jShaderInfo& shaderInfo) const override;
	virtual bool CreateShader(jShader* OutShader, const jShaderInfo& shaderInfo) const override;
	virtual void ReleaseShader(jShader* shader) const override;
	virtual jTexture* CreateNullTexture() const override;
	virtual jTexture* CreateTextureFromData(void* data, int32 width, int32 height, bool sRGB
		, EFormatType dataType = EFormatType::UNSIGNED_BYTE, ETextureFormat textureFormat = ETextureFormat::RGBA) const override;
	virtual bool SetUniformbuffer(const IUniformBuffer* buffer, const jShader* shader) const override;
	virtual void SetMatetrial(jMaterialData* materialData, const jShader* shader, int32 baseBindingIndex = 0) const override;
	virtual void SetTexture(int32 index, const jTexture* texture) const override;
	virtual void SetTextureFilter(ETextureType type, ETextureFilterTarget target, ETextureFilter filter) const override;
	virtual void EnableCullFace(bool enable) const override;
	virtual jRenderTarget* CreateRenderTarget(const jRenderTargetInfo& info) const override;
	virtual void EnableDepthTest(bool enable) const override;
	virtual void SetRenderTarget(const jRenderTarget* rt, int32 index = 0, bool mrt = false) const override;
	virtual void SetDrawBuffers(const std::initializer_list<EDrawBufferType>& list) const override;
	virtual void UpdateVertexBuffer(jVertexBuffer* vb, const std::shared_ptr<jVertexStreamData>& streamData) const override;
	virtual void UpdateVertexBuffer(jVertexBuffer* vb, IStreamParam* streamParam, int32 streamParamIndex) const override;
	virtual void EnableBlend(bool enable) const override;
	virtual void SetBlendFunc(EBlendSrc src, EBlendDest dest) const override;
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
};

