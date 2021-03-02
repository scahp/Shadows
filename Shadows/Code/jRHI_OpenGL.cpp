#include "pch.h"
#include "jRHI_OpenGL.h"
#include "jFile.h"
#include "jRHIType.h"
#include "jShader.h"

//////////////////////////////////////////////////////////////////////////
// OpenGL utility functions
unsigned int GetPrimitiveType(EPrimitiveType type)
{
	unsigned int primitiveType = 0;
	switch (type)
	{
	case EPrimitiveType::POINTS:
		primitiveType = GL_POINTS;
		break;
	case EPrimitiveType::LINES:
		primitiveType = GL_LINES;
		break;
	case EPrimitiveType::LINES_ADJACENCY:
		primitiveType = GL_LINES_ADJACENCY;
		break;
	case EPrimitiveType::LINE_STRIP_ADJACENCY:
		primitiveType = GL_LINE_STRIP_ADJACENCY;
		break;
	case EPrimitiveType::TRIANGLES:
		primitiveType = GL_TRIANGLES;
		break;
	case EPrimitiveType::TRIANGLE_STRIP:
		primitiveType = GL_TRIANGLE_STRIP;
		break;
	case EPrimitiveType::TRIANGLES_ADJACENCY:
		primitiveType = GL_TRIANGLES_ADJACENCY;
		break;
	case EPrimitiveType::TRIANGLE_STRIP_ADJACENCY:
		primitiveType = GL_TRIANGLE_STRIP_ADJACENCY;
		break;
	}
	return primitiveType;
}

uint32 GetOpenGLTextureType(ETextureType textureType)
{
	uint32 result = 0;
	switch (textureType)
	{
	case ETextureType::TEXTURE_2D:
		result = GL_TEXTURE_2D;
		break;
	case ETextureType::TEXTURE_2D_ARRAY:
	case ETextureType::TEXTURE_2D_ARRAY_OMNISHADOW:
		result = GL_TEXTURE_2D;
		break;
	case ETextureType::TEXTURE_CUBE:
		result = GL_TEXTURE_CUBE_MAP;
		break;
	default:
		break;
	}
	return result;
}

uint32 GetOpenGLTextureFormat(ETextureFormat format)
{
	uint32 result = 0;
	switch (format)
	{
	case ETextureFormat::RGB:
		result = GL_RGB;
		break;
	case ETextureFormat::RGBA:
		result = GL_RGBA;
		break;
	case ETextureFormat::RG:
		result = GL_RG;
		break;
	case ETextureFormat::R:
		result = GL_RED;
		break;
	case ETextureFormat::R_INTEGER:
		result = GL_RED_INTEGER;
		break;
	case ETextureFormat::R32UI:
		result = GL_R32UI;
		break;
	case ETextureFormat::R32F:
		result = GL_R32F;
		break;
	case ETextureFormat::RG32F:
		result = GL_RG32F;
		break;
	case ETextureFormat::RGBA32F:
		result = GL_RGBA32F;
		break;
	case ETextureFormat::RGBA16F:
		result = GL_RGBA16F;
		break;
	case ETextureFormat::R11G11B10F:
		result = GL_R11F_G11F_B10F;
		break;
	case ETextureFormat::RGB16F:
		result = GL_RGB16F;
		break;
	case ETextureFormat::RGB32F:
		result = GL_RGB32F;
		break;
	case ETextureFormat::DEPTH:
		result = GL_DEPTH_COMPONENT;
		break;
	default:
		break;
	}
	return result;
}

uint32 GetOpenGLTextureFormatSimple(ETextureFormat format)
{
	uint32 result = 0;
	switch (format)
	{
	case ETextureFormat::RGB32F:
	case ETextureFormat::RGB16F:
	case ETextureFormat::R11G11B10F:
	case ETextureFormat::RGB:
		result = GL_RGB;
		break;
	case ETextureFormat::RGBA16F:
	case ETextureFormat::RGBA32F:
	case ETextureFormat::RGBA:
		result = GL_RGBA;
		break;
	case ETextureFormat::RG32F:
	case ETextureFormat::RG:
		result = GL_RG;
		break;
	case ETextureFormat::R:
	case ETextureFormat::R32F:
		result = GL_RED;
		break;
	default:
		break;
	}
	return result;
}

uint32 GetOpenGLFormatType(EFormatType format)
{
	uint32 result = 0;
	switch (format)
	{
	case EFormatType::BYTE:
		result = GL_BYTE;
		break;
	case EFormatType::UNSIGNED_BYTE:
		result = GL_UNSIGNED_BYTE;
		break;
	case EFormatType::INT:
		result = GL_INT;
		break;
	case EFormatType::UNSIGNED_INT:
		result = GL_UNSIGNED_INT;
		break;
	case EFormatType::FLOAT:
		result = GL_FLOAT;
		break;
	default:
		break;
	}

	return result;
}

uint32 GetOpenGLTextureFilterType(ETextureFilter filter)
{
	uint32 texFilter = 0;
	switch (filter)
	{
	case ETextureFilter::NEAREST:
		texFilter = GL_NEAREST;
		break;
	case ETextureFilter::LINEAR:
		texFilter = GL_LINEAR;
		break;
	case ETextureFilter::NEAREST_MIPMAP_NEAREST:
		texFilter = GL_NEAREST_MIPMAP_NEAREST;
		break;
	case ETextureFilter::LINEAR_MIPMAP_NEAREST:
		texFilter = GL_LINEAR_MIPMAP_NEAREST;
		break;
	case ETextureFilter::NEAREST_MIPMAP_LINEAR:
		texFilter = GL_NEAREST_MIPMAP_LINEAR;
		break;
	case ETextureFilter::LINEAR_MIPMAP_LINEAR:
		texFilter = GL_LINEAR_MIPMAP_LINEAR;
		break;
	default:
		break;
	}
	return texFilter;
}

uint32 GetOpenGLTextureAddressMode(ETextureAddressMode textureAddressMode)
{
	uint32 result = 0;
	switch (textureAddressMode)
	{
	case ETextureAddressMode::REPEAT:
		result = GL_REPEAT;
		break;
	case ETextureAddressMode::MIRRORED_REPEAT:
		result = GL_MIRRORED_REPEAT;
		break;
	case ETextureAddressMode::CLAMP_TO_EDGE:
		result = GL_CLAMP_TO_EDGE;
		break;
	case ETextureAddressMode::CLAMP_TO_BORDER:
		result = GL_CLAMP_TO_BORDER;
		break;
	default:
		break;
	}
	return result;
}

uint32 GetOpenGLComparisonFunc(EComparisonFunc func)
{
	unsigned int result = 0;
	switch (func)
	{
	case EComparisonFunc::NEVER:
		result = GL_NEVER;
		break;
	case EComparisonFunc::LESS:
		result = GL_LESS;
		break;
	case EComparisonFunc::EQUAL:
		result = GL_EQUAL;
		break;
	case EComparisonFunc::LEQUAL:
		result = GL_LEQUAL;
		break;
	case EComparisonFunc::GREATER:
		result = GL_GREATER;
		break;
	case EComparisonFunc::NOTEQUAL:
		result = GL_NOTEQUAL;
		break;
	case EComparisonFunc::GEQUAL:
		result = GL_GEQUAL;
		break;
	case EComparisonFunc::ALWAYS:
		result = GL_ALWAYS;
		break;
	}
	return result;
}

uint32 GetOpenGLTextureComparisonMode(ETextureComparisonMode mode)
{
	uint32 result = 0;
	switch (mode)
	{
	case ETextureComparisonMode::NONE:
		result = GL_NONE;
		break;
	case ETextureComparisonMode::COMPARE_REF_TO_TEXTURE:		// to use PCF filtering by using samplerXXShadow series.
		result = GL_COMPARE_REF_TO_TEXTURE;
		break;
	default:
		break;
	}
	return result;
}

uint32 GetOpenGLImageTextureAccessType(EImageTextureAccessType type)
{
	uint32 result = 0;
	switch (type)
	{
	case EImageTextureAccessType::READ_ONLY:
		result = GL_READ_ONLY;
		break;
	case EImageTextureAccessType::WRITE_ONLY:
		result = GL_WRITE_ONLY;
		break;
	case EImageTextureAccessType::READ_WRITE:
		result = GL_READ_WRITE;
		break;
	default:
		break;
	}
	return result;
}

bool GetDepthBufferFormatAndType(uint32& depthBufferFormat, uint32& depthBufferType, const EDepthBufferType& InType)
{
	switch (InType)
	{
	case EDepthBufferType::DEPTH16:
		depthBufferFormat = GL_DEPTH_COMPONENT16;
		depthBufferType = GL_DEPTH_ATTACHMENT;
		break;
	case EDepthBufferType::DEPTH24:
		depthBufferFormat = GL_DEPTH_COMPONENT24;
		depthBufferType = GL_DEPTH_ATTACHMENT;
		break;
	case EDepthBufferType::DEPTH32:
		depthBufferFormat = GL_DEPTH_COMPONENT32;
		depthBufferType = GL_DEPTH_ATTACHMENT;
		break;
	case EDepthBufferType::DEPTH24_STENCIL8:
		depthBufferFormat = GL_DEPTH24_STENCIL8;
		depthBufferType = GL_DEPTH_STENCIL_ATTACHMENT;
		break;
	default:
		depthBufferFormat = 0;
		depthBufferType = 0;
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////
// jRHI_OpenGL
jRHI_OpenGL::jRHI_OpenGL()
{
}


jRHI_OpenGL::~jRHI_OpenGL()
{
}

jSamplerState* jRHI_OpenGL::CreateSamplerState(const jSamplerStateInfo& info) const
{
	auto samplerState = new jSamplerState_OpenGL(info);
	glGenSamplers(1, &samplerState->SamplerId);
	glSamplerParameteri(samplerState->SamplerId, GL_TEXTURE_MIN_FILTER, GetOpenGLTextureFilterType(info.Minification));
	glSamplerParameteri(samplerState->SamplerId, GL_TEXTURE_MAG_FILTER, GetOpenGLTextureFilterType(info.Magnification));
	glSamplerParameteri(samplerState->SamplerId, GL_TEXTURE_WRAP_S, GetOpenGLTextureAddressMode(info.AddressU));
	glSamplerParameteri(samplerState->SamplerId, GL_TEXTURE_WRAP_T, GetOpenGLTextureAddressMode(info.AddressV));
	glSamplerParameteri(samplerState->SamplerId, GL_TEXTURE_WRAP_R, GetOpenGLTextureAddressMode(info.AddressW));
	glSamplerParameterf(samplerState->SamplerId, GL_TEXTURE_LOD_BIAS, info.MipLODBias);
	glSamplerParameterf(samplerState->SamplerId, GL_TEXTURE_MAX_ANISOTROPY, info.MaxAnisotropy);
	glSamplerParameterfv(samplerState->SamplerId, GL_TEXTURE_BORDER_COLOR, info.BorderColor.v);
	glSamplerParameterf(samplerState->SamplerId, GL_TEXTURE_MIN_LOD, info.MinLOD);
	glSamplerParameterf(samplerState->SamplerId, GL_TEXTURE_MAX_LOD, info.MaxLOD);
	glSamplerParameteri(samplerState->SamplerId, GL_TEXTURE_COMPARE_MODE, GetOpenGLTextureComparisonMode(info.TextureComparisonMode));
	glSamplerParameteri(samplerState->SamplerId, GL_TEXTURE_COMPARE_FUNC, GetOpenGLComparisonFunc(info.ComparisonFunc));

	return samplerState;
}

void jRHI_OpenGL::ReleaseSamplerState(jSamplerState* samplerState) const
{
	auto samplerState_gl = static_cast<jSamplerState_OpenGL*>(samplerState);
	glDeleteSamplers(1, &samplerState_gl->SamplerId);
}

void jRHI_OpenGL::BindSamplerState(int32 index, const jSamplerState* samplerState) const
{
	if (samplerState)
	{
		auto samplerState_gl = static_cast<const jSamplerState_OpenGL*>(samplerState);
		glBindSampler(index, samplerState_gl->SamplerId);
	}
	else
	{
		glBindSampler(index, 0);
	}
}

jVertexBuffer* jRHI_OpenGL::CreateVertexBuffer(const std::shared_ptr<jVertexStreamData>& streamData) const
{
	if (!streamData)
		return nullptr;

	jVertexBuffer_OpenGL* vertexBuffer = new jVertexBuffer_OpenGL();
	vertexBuffer->VertexStreamData = streamData;
	glGenVertexArrays(1, &vertexBuffer->VAO);
	glBindVertexArray(vertexBuffer->VAO);
	std::list<uint32> buffers;
	for (const auto& iter : streamData->Params)
	{
		if (iter->Stride <= 0)
			continue;

		unsigned int bufferType = GL_STATIC_DRAW;
		switch (iter->BufferType)
		{
		case EBufferType::STATIC:
			bufferType = GL_STATIC_DRAW;
			break;
		case EBufferType::DYNAMIC:
			bufferType = GL_DYNAMIC_DRAW;
			break;
		}

		jVertexStream_OpenGL stream;
		glGenBuffers(1, &stream.BufferID);
		glBindBuffer(GL_ARRAY_BUFFER, stream.BufferID);
		glBufferData(GL_ARRAY_BUFFER, iter->GetBufferSize(), iter->GetBufferData(), bufferType);
		stream.Name = iter->Name;
		stream.Count = iter->Stride / iter->ElementTypeSize;
		stream.BufferType = EBufferType::STATIC;
		stream.ElementType = iter->ElementType;
		stream.Stride = iter->Stride;
		stream.Offset = 0;
		stream.InstanceDivisor = iter->InstanceDivisor;

		vertexBuffer->Streams.emplace_back(stream);
	}

	return vertexBuffer;
}

jIndexBuffer* jRHI_OpenGL::CreateIndexBuffer(const std::shared_ptr<jIndexStreamData>& streamData) const
{
	if (!streamData)
		return nullptr;

	unsigned int bufferType = GL_STATIC_DRAW;
	switch (streamData->Param->BufferType)
	{
	case EBufferType::STATIC:
		bufferType = GL_STATIC_DRAW;
		break;
	case EBufferType::DYNAMIC:
		bufferType = GL_DYNAMIC_DRAW;
		break;
	}

	jIndexBuffer_OpenGL* indexBuffer = new jIndexBuffer_OpenGL();
	indexBuffer->IndexStreamData = streamData;
	std::list<uint32> buffers;
	glGenBuffers(1, &indexBuffer->BufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->BufferID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, streamData->Param->GetBufferSize(), streamData->Param->GetBufferData(), bufferType);

	return indexBuffer;
}

void jRHI_OpenGL::BindVertexBuffer(const jVertexBuffer* vb, const jShader* shader) const
{
	auto vb_gl = static_cast<const jVertexBuffer_OpenGL*>(vb);
	glBindVertexArray(vb_gl->VAO);
	auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
	for (const auto& iter : vb_gl->Streams)
	{
		auto loc = glGetAttribLocation(shader_gl->program, iter.Name.c_str());
		if (loc != -1)
		{
			unsigned int elementType = 0;
			switch (iter.ElementType)
			{
			case EBufferElementType::BYTE:
				elementType = GL_UNSIGNED_BYTE;
				break;
			case EBufferElementType::INT:
				elementType = GL_UNSIGNED_INT;
				break;
			case EBufferElementType::FLOAT:
				elementType = GL_FLOAT;
				break;
			}

			glBindBuffer(GL_ARRAY_BUFFER, iter.BufferID);

			if (iter.Count <= 4)
			{
				glVertexAttribPointer(loc, iter.Count, elementType, iter.Normalized, iter.Stride, reinterpret_cast<const void*>(iter.Offset));
				glEnableVertexAttribArray(loc);
				glVertexAttribDivisor(loc, iter.InstanceDivisor);

			}
			else
			{
				const int cnt = iter.Count / 4;
				int32 remainSize = iter.Count;
				const int32 step = sizeof(float) * 4;
				for (int32 i = 0; i < cnt; ++i, remainSize -= 4)
				{
					glVertexAttribPointer(loc + i, Min(4, remainSize), elementType, iter.Normalized, iter.Stride, reinterpret_cast<const void*>(iter.Offset + step * i));
					glEnableVertexAttribArray(loc + i);
					glVertexAttribDivisor(loc + i, iter.InstanceDivisor);
				}
			}
		}
	}
}

void jRHI_OpenGL::BindIndexBuffer(const jIndexBuffer* ib, const jShader* shader) const
{
	auto ib_gl = static_cast<const jIndexBuffer_OpenGL*>(ib);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ib_gl->BufferID);
}

void jRHI_OpenGL::MapBufferdata(IBuffer* buffer) const
{
	throw std::logic_error("The method or operation is not implemented.");
}

void jRHI_OpenGL::DrawArrays(EPrimitiveType type, int vertStartIndex, int vertCount) const
{
	glDrawArrays(GetPrimitiveType(type), vertStartIndex, vertCount);
}

void jRHI_OpenGL::DrawArraysInstanced(EPrimitiveType type, int32 vertStartIndex, int32 vertCount, int32 instanceCount) const
{
	glDrawArraysInstanced(GetPrimitiveType(type), vertStartIndex, vertCount, instanceCount);
}

void jRHI_OpenGL::DrawElements(EPrimitiveType type, int32 elementSize, int32 startIndex, int32 count) const
{
	union jOffset
	{
		int64 offset;
		void* offset_pointer;
	} u_offset;
	u_offset.offset = startIndex * elementSize;
	const auto elementType = (elementSize == 4) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;	
	glDrawElements(GetPrimitiveType(type), count, elementType, u_offset.offset_pointer);
}

void jRHI_OpenGL::DrawElementsInstanced(EPrimitiveType type, int32 elementSize, int32 startIndex, int32 count, int32 instanceCount) const
{
	union jOffset
	{
		int64 offset;
		void* offset_pointer;
	} u_offset;
	u_offset.offset = startIndex * elementSize;
	const auto elementType = (elementSize == 4) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
	glDrawElementsInstanced(GetPrimitiveType(type), count, elementType, u_offset.offset_pointer, instanceCount);
}

void jRHI_OpenGL::DrawElementsBaseVertex(EPrimitiveType type, int elementSize, int32 startIndex, int32 count, int32 baseVertexIndex) const
{
	union jOffset
	{
		int64 offset;
		void* offset_pointer;
	} u_offset;
	u_offset.offset = startIndex * elementSize;
	const auto elementType = (elementSize == 4) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
	glDrawElementsBaseVertex(GetPrimitiveType(type), count, elementType, u_offset.offset_pointer, baseVertexIndex);
}

void jRHI_OpenGL::DrawElementsInstancedBaseVertex(EPrimitiveType type, int elementSize, int32 startIndex, int32 count, int32 baseVertexIndex, int32 instanceCount) const
{
	union jOffset
	{
		int64 offset;
		void* offset_pointer;
	} u_offset;
	u_offset.offset = startIndex * elementSize;
	const auto elementType = (elementSize == 4) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
	glDrawElementsInstancedBaseVertex(GetPrimitiveType(type), count, elementType, u_offset.offset_pointer, instanceCount, baseVertexIndex);
}

void jRHI_OpenGL::DispatchCompute(uint32 numGroupsX, uint32 numGroupsY, uint32 numGroupsZ) const
{
	glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
}

void jRHI_OpenGL::EnableDepthBias(bool enable) const
{
	if (enable)
		glEnable(GL_POLYGON_OFFSET_FILL);
	else
		glDisable(GL_POLYGON_OFFSET_FILL);
}

void jRHI_OpenGL::SetDepthBias(float constant, float slope) const
{
	glPolygonOffset(constant, slope);
}

void jRHI_OpenGL::EnableSRGB(bool enable) const
{
	if (enable)
		glEnable(GL_FRAMEBUFFER_SRGB);
	else
		glDisable(GL_FRAMEBUFFER_SRGB);
}

IShaderStorageBufferObject* jRHI_OpenGL::CreateShaderStorageBufferObject(const char* blockname) const
{
	auto ssbo = new jShaderStorageBufferObject_OpenGL(blockname);
	ssbo->Init();
	return ssbo;
}

IAtomicCounterBuffer* jRHI_OpenGL::CreateAtomicCounterBuffer(const char* name, int32 bindingPoint) const
{
	auto acbo = new jAtomicCounterBuffer_OpenGL(name, bindingPoint);
	acbo->Init();
	return acbo;
}

void jRHI_OpenGL::SetViewport(int32 x, int32 y, int32 width, int32 height) const
{
	glViewport(x, y, width, height);
}

void jRHI_OpenGL::SetViewport(const jViewport& viewport) const
{
	glViewport(static_cast<int32>(viewport.X), static_cast<int32>(viewport.Y), static_cast<int32>(viewport.Width), static_cast<int32>(viewport.Height));
}

void jRHI_OpenGL::SetViewportIndexed(int32 index, float x, float y, float width, float height) const
{
	glViewportIndexedf(index, x, y, width, height);
}

void jRHI_OpenGL::SetViewportIndexed(int32 index, const jViewport& viewport) const
{
	glViewportIndexedfv(index, reinterpret_cast<const float*>(&viewport));
}

void jRHI_OpenGL::SetViewportIndexedArray(int32 startIndex, int32 count, const jViewport* viewports) const
{
	glViewportArrayv(startIndex, count, reinterpret_cast<const float*>(viewports));
}

void jRHI_OpenGL::EnableDepthClip(bool enable) const
{
	// Enabling GL_DEPTH_CLAMP means that it disabling depth clip.
	if (enable)
		glDisable(GL_DEPTH_CLAMP);
	else
		glEnable(GL_DEPTH_CLAMP);
}

void jRHI_OpenGL::BeginDebugEvent(const char* name) const
{
	glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, static_cast<int32>(strlen(name)), name);
}

void jRHI_OpenGL::EndDebugEvent() const
{
	glPopDebugGroup();
}

void jRHI_OpenGL::GenerateMips(const jTexture* texture) const
{
	const auto texture_gl = static_cast<const jTexture_OpenGL*>(texture);
	glActiveTexture(GL_TEXTURE0);
	const auto textureType = GetOpenGLTextureType(texture_gl->TextureType);
	glBindTexture(textureType, texture_gl->TextureID);
	glGenerateMipmap(textureType);
	glBindTexture(textureType, 0);
}

jQueryTime* jRHI_OpenGL::CreateQueryTime() const
{
	auto queryTime = new jQueryTime_OpenGL();
	glGenQueries(1, &queryTime->QueryId);
	return queryTime;
}

void jRHI_OpenGL::ReleaseQueryTime(jQueryTime* queryTime) const 
{
	auto queryTime_gl = static_cast<jQueryTime_OpenGL*>(queryTime);
	glDeleteQueries(1, &queryTime_gl->QueryId);
	queryTime_gl->QueryId = 0;
}

void jRHI_OpenGL::QueryTimeStamp(const jQueryTime* queryTimeStamp) const
{
	const auto queryTimeStamp_gl = static_cast<const jQueryTime_OpenGL*>(queryTimeStamp);
	JASSERT(queryTimeStamp_gl->QueryId > 0);
	glQueryCounter(queryTimeStamp_gl->QueryId, GL_TIMESTAMP);
}

bool jRHI_OpenGL::IsQueryTimeStampResult(const jQueryTime* queryTimeStamp, bool isWaitUntilAvailable) const
{
	auto queryTimeStamp_gl = static_cast<const jQueryTime_OpenGL*>(queryTimeStamp);
	JASSERT(queryTimeStamp_gl->QueryId > 0);
	uint32 available = 0;
	if (isWaitUntilAvailable)
	{
		while (!available)
			glGetQueryObjectuiv(queryTimeStamp_gl->QueryId, GL_QUERY_RESULT_AVAILABLE, &available);
	}
	else
	{
		glGetQueryObjectuiv(queryTimeStamp_gl->QueryId, GL_QUERY_RESULT_AVAILABLE, &available);
	}
	return !!available;
}

void jRHI_OpenGL::GetQueryTimeStampResult(jQueryTime* queryTimeStamp) const
{
	auto queryTimeStamp_gl = static_cast<jQueryTime_OpenGL*>(queryTimeStamp);
	JASSERT(queryTimeStamp_gl->QueryId > 0);
	glGetQueryObjectui64v(queryTimeStamp_gl->QueryId, GL_QUERY_RESULT, &queryTimeStamp_gl->TimeStamp);
}

void jRHI_OpenGL::BeginQueryTimeElapsed(const jQueryTime* queryTimeElpased) const
{
	const auto queryTimeElpased_gl = static_cast<const jQueryTime_OpenGL*>(queryTimeElpased);
	JASSERT(queryTimeElpased_gl->QueryId > 0);
	glBeginQuery(GL_TIME_ELAPSED, queryTimeElpased_gl->QueryId);
}

void jRHI_OpenGL::EndQueryTimeElapsed(const jQueryTime* queryTimeElpased) const 
{
	glEndQuery(GL_TIME_ELAPSED);
}

void jRHI_OpenGL::SetImageTexture(int32 index, const jTexture* texture, EImageTextureAccessType type) const
{
	const auto texture_gl = static_cast<const jTexture_OpenGL*>(texture);
	const auto textureType = GetOpenGLTextureFormat(texture_gl->ColorBufferType);
	const auto accesstype_gl = GetOpenGLImageTextureAccessType(type);
	glBindImageTexture(index, texture_gl->TextureID, 0, GL_FALSE, 0, accesstype_gl, textureType);
}

uint32 jRHI_OpenGL::GetUniformLocation(uint32 InProgram, const char* name) const
{
	return glGetUniformLocation(InProgram, name);
}

void jRHI_OpenGL::SetClear(ERenderBufferType typeBit) const
{
	uint32 clearBufferBit = 0;
	if (!!(ERenderBufferType::COLOR & typeBit))
		clearBufferBit |= GL_COLOR_BUFFER_BIT;
	if (!!(ERenderBufferType::DEPTH & typeBit))
		clearBufferBit |= GL_DEPTH_BUFFER_BIT;
	if (!!(ERenderBufferType::STENCIL & typeBit))
		clearBufferBit |= GL_STENCIL_BUFFER_BIT;

	glClear(clearBufferBit);
}

void jRHI_OpenGL::SetClearColor(float r, float g, float b, float a) const
{
	glClearColor(r, g, b, a);
}

void jRHI_OpenGL::SetClearColor(Vector4 rgba) const
{
	glClearColor(rgba.x, rgba.y, rgba.z, rgba.w);
}

void jRHI_OpenGL::SetShader(const jShader* shader) const
{
	JASSERT(shader);
	auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
	glUseProgram(shader_gl->program);
}

jShader* jRHI_OpenGL::CreateShader(const jShaderInfo& shaderInfo) const
{
	auto shader = new jShader_OpenGL();
	CreateShader(shader, shaderInfo);
	return shader;
}

bool jRHI_OpenGL::CreateShader(jShader* OutShader, const jShaderInfo& shaderInfo) const
{
	JASSERT(OutShader);

	// https://stackoverflow.com/questions/5878775/how-to-find-and-replace-string
	auto ReplaceString = [](std::string subject, const std::string& search, const std::string& replace) {
		size_t pos = 0;
		while ((pos = subject.find(search, pos)) != std::string::npos) {
			subject.replace(pos, search.length(), replace);
			pos += replace.length();
		}
		return subject;
	};

	static bool initializedCommonShader = false;
	static std::string commonShader;
	static std::string shadowShader;
	if (!initializedCommonShader)
	{
		initializedCommonShader = true;

		jFile commonFile;
		commonFile.OpenFile("Shaders/common.glsl", FileType::TEXT, ReadWriteType::READ);
		commonFile.ReadFileToBuffer(false);
		commonShader = commonFile.GetBuffer();

		jFile shadowFile;
		shadowFile.OpenFile("Shaders/shadow.glsl", FileType::TEXT, ReadWriteType::READ);
		shadowFile.ReadFileToBuffer(false);
		shadowShader = shadowFile.GetBuffer();
	}

	uint32 program = 0;
	if (shaderInfo.cs.length())
	{
		jFile csFile;
		csFile.OpenFile(shaderInfo.cs.c_str(), FileType::TEXT, ReadWriteType::READ);
		csFile.ReadFileToBuffer(false);
		std::string csText(csFile.GetBuffer());

		auto compute_shader = glCreateShader(GL_COMPUTE_SHADER);
		const char* vsPtr = csText.c_str();
		glShaderSource(compute_shader, 1, &vsPtr, nullptr);
		glCompileShader(compute_shader);
		int isValid = 0;
		glGetShaderiv(compute_shader, GL_COMPILE_STATUS, &isValid);
		if (!isValid)
		{
			int maxLength = 0;
			glGetShaderiv(compute_shader, GL_INFO_LOG_LENGTH, &maxLength);

			if (maxLength > 0)
			{
				std::vector<char> errorLog(maxLength + 1, 0);
				glGetShaderInfoLog(compute_shader, maxLength, &maxLength, &errorLog[0]);
				JMESSAGE(&errorLog[0]);
				glDeleteShader(compute_shader);
			}
			return false;
		}

		program = glCreateProgram();
		glAttachShader(program, compute_shader);
		glLinkProgram(program);
		isValid = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &isValid);
		if (!isValid)
		{
			int maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			if (maxLength > 0)
			{
				std::vector<char> errorLog(maxLength + 1, 0);
				glGetProgramInfoLog(program, maxLength, &maxLength, &errorLog[0]);
				JMESSAGE(&errorLog[0]);
				glDeleteShader(compute_shader);
				glDeleteProgram(program);
			}
			return false;
		}

		glDeleteShader(compute_shader);
	}
	else
	{
		jFile vsFile;
		vsFile.OpenFile(shaderInfo.vs.c_str(), FileType::TEXT, ReadWriteType::READ);
		vsFile.ReadFileToBuffer(false);
		std::string vsText(vsFile.IsBufferEmpty() ? "" : vsFile.GetBuffer());
		vsText = ReplaceString(vsText, "#include \"shadow.glsl\"", shadowShader);
		vsText = ReplaceString(vsText, "#include \"common.glsl\"", commonShader);
		vsText = ReplaceString(vsText, "#preprocessor", shaderInfo.vsPreProcessor);
		vsFile.CloseFile();

		jFile gsFile;
		gsFile.OpenFile(shaderInfo.gs.c_str(), FileType::TEXT, ReadWriteType::READ);
		gsFile.ReadFileToBuffer(false);
		std::string gsText(gsFile.IsBufferEmpty() ? "" : gsFile.GetBuffer());
		gsText = ReplaceString(gsText, "#include \"shadow.glsl\"", shadowShader);
		gsText = ReplaceString(gsText, "#include \"common.glsl\"", commonShader);
		gsText = ReplaceString(gsText, "#preprocessor", shaderInfo.gsPreProcessor);
		gsFile.CloseFile();

		jFile fsFile;
		fsFile.OpenFile(shaderInfo.fs.c_str(), FileType::TEXT, ReadWriteType::READ);
		fsFile.ReadFileToBuffer(false);
		std::string fsText(fsFile.IsBufferEmpty() ? "" : fsFile.GetBuffer());
		fsText = ReplaceString(fsText, "#include \"shadow.glsl\"", shadowShader);
		fsText = ReplaceString(fsText, "#include \"common.glsl\"", commonShader);
		fsText = ReplaceString(fsText, "#preprocessor", shaderInfo.fsPreProcessor);
		fsFile.CloseFile();

		auto vs = glCreateShader(GL_VERTEX_SHADER);
		const char* vsPtr = vsText.c_str();
		glShaderSource(vs, 1, &vsPtr, nullptr);
		glCompileShader(vs);

		auto checkShaderValid = [](uint32 shaderId, const std::string& filename)
		{
			int isValid = 0;
			glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isValid);
			if (!isValid)
			{
				int maxLength = 0;
				glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength);

				if (maxLength > 0)
				{
					std::string errorLog(maxLength, 0);
					glGetShaderInfoLog(shaderId, maxLength, &maxLength, &errorLog[0]);
					if (maxLength > 0)
						errorLog.resize(maxLength - 1);	// remove null character to concatenate
					JMESSAGE(std::string(errorLog + "(" + filename + ")").c_str());
				}
				glDeleteShader(shaderId);
				return false;
			}
			return true;
		};
		if (!checkShaderValid(vs, shaderInfo.vs))
			return false;

		uint32 gs = 0;
		if (!gsText.empty())
		{
			gs = glCreateShader(GL_GEOMETRY_SHADER);
			const char* gsPtr = gsText.c_str();
			glShaderSource(gs, 1, &gsPtr, nullptr);
			glCompileShader(gs);

			if (!checkShaderValid(gs, shaderInfo.gs))
			{
				glDeleteShader(vs);
				return false;
			}
		}

		auto fs = glCreateShader(GL_FRAGMENT_SHADER);
		const char* fsPtr = fsText.c_str();
		int32 fragment_shader_string_length = static_cast<int32>(fsText.length());
		glShaderSource(fs, 1, &fsPtr, &fragment_shader_string_length);
		glCompileShader(fs);

		if (!checkShaderValid(fs, shaderInfo.fs))
		{
			glDeleteShader(vs);
			if (gs)
				glDeleteShader(gs);
			return false;
		}

		program = glCreateProgram();
		glAttachShader(program, vs);
		if (gs)
			glAttachShader(program, gs);
		glAttachShader(program, fs);
		glLinkProgram(program);

		int isValid = 0;
		glGetProgramiv(program, GL_LINK_STATUS, &isValid);
		if (!isValid)
		{
			int maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			if (maxLength > 0)
			{
				std::vector<char> errorLog(maxLength + 1, 0);
				glGetProgramInfoLog(program, maxLength, &maxLength, &errorLog[0]);
				JMESSAGE(&errorLog[0]);
			}
			glDeleteShader(vs);
			glDeleteShader(fs);
			if (gs)
				glDeleteShader(gs);
			glDeleteShader(program);
			return false;
		}

		glValidateProgram(program);
		isValid = 0;
		glGetProgramiv(program, GL_VALIDATE_STATUS, &isValid);
		if (!isValid)
		{
			int maxLength = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

			if (maxLength > 0)
			{
				std::vector<char> errorLog(maxLength + 1, 0);
				glGetProgramInfoLog(program, maxLength, &maxLength, &errorLog[0]);
				JMESSAGE(&errorLog[0]);
			}
			glDeleteShader(vs);
			glDeleteShader(fs);
			if (gs)
				glDeleteShader(gs);
			glDeleteShader(program);
			return false;
		}
		glDeleteShader(vs);
		if (gs)
			glDeleteShader(gs);
		glDeleteShader(fs);
	}

	auto shader_gl = static_cast<jShader_OpenGL*>(OutShader);
	if (shader_gl->program)
		glDeleteProgram(shader_gl->program);
	shader_gl->program = program;
	shader_gl->ShaderInfo = shaderInfo;

	return true;
}

void jRHI_OpenGL::ReleaseShader(jShader* shader) const
{
	JASSERT(shader);
	auto shader_gl = static_cast<jShader_OpenGL*>(shader);
	if (shader_gl->program)
		glDeleteProgram(shader_gl->program);
	shader_gl->program = 0;
}

jTexture* jRHI_OpenGL::CreateNullTexture() const
{
	auto texture = new jTexture_OpenGL();
	texture->sRGB = false;
	texture->ColorBufferType = ETextureFormat::RGB;
	texture->ColorFormatType = EFormatType::UNSIGNED_BYTE;
	glGenTextures(1, &texture->TextureID);
	glBindTexture(GL_TEXTURE_2D, texture->TextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 2, 2, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	return texture;
}

jTexture* jRHI_OpenGL::CreateTextureFromData(void* data, int32 width, int32 height, bool sRGB
	, EFormatType dataType /*= EFormatType::UNSIGNED_BYTE*/, ETextureFormat textureFormat /*= ETextureFormat::RGBA*/, bool createMipmap /*= false*/) const
{
	const uint32 internalFormat = GetOpenGLTextureFormat(textureFormat);
	const uint32 simpleFormat = GetOpenGLTextureFormatSimple(textureFormat);
	const uint32 formatType = GetOpenGLFormatType(dataType);

	auto texture = new jTexture_OpenGL();
	texture->sRGB = sRGB;
	texture->ColorBufferType = textureFormat;
	texture->ColorFormatType = dataType;
	glGenTextures(1, &texture->TextureID);
	glBindTexture(GL_TEXTURE_2D, texture->TextureID);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, simpleFormat, formatType, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	if (createMipmap)
		glGenerateMipmap(GL_TEXTURE_2D);
	return texture;
}

bool jRHI_OpenGL::SetUniformbuffer(const char* name, const Matrix& InData, const jShader* InShader) const
{
	JASSERT(name);

	auto shader_gl = static_cast<const jShader_OpenGL*>(InShader);
	auto loc = shader_gl->TryGetUniformLocation(name);
	if (loc == -1)
		return false;
	glUniformMatrix4fv(loc, 1, true, &InData.mm[0]);
	
	return true;
}

bool jRHI_OpenGL::SetUniformbuffer(const char* name, const int InData, const jShader* InShader) const
{
	JASSERT(name);

	auto shader_gl = static_cast<const jShader_OpenGL*>(InShader);
	auto loc = shader_gl->TryGetUniformLocation(name);
	if (loc == -1)
		return false;
	glUniform1i(loc, InData);
	return true;
}

bool jRHI_OpenGL::SetUniformbuffer(const char* name, const uint32 InData, const jShader* InShader) const
{
	JASSERT(name);

	auto shader_gl = static_cast<const jShader_OpenGL*>(InShader);
	auto loc = shader_gl->TryGetUniformLocation(name);
	if (loc == -1)
		return false;
	glUniform1ui(loc, InData);
	return true;
}

bool jRHI_OpenGL::SetUniformbuffer(const char* name, const float InData, const jShader* InShader) const
{
	JASSERT(name);

	auto shader_gl = static_cast<const jShader_OpenGL*>(InShader);
	auto loc = shader_gl->TryGetUniformLocation(name);
	if (loc == -1)
		return false;
	glUniform1f(loc, InData);
	return true;
}

bool jRHI_OpenGL::SetUniformbuffer(const char* name, const Vector2& InData, const jShader* InShader) const
{
	auto shader_gl = static_cast<const jShader_OpenGL*>(InShader);
	auto loc = shader_gl->TryGetUniformLocation(name);
	if (loc == -1)
		return false;
	glUniform2fv(loc, 1, &InData.v[0]);
	return true;
}

bool jRHI_OpenGL::SetUniformbuffer(const char* name, const Vector& InData, const jShader* InShader) const
{
	auto shader_gl = static_cast<const jShader_OpenGL*>(InShader);
	auto loc = shader_gl->TryGetUniformLocation(name);
	if (loc == -1)
		return false;
	glUniform3fv(loc, 1, &InData.v[0]);
	return true;
}

bool jRHI_OpenGL::SetUniformbuffer(const char* name, const Vector4& InData, const jShader* InShader) const
{
	auto shader_gl = static_cast<const jShader_OpenGL*>(InShader);
	auto loc = shader_gl->TryGetUniformLocation(name);
	if (loc == -1)
		return false;
	glUniform4fv(loc, 1, &InData.v[0]);
	return true;
}

//bool jRHI_OpenGL::SetUniformbuffer(const IUniformBuffer* buffer, const jShader* shader) const
//{
//	switch (buffer->GetType())
//	{
//	case EUniformType::MATRIX:
//	{
//		auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
//
//		auto uniformMatrix = static_cast<const jUniformBuffer<Matrix>*>(buffer);
//		auto loc = glGetUniformLocation(shader_gl->program, uniformMatrix->Name.c_str());
//		if (loc == -1)
//			return false;
//		glUniformMatrix4fv(loc, 1, true, &uniformMatrix->Data.mm[0]);
//		break;
//	}
//	case EUniformType::BOOL:
//	{
//		auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
//
//		auto uniformVector = static_cast<const jUniformBuffer<bool>*>(buffer);
//		auto loc = glGetUniformLocation(shader_gl->program, uniformVector->Name.c_str());
//		if (loc != -1)
//			glUniform1i(loc, (int32)uniformVector->Data);
//		break;
//	}
//	case EUniformType::INT:
//	{
//		auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
//
//		auto uniformVector = static_cast<const jUniformBuffer<int>*>(buffer);
//		auto loc = glGetUniformLocation(shader_gl->program, uniformVector->Name.c_str());
//		if (loc == -1)
//			return false;
//		glUniform1i(loc, uniformVector->Data);
//		break;
//	}
//	case EUniformType::FLOAT:
//	{
//		auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
//
//		auto uniformVector = static_cast<const jUniformBuffer<float>*>(buffer);
//		auto loc = glGetUniformLocation(shader_gl->program, uniformVector->Name.c_str());
//		if (loc == -1)
//			return false;
//		glUniform1f(loc, uniformVector->Data);
//		break;
//	}
//	case EUniformType::VECTOR2:
//	{
//		auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
//
//		auto uniformVector = static_cast<const jUniformBuffer<Vector2>*>(buffer);
//		auto loc = glGetUniformLocation(shader_gl->program, uniformVector->Name.c_str());
//		if (loc == -1)
//			return false;
//		glUniform2fv(loc, 1, &uniformVector->Data.v[0]);
//		break;
//	}
//	case EUniformType::VECTOR3:
//	{
//		auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
//
//		auto uniformVector = static_cast<const jUniformBuffer<Vector>*>(buffer);
//		auto loc = glGetUniformLocation(shader_gl->program, uniformVector->Name.c_str());
//		if (loc == -1)
//			return false;
//		glUniform3fv(loc, 1, &uniformVector->Data.v[0]);
//		break;
//	}
//	case EUniformType::VECTOR4:
//	{
//		auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
//
//		auto uniformVector = static_cast<const jUniformBuffer<Vector4>*>(buffer);
//		auto loc = glGetUniformLocation(shader_gl->program, uniformVector->Name.c_str());
//		if (loc == -1)
//			return false;
//		glUniform4fv(loc, 1, &uniformVector->Data.v[0]);
//		break;
//	}
//	default:
//		JASSERT(0);
//		return false;
//	}
//
//	return true;
//}

int32 jRHI_OpenGL::SetMatetrial(const jMaterialData* materialData, const jShader* shader, int32 baseBindingIndex /*= 0*/) const
{
	JASSERT(materialData);

	int32 index = baseBindingIndex;
	const int32 NumOfParams = (int32)materialData->Params.size();
	for (int32 i = 0; i < NumOfParams; ++i)
	{
		auto matParam = materialData->Params[i];
		if (!matParam)
			continue;

		auto tex_gl = static_cast<const jTexture_OpenGL*>(matParam->Texture);
		if (!tex_gl)
			continue;

		if (!g_rhi->SetUniformbuffer(matParam->Name.c_str(), index, shader))
			continue;
		SetTexture(index, matParam->Texture);

		if (matParam->SamplerState)
		{
			BindSamplerState(index, matParam->SamplerState);
		}
		else
		{
			BindSamplerState(index, nullptr);
			SetTextureFilter(tex_gl->TextureType, ETextureFilterTarget::MAGNIFICATION, tex_gl->Magnification);
			SetTextureFilter(tex_gl->TextureType, ETextureFilterTarget::MINIFICATION, tex_gl->Minification);
		}

		static char szTemp[128] = { 0, };
		sprintf_s(szTemp, sizeof(szTemp), "TextureSRGB[%d]", index);
		g_rhi->SetUniformbuffer(szTemp , tex_gl->sRGB, shader);

		++index;
	}

	return index;
}

void jRHI_OpenGL::SetTexture(int32 index, const jTexture* texture) const
{
	glActiveTexture(GL_TEXTURE0 + index);
	
	auto texture_gl = static_cast<const jTexture_OpenGL*>(texture);
	auto textureType = GetOpenGLTextureType(texture_gl->TextureType);
	glBindTexture(textureType, texture_gl->TextureID);
}

void jRHI_OpenGL::SetTextureFilter(ETextureType type, ETextureFilterTarget target, ETextureFilter filter) const
{
	const uint32 texFilter = GetOpenGLTextureFilterType(filter);

	uint32 texTarget = 0;
	switch (target)
	{
	case ETextureFilterTarget::MINIFICATION:
		texTarget = GL_TEXTURE_MIN_FILTER;
		break;
	case ETextureFilterTarget::MAGNIFICATION:
		texTarget = GL_TEXTURE_MAG_FILTER;
		break;
	default:
		break;
	}

	uint32 textureType = 0;
	switch (type)
	{
	case ETextureType::TEXTURE_2D:
		textureType = GL_TEXTURE_2D;
		break;
	case ETextureType::TEXTURE_2D_ARRAY:
	case ETextureType::TEXTURE_2D_ARRAY_OMNISHADOW:
		textureType = GL_TEXTURE_2D_ARRAY;
		break;
	case ETextureType::TEXTURE_CUBE:
		textureType = GL_TEXTURE_CUBE_MAP;
		break;
	default:
		break;
	}

	glTexParameteri(textureType, texTarget, texFilter);
}

void jRHI_OpenGL::EnableCullFace(bool enable) const
{
	if (enable)
		glEnable(GL_CULL_FACE);
	else
		glDisable(GL_CULL_FACE);
}

jRenderTarget* jRHI_OpenGL::CreateRenderTarget(const jRenderTargetInfo& info) const
{
	const uint32 internalFormat = GetOpenGLTextureFormat(info.InternalFormat);
	const uint32 format = GetOpenGLTextureFormat(info.Format);
	const uint32 formatType = GetOpenGLFormatType(info.FormatType);

	uint32 depthBufferFormat = 0;
	uint32 depthBufferType = 0;
	const bool hasDepthAttachment = GetDepthBufferFormatAndType(depthBufferFormat, depthBufferType, info.DepthBufferType);

	const uint32 magnification = GetOpenGLTextureFilterType(info.Magnification);
	const uint32 minification = GetOpenGLTextureFilterType(info.Minification);

	auto rt_gl = new jRenderTarget_OpenGL();
	rt_gl->Info = info;
	
	const int32 MipLevels = (info.IsGenerateMipmapDepth ? jTexture::GetMipLevels(info.Width, info.Height) : 1);

	if (info.TextureType == ETextureType::TEXTURE_2D)
	{
		uint32 fbo = 0;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		rt_gl->Textures.resize(info.TextureCount);
		for (int i = 0; i < info.TextureCount; ++i)
		{
			uint32 tbo = 0;
			glGenTextures(1, &tbo);
			glBindTexture(GL_TEXTURE_2D, tbo);
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, info.Width, info.Height, 0, format, formatType, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minification);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magnification);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			if (info.IsGenerateMipmap)
				glGenerateMipmap(GL_TEXTURE_2D);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, tbo, 0);
			rt_gl->drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);

			auto tex_gl = new jTexture_OpenGL();
			tex_gl->TextureType = info.TextureType;
			tex_gl->ColorBufferType = info.InternalFormat;
			tex_gl->ColorFormatType = info.FormatType;
			tex_gl->Magnification = info.Magnification;
			tex_gl->Minification = info.Minification;
			tex_gl->TextureID = tbo;
			rt_gl->Textures[i] = std::shared_ptr<jTexture>(tex_gl);
		}

		if (hasDepthAttachment)
		{
			uint32 tbo = 0;
			const int32 MipLevels = (info.IsGenerateMipmapDepth ? jTexture::GetMipLevels(info.Width, info.Height) : 1);
			glGenTextures(1, &tbo);
			glBindTexture(GL_TEXTURE_2D, tbo);
			glTexStorage2D(GL_TEXTURE_2D, MipLevels, depthBufferFormat, info.Width, info.Height);
			glFramebufferTexture2D(GL_FRAMEBUFFER, depthBufferType, GL_TEXTURE_2D, tbo, 0);

			auto tex_gl = new jTexture_OpenGL();
			tex_gl->TextureType = info.TextureType;
			tex_gl->DepthBufferType = info.DepthBufferType;
			tex_gl->TextureID = tbo;
			tex_gl->DepthBufferType = info.DepthBufferType;
			rt_gl->TextureDepth = std::shared_ptr<jTexture>(tex_gl);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			auto status_code = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			char szTemp[256] = { 0, };
			sprintf_s(szTemp, sizeof(szTemp), "Failed to create Texture2D framebuffer which is not complete : %d", status_code);
			JMESSAGE(szTemp);
			return nullptr;
		}
	
		rt_gl->fbos.push_back(fbo);
	}
	else if (info.TextureType == ETextureType::TEXTURE_2D_ARRAY)
	{
		auto tex_gl = new jTexture_OpenGL();
		tex_gl->Magnification = info.Magnification;
		tex_gl->Minification = info.Minification;
		tex_gl->ColorBufferType = info.InternalFormat;
		tex_gl->ColorFormatType = info.FormatType;

		uint32 tbo = 0;
		glGenTextures(1, &tbo);
		glBindTexture(GL_TEXTURE_2D_ARRAY, tbo);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, minification);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, magnification);
		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, internalFormat, info.Width, info.Height, info.TextureCount, 0, format, formatType, nullptr);
		if (info.IsGenerateMipmap)
			glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

		uint32 fbo = 0;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		for (int32 i = 0; i < info.TextureCount; ++i)
		{
			glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, tbo, 0, i);
			rt_gl->drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
		}

		if (hasDepthAttachment)
		{
			uint32 tbo = 0;
			glGenTextures(1, &tbo);
			glBindTexture(GL_TEXTURE_2D, tbo);
			glTexStorage2D(GL_TEXTURE_2D, MipLevels, depthBufferFormat, info.Width, info.Height);
			glFramebufferTexture2D(GL_FRAMEBUFFER, depthBufferType, GL_TEXTURE_2D, tbo, 0);

			auto tex_gl = new jTexture_OpenGL();
			tex_gl->TextureType = info.TextureType;
			tex_gl->Magnification = info.Magnification;
			tex_gl->Minification = info.Minification;
			tex_gl->TextureID = tbo;
			tex_gl->DepthBufferType = info.DepthBufferType;
			rt_gl->TextureDepth = std::shared_ptr<jTexture>(tex_gl);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			auto status_code = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			char szTemp[256] = { 0, };
			sprintf_s(szTemp, sizeof(szTemp), "Failed to create Texture2DArray framebuffer which is not complete : %d", status_code);
			JMESSAGE(szTemp);
			return nullptr;
		}

		tex_gl->TextureType = info.TextureType;
		tex_gl->TextureID = tbo;
		rt_gl->Textures.push_back(std::shared_ptr<jTexture>(tex_gl));

		rt_gl->fbos.push_back(fbo);
	}
	else if (info.TextureType == ETextureType::TEXTURE_2D_ARRAY_OMNISHADOW)
	{
		uint32 fbo = 0;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);

		rt_gl->Textures.resize(info.TextureCount);

		JASSERT(info.TextureCount == 1);
		for (int i = 0; i < info.TextureCount; ++i)
		{
			uint32 tbo = 0;
			glGenTextures(1, &tbo);
			glBindTexture(GL_TEXTURE_2D, tbo);
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, info.Width, info.Height, 0, format, formatType, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minification);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magnification);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			if (info.IsGenerateMipmap)
				glGenerateMipmap(GL_TEXTURE_2D);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, tbo, 0);
			rt_gl->drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);

			auto tex_gl = new jTexture_OpenGL();
			tex_gl->TextureType = info.TextureType;
			tex_gl->Magnification = info.Magnification;
			tex_gl->Minification = info.Minification;
			tex_gl->TextureID = tbo;
			tex_gl->ColorBufferType = info.InternalFormat;
			tex_gl->ColorFormatType = info.FormatType;
			rt_gl->Textures[i] = std::shared_ptr<jTexture>(tex_gl);
		}

		if (hasDepthAttachment)
		{
			uint32 tbo = 0;
			glGenTextures(1, &tbo);
			glBindTexture(GL_TEXTURE_2D, tbo);
			glTexStorage2D(GL_TEXTURE_2D, MipLevels, depthBufferFormat, info.Width, info.Height);
			glFramebufferTexture2D(GL_FRAMEBUFFER, depthBufferType, GL_TEXTURE_2D, tbo, 0);

			auto tex_gl = new jTexture_OpenGL();
			tex_gl->TextureType = info.TextureType;
			tex_gl->Magnification = info.Magnification;
			tex_gl->Minification = info.Minification;
			tex_gl->TextureID = tbo;
			tex_gl->DepthBufferType = info.DepthBufferType;
			rt_gl->TextureDepth = std::shared_ptr<jTexture>(tex_gl);
		}

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			auto status_code = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			char szTemp[256] = { 0, };
			sprintf_s(szTemp, sizeof(szTemp), "Failed to create Texture2D framebuffer which is not complete : %d", status_code);
			JMESSAGE(szTemp);
			return nullptr;
		}

		rt_gl->fbos.push_back(fbo);
	}
	else if (info.TextureType == ETextureType::TEXTURE_CUBE)
	{
		uint32 tbo = 0;
		glGenTextures(1, &tbo);
		glBindTexture(GL_TEXTURE_CUBE_MAP, tbo);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minification);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, magnification);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		for (int i = 0; i < 6; ++i)
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, info.Width, info.Height, 0, format, formatType, nullptr);

		if (info.IsGenerateMipmap)
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

		auto tex_gl = new jTexture_OpenGL();
		tex_gl->TextureType = info.TextureType;
		tex_gl->TextureID = tbo;
		tex_gl->Magnification = info.Magnification;
		tex_gl->Minification = info.Minification;
		tex_gl->ColorBufferType = info.InternalFormat;
		tex_gl->ColorFormatType = info.FormatType;
		rt_gl->Textures.push_back(std::shared_ptr<jTexture>(tex_gl));

		for (int i = 0; i < 6; ++i)
		{
			uint32 fbo = 0;
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, tbo, 0);

			if (hasDepthAttachment)
			{
				uint32 rbo = 0;
				glGenRenderbuffers(1, &rbo);
				glBindRenderbuffer(GL_RENDERBUFFER, rbo);
				glRenderbufferStorage(GL_RENDERBUFFER, depthBufferFormat, info.Width, info.Height);
				glFramebufferRenderbuffer(GL_FRAMEBUFFER, depthBufferType, GL_RENDERBUFFER, rbo);
				rt_gl->rbos.push_back(rbo);
			}

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				auto status_code = glCheckFramebufferStatus(GL_FRAMEBUFFER);
				char szTemp[256] = { 0, };
				sprintf_s(szTemp, sizeof(szTemp), "Failed to create TextureCube framebuffer %d which is not complete : %d", i, status_code);
				JMESSAGE(szTemp);
				return nullptr;
			}

			rt_gl->fbos.push_back(fbo);
		}

		rt_gl->drawBuffers.push_back(GL_COLOR_ATTACHMENT0);
	}
	else
	{
		JMESSAGE("Unsupported type texture in FramebufferPool");
		return nullptr;
	}

	return rt_gl;
}

void jRHI_OpenGL::EnableDepthTest(bool enable) const
{
	if (enable)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);
}

void jRHI_OpenGL::SetRenderTarget(const jRenderTarget* rt, int32 index /*= 0*/, bool mrt /*= false*/) const
{
	if (rt)
	{
		rt->Begin(index, mrt);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
	}
}

void jRHI_OpenGL::SetDrawBuffers(const std::initializer_list<EDrawBufferType>& list) const
{
	std::vector<uint32> buffers;
	for(auto& iter : list)
	{
		switch (iter)
		{
		case EDrawBufferType::COLOR_ATTACHMENT0:
			buffers.push_back(GL_COLOR_ATTACHMENT0);
			break;
		case EDrawBufferType::COLOR_ATTACHMENT1:
			buffers.push_back(GL_COLOR_ATTACHMENT1);
			break;
		case EDrawBufferType::COLOR_ATTACHMENT2:
			buffers.push_back(GL_COLOR_ATTACHMENT2);
			break;
		case EDrawBufferType::COLOR_ATTACHMENT3:
			buffers.push_back(GL_COLOR_ATTACHMENT3);
			break;
		case EDrawBufferType::COLOR_ATTACHMENT4:
			buffers.push_back(GL_COLOR_ATTACHMENT4);
			break;
		case EDrawBufferType::COLOR_ATTACHMENT5:
			buffers.push_back(GL_COLOR_ATTACHMENT5);
			break;
		case EDrawBufferType::COLOR_ATTACHMENT6:
			buffers.push_back(GL_COLOR_ATTACHMENT6);
			break;
		default:
			JASSERT(0);
			break;
		}
	}

	if (!buffers.empty())
		glDrawBuffers(static_cast<int32>(buffers.size()), &buffers[0]);
}

void jRHI_OpenGL::UpdateVertexBuffer(jVertexBuffer* vb, const std::shared_ptr<jVertexStreamData>& streamData) const
{
	auto vb_gl = static_cast<jVertexBuffer_OpenGL*>(vb);

	glBindVertexArray(vb_gl->VAO);
	for (auto& iter : vb_gl->Streams)
		glDeleteBuffers(1, &iter.BufferID);
	vb_gl->Streams.clear();

	vb_gl->VertexStreamData = streamData;

	for (const auto& iter : streamData->Params)
	{
		if (iter->Stride <= 0)
			continue;

		unsigned int bufferType = GL_STATIC_DRAW;
		switch (iter->BufferType)
		{
		case EBufferType::STATIC:
			bufferType = GL_STATIC_DRAW;
			break;
		case EBufferType::DYNAMIC:
			bufferType = GL_DYNAMIC_DRAW;
			break;
		}

		jVertexStream_OpenGL stream;
		glGenBuffers(1, &stream.BufferID);
		glBindBuffer(GL_ARRAY_BUFFER, stream.BufferID);
		glBufferData(GL_ARRAY_BUFFER, iter->GetBufferSize(), iter->GetBufferData(), bufferType);
		stream.Name = iter->Name;
		stream.Count = iter->Stride / iter->ElementTypeSize;
		stream.BufferType = iter->BufferType;
		stream.ElementType = iter->ElementType;
		stream.Stride = iter->Stride;
		stream.InstanceDivisor = iter->InstanceDivisor;
		stream.Offset = 0;

		vb_gl->Streams.emplace_back(stream);
	}
}

void jRHI_OpenGL::UpdateVertexBuffer(jVertexBuffer* vb, IStreamParam* streamParam, int32 streamParamIndex) const
{
	JASSERT(streamParam->Stride > 0);
	if (streamParam->Stride <= 0)
		return;

	auto vb_gl = static_cast<jVertexBuffer_OpenGL*>(vb);
	
	auto stream = vb_gl->Streams[streamParamIndex];

	unsigned int bufferType = GL_STATIC_DRAW;
	switch (streamParam->BufferType)
	{
	case EBufferType::STATIC:
		bufferType = GL_STATIC_DRAW;
		break;
	case EBufferType::DYNAMIC:
		bufferType = GL_DYNAMIC_DRAW;
		break;
	}

	glBindVertexArray(vb_gl->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, stream.BufferID);
	glBufferData(GL_ARRAY_BUFFER, streamParam->GetBufferSize(), streamParam->GetBufferData(), bufferType);
	stream.Name = streamParam->Name;
	stream.Count = streamParam->Stride / streamParam->ElementTypeSize;
	stream.ElementType = streamParam->ElementType;
	stream.Stride = streamParam->Stride;
	stream.Offset = 0;
}

void jRHI_OpenGL::EnableBlend(bool enable) const
{
	if (enable)
		glEnable(GL_BLEND);
	else
		glDisable(GL_BLEND);
}

void jRHI_OpenGL::SetBlendFunc(EBlendSrc src, EBlendDest dest) const
{
	unsigned int src_gl = 0;
	switch (src)
	{
	case EBlendSrc::ZERO:
		src_gl = GL_ZERO;
		break;
	case EBlendSrc::ONE:
		src_gl = GL_ONE;
		break;
	case EBlendSrc::SRC_COLOR:
		src_gl = GL_SRC_COLOR;
		break;
	case EBlendSrc::ONE_MINUS_SRC_COLOR:
		src_gl = GL_ONE_MINUS_SRC_COLOR;
		break;
	case EBlendSrc::DST_COLOR:
		src_gl = GL_DST_COLOR;
		break;
	case EBlendSrc::ONE_MINUS_DST_COLOR:
		src_gl = GL_ONE_MINUS_DST_COLOR;
		break;
	case EBlendSrc::SRC_ALPHA:
		src_gl = GL_SRC_ALPHA;
		break;
	case EBlendSrc::ONE_MINUS_SRC_ALPHA:
		src_gl = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case EBlendSrc::DST_ALPHA:
		src_gl = GL_DST_ALPHA;
		break;
	case EBlendSrc::ONE_MINUS_DST_ALPHA:
		src_gl = GL_ONE_MINUS_DST_ALPHA;
		break;
	case EBlendSrc::CONSTANT_COLOR:
		src_gl = GL_CONSTANT_COLOR;
		break;
	case EBlendSrc::ONE_MINUS_CONSTANT_COLOR:
		src_gl = GL_ONE_MINUS_CONSTANT_COLOR;
		break;
	case EBlendSrc::CONSTANT_ALPHA:
		src_gl = GL_CONSTANT_ALPHA;
		break;
	case EBlendSrc::ONE_MINUS_CONSTANT_ALPHA:
		src_gl = GL_ONE_MINUS_CONSTANT_ALPHA;
		break;
	case EBlendSrc::SRC_ALPHA_SATURATE:
		src_gl = GL_SRC_ALPHA_SATURATE;
		break;
	default:
		JASSERT(0);
		break;
	}

	unsigned int dest_gl = 0;
	switch (dest)
	{
	case EBlendDest::ZERO:
		dest_gl = GL_ZERO;
		break;
	case EBlendDest::ONE:
		dest_gl = GL_ONE;
		break;
	case EBlendDest::SRC_COLOR:
		dest_gl = GL_SRC_COLOR;
		break;
	case EBlendDest::ONE_MINUS_SRC_COLOR:
		dest_gl = GL_ONE_MINUS_SRC_COLOR;
		break;
	case EBlendDest::DST_COLOR:
		dest_gl = GL_DST_COLOR;
		break;
	case EBlendDest::ONE_MINUS_DST_COLOR:
		dest_gl = GL_ONE_MINUS_DST_COLOR;
		break;
	case EBlendDest::SRC_ALPHA:
		dest_gl = GL_SRC_ALPHA;
		break;
	case EBlendDest::ONE_MINUS_SRC_ALPHA:
		dest_gl = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case EBlendDest::DST_ALPHA:
		dest_gl = GL_DST_ALPHA;
		break;
	case EBlendDest::ONE_MINUS_DST_ALPHA:
		dest_gl = GL_ONE_MINUS_DST_ALPHA;
		break;
	case EBlendDest::CONSTANT_COLOR:
		dest_gl = GL_CONSTANT_COLOR;
		break;
	case EBlendDest::ONE_MINUS_CONSTANT_COLOR:
		dest_gl = GL_ONE_MINUS_CONSTANT_COLOR;
		break;
	case EBlendDest::CONSTANT_ALPHA:
		dest_gl = GL_CONSTANT_ALPHA;
		break;
	case EBlendDest::ONE_MINUS_CONSTANT_ALPHA:
		dest_gl = GL_ONE_MINUS_CONSTANT_ALPHA;
		break;
	default:
		JASSERT(0);
		break;
	}

	glBlendFunc(src_gl, dest_gl);
}

void jRHI_OpenGL::EnableStencil(bool enable) const
{
	if (enable)
		glEnable(GL_STENCIL_TEST);
	else
		glDisable(GL_STENCIL_TEST);
}

void jRHI_OpenGL::SetStencilOpSeparate(EFace face, EStencilOp sFail, EStencilOp dpFail, EStencilOp dpPass) const
{
	unsigned int face_gl = 0;
	switch (face)
	{
	case EFace::BACK:
		face_gl = GL_BACK;
		break;
	case EFace::FRONT:
		face_gl = GL_FRONT;
		break;
	case EFace::FRONT_AND_BACK:
		face_gl = GL_FRONT_AND_BACK;
		break;
	}

	auto func = [](EStencilOp op)
	{
		switch (op)
		{
		case EStencilOp::KEEP:		return GL_KEEP;
		case EStencilOp::ZERO:		return GL_ZERO;
		case EStencilOp::REPLACE:	return GL_REPLACE;
		case EStencilOp::INCR:		return GL_INCR;
		case EStencilOp::INCR_WRAP:	return GL_INCR_WRAP;
		case EStencilOp::DECR:		return GL_DECR;
		case EStencilOp::DECR_WRAP:	return GL_DECR_WRAP;
		case EStencilOp::INVERT:	return GL_INVERT;
		default:					return GL_NONE;
		}
	};

	glStencilOpSeparate(face_gl, func(sFail), func(dpFail), func(dpPass));
}

void jRHI_OpenGL::SetStencilFunc(EComparisonFunc func, int32 ref, uint32 mask) const
{
	uint32 func_gl = GetOpenGLComparisonFunc(func);
	glStencilFunc(func_gl, ref, mask);
}

void jRHI_OpenGL::SetDepthFunc(EComparisonFunc func) const
{
	unsigned int func_gl = GetOpenGLComparisonFunc(func);
	glDepthFunc(func_gl);
}

void jRHI_OpenGL::SetDepthMask(bool enable) const
{
	glDepthMask(enable);
}

void jRHI_OpenGL::SetColorMask(bool r, bool g, bool b, bool a) const
{
	glColorMask(r, g, b, a);
}

IUniformBufferBlock* jRHI_OpenGL::CreateUniformBufferBlock(const char* blockname) const
{
	auto uniformBufferBlock = new jUniformBufferBlock_OpenGL(blockname);
	uniformBufferBlock->Init();
	return uniformBufferBlock;
}

//////////////////////////////////////////////////////////////////////////
void jVertexBuffer_OpenGL::Bind(const jShader* shader) const
{
	g_rhi->BindVertexBuffer(this, shader);
}

void jIndexBuffer_OpenGL::Bind(const jShader* shader) const
{
	g_rhi->BindIndexBuffer(this, shader);
}

bool jRenderTarget_OpenGL::Begin(int index, bool mrt) const
{
	if (mrt && mrt_fbo)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, mrt_fbo);
		if (mrt_drawBuffers.empty())
			glDrawBuffer(GL_NONE);
		else
			glDrawBuffers(static_cast<int32>(mrt_drawBuffers.size()), &mrt_drawBuffers[0]);
	}
	else
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbos[index]);
		if (drawBuffers.empty())
			glDrawBuffer(GL_NONE);
		else
			glDrawBuffers(static_cast<int32>(drawBuffers.size()), &drawBuffers[0]);
	}

	glViewport(0, 0, Info.Width, Info.Height);
	//glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	return true;
}

void jRenderTarget_OpenGL::End() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
}

bool jRenderTarget_OpenGL::SetDepthAttachment(const std::shared_ptr<jTexture>& InDepth)
{
	if (!__super::SetDepthAttachment(InDepth))
		return false;

	const jTexture_OpenGL* tex_gl = static_cast<jTexture_OpenGL*>(InDepth.get());

	uint32 depthBufferFormat = 0;
	uint32 depthBufferType = 0;
	const bool hasDepthAttachment = GetDepthBufferFormatAndType(depthBufferFormat, depthBufferType, tex_gl->DepthBufferType);
	JASSERT(hasDepthAttachment);
	if (!hasDepthAttachment)
		return false;

	JASSERT(fbos.size() == 1);
	glBindFramebuffer(GL_FRAMEBUFFER, fbos[0]);
	glFramebufferTexture2D(GL_FRAMEBUFFER, depthBufferType, GetOpenGLTextureType(tex_gl->TextureType), tex_gl->TextureID, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		auto status_code = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		char szTemp[256] = { 0, };
		sprintf_s(szTemp, sizeof(szTemp), "Failed to create Texture2D framebuffer which is not complete : %d", status_code);
		JMESSAGE(szTemp);
		return false;
	}

	return true;
}

void jRenderTarget_OpenGL::SetDepthMipLevel(int32 InLevel)
{
	const jTexture_OpenGL* tex_gl = static_cast<jTexture_OpenGL*>(TextureDepth.get());
	JASSERT(tex_gl);

	uint32 depthBufferFormat = 0;
	uint32 depthBufferType = 0;
	const bool hasDepthAttachment = GetDepthBufferFormatAndType(depthBufferFormat, depthBufferType, tex_gl->DepthBufferType);

	glFramebufferTexture2D(GL_FRAMEBUFFER, depthBufferType, GetOpenGLTextureType(tex_gl->TextureType), tex_gl->TextureID, InLevel);
}

//////////////////////////////////////////////////////////////////////////
void jUniformBufferBlock_OpenGL::UpdateBufferData(const void* newData, size_t size)
{
	Size = size;
	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferData(GL_UNIFORM_BUFFER, size, newData, GL_DYNAMIC_DRAW);
}

void jUniformBufferBlock_OpenGL::ClearBuffer(int32 clearValue)
{
	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glClearBufferData(GL_UNIFORM_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &clearValue);
}

void jUniformBufferBlock_OpenGL::Init()
{
	BindingPoint = GetBindPoint();
	glGenBuffers(1, &UBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, BindingPoint, UBO);
}

void jUniformBufferBlock_OpenGL::Bind(const jShader* shader) const
{
	if (-1 == BindingPoint)
		return;

	auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
	uint32 index = glGetUniformBlockIndex(shader_gl->program, Name.c_str());
	if (-1 != index)
		glUniformBlockBinding(shader_gl->program, index, BindingPoint);
}

void jShaderStorageBufferObject_OpenGL::Init()
{
	BindingPoint = GetBindPoint();
	glGenBuffers(1, &SSBO);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BindingPoint, SSBO);
}

void jShaderStorageBufferObject_OpenGL::UpdateBufferData(void* newData, size_t size)
{
	Size = size;
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, size, newData, GL_DYNAMIC_COPY);
}

void jShaderStorageBufferObject_OpenGL::ClearBuffer(int32 clearValue)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
	glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &clearValue);
}

void jShaderStorageBufferObject_OpenGL::GetBufferData(void* newData, size_t size)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
	void* p = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	if (p)
		memcpy(newData, p, size);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}

void jShaderStorageBufferObject_OpenGL::Bind(const jShader* shader) const
{
	if (-1 == BindingPoint)
		return;

	auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
	uint32 index = glGetProgramResourceIndex(shader_gl->program, GL_SHADER_STORAGE_BLOCK, Name.c_str());
	if (-1 != index)
		glShaderStorageBlockBinding(shader_gl->program, index, BindingPoint);
}

void jAtomicCounterBuffer_OpenGL::Init()
{
	glGenBuffers(1, &ACBO);
	glBindBufferBase(GL_ATOMIC_COUNTER_BUFFER, BindingPoint, ACBO);
}

void jAtomicCounterBuffer_OpenGL::UpdateBufferData(void* newData, size_t size)
{
	Size = size;
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ACBO);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, size, newData, GL_DYNAMIC_COPY);
}

void jAtomicCounterBuffer_OpenGL::GetBufferData(void* newData, size_t size)
{
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ACBO);
	void* p = glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_READ_ONLY);
	if (p)
		memcpy(newData, p, size);
	glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);
}

void jAtomicCounterBuffer_OpenGL::ClearBuffer(int32 clearValue)
{
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ACBO);
	glClearBufferData(GL_ATOMIC_COUNTER_BUFFER, GL_R32I, GL_RED_INTEGER, GL_INT, &clearValue);
}

void jAtomicCounterBuffer_OpenGL::Bind(const jShader* shader) const
{
	if (-1 == BindingPoint)
		return;

	//glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, ACBO);
	//void* p = glMapBuffer(GL_ATOMIC_COUNTER_BUFFER, GL_WRITE_ONLY);
	//if (Data)
	//	memcpy(p, Data, Size);
	//glUnmapBuffer(GL_ATOMIC_COUNTER_BUFFER);

	//auto shader_gl = static_cast<const jShader_OpenGL*>(shader);
	//uint32 index = glGetUniformLocation(shader_gl->program, Name.c_str());
	//uint32 index = glGetProgramResourceIndex(shader_gl->program, GL_ATOMIC_COUNTER_BUFFER, Name.c_str());
	//if (-1 != index)
	//	glShaderStorageBlockBinding(shader_gl->program, index, BindingPoint);
}

int32 jShader_OpenGL::TryGetUniformLocation(const char* name) const
{
	{
		auto rhi_gl = static_cast<jRHI_OpenGL*>(g_rhi);
		return rhi_gl->GetUniformLocation(program, name);
	}

	size_t HashResult = 0;
	hash_combine(HashResult, name);

	//std::string HashResult = name;
	
	auto it_find = UniformNameMap.find(HashResult);
	bool found =  (it_find != UniformNameMap.end());
	if (!found)
	{
		auto rhi_gl = static_cast<jRHI_OpenGL*>(g_rhi);
		int32 UniformLocation = rhi_gl->GetUniformLocation(program, name);
		UniformNameMap[HashResult] = UniformLocation;
		return UniformLocation;
	}
	return it_find->second;
}
