﻿#pragma once

#include <type_traits>
#include "Core/jName.h"

#define DECLARE_ENUM_BIT_OPERATORS(ENUM_TYPE)\
FORCEINLINE ENUM_TYPE operator | (ENUM_TYPE lhs, ENUM_TYPE rhs)\
{\
	using T = std::underlying_type<ENUM_TYPE>::type;\
	return static_cast<ENUM_TYPE>(static_cast<T>(lhs) | static_cast<T>(rhs));\
}\
FORCEINLINE ENUM_TYPE operator & (ENUM_TYPE lhs, ENUM_TYPE rhs)\
{\
	using T = std::underlying_type<ENUM_TYPE>::type;\
	return static_cast<ENUM_TYPE>(static_cast<T>(lhs) & static_cast<T>(rhs));\
}\
FORCEINLINE decltype(auto) operator ! (ENUM_TYPE value)\
{\
	using T = std::underlying_type<ENUM_TYPE>::type;\
	return !static_cast<T>(value);\
}

enum class EPrimitiveType
{
	POINTS,
	LINES,
	LINES_ADJACENCY,
	LINE_STRIP_ADJACENCY,
	TRIANGLES,
	TRIANGLE_STRIP,
	TRIANGLES_ADJACENCY,
	TRIANGLE_STRIP_ADJACENCY,
	MAX
};

enum class EBufferType
{
	STATIC,
	DYNAMIC
};

enum class EBufferElementType
{
	BYTE,
	UNSIGNED_INT,
	FLOAT,
	MAX,
};

struct IStreamParam
{
	IStreamParam() = default;
	IStreamParam(const jName& name, const EBufferType& bufferType, int32 stride, EBufferElementType elementType, int32 elementTypeSize, int32 instanceDivisor = 0)
		: Name(name), BufferType(bufferType), Stride(stride), ElementType(elementType), ElementTypeSize(elementTypeSize), InstanceDivisor(instanceDivisor)
	{}
	virtual ~IStreamParam() {}

	jName Name;
	EBufferType BufferType = EBufferType::STATIC;
	int32 Stride = 0;
	EBufferElementType ElementType = EBufferElementType::BYTE;
	int32 ElementTypeSize = 0;
	int32 InstanceDivisor = 0;

	virtual const void* GetBufferData() const = 0;
	virtual size_t GetBufferSize() const { return 0; }
	virtual size_t GetElementSize() const { return 0; }
};

template <typename T>
struct jStreamParam : public IStreamParam
{
	static constexpr int ElementSize = sizeof(T);

	std::vector<T> Data;

	virtual const void* GetBufferData() const override { return &Data[0]; }
	virtual size_t GetBufferSize() const override { return Data.size() * ElementSize; }
	virtual size_t GetElementSize() const override { return ElementSize; }
};

struct jVertexStreamData : public std::enable_shared_from_this<jVertexStreamData>
{
	~jVertexStreamData()
	{
		for (auto param : Params)
			delete param;
		Params.clear();
	}

	std::vector<IStreamParam*> Params;
	EPrimitiveType PrimitiveType;
	int ElementCount = 0;
};

struct jIndexStreamData : public std::enable_shared_from_this<jIndexStreamData>
{
	~jIndexStreamData()
	{
		delete Param;
		Param = nullptr;
	}

	IStreamParam* Param = nullptr;
	int32 ElementCount = 0;
};

enum class ETextureFilterTarget
{
	MINIFICATION = 0,
	MAGNIFICATION,
	MAX
};

enum class ETextureFilter
{
	NEAREST = 0,
	LINEAR,
	NEAREST_MIPMAP_NEAREST,
	LINEAR_MIPMAP_NEAREST,
	NEAREST_MIPMAP_LINEAR,
	LINEAR_MIPMAP_LINEAR,
	MAX
};

enum class ETextureType
{
	TEXTURE_2D = 0,
	TEXTURE_2D_ARRAY,
	TEXTURE_CUBE,
	TEXTURE_2D_ARRAY_OMNISHADOW,
	TEXTURE_2D_MULTISAMPLE,
	TEXTURE_2D_ARRAY_MULTISAMPLE,
	TEXTURE_2D_ARRAY_OMNISHADOW_MULTISAMPLE,
	MAX,
};

enum class ETextureFormat
{
	// TextureFormat + InternalFormat
	RGB32F = 0,
	RGB16F,
	R11G11B10F,
	RGB,
	RGBA16F,
	RGBA32F,
	RGBA,
	RG32F,
	RG,
	R,
	R32F,

	// below is Internal Format only
	RGBA_INTEGER,
	R_INTEGER,
	R32UI,
	RGBA8,
	RGBA8I,
	RGBA8UI,
	DEPTH,
	MAX,
};

enum class EFormatType
{
	BYTE = 0,
	UNSIGNED_BYTE,
	INT,
	UNSIGNED_INT,
	HALF,
	FLOAT,
	MAX
};

enum class EBlendSrc
{
	ZERO = 0,
	ONE,
	SRC_COLOR,
	ONE_MINUS_SRC_COLOR,
	DST_COLOR,
	ONE_MINUS_DST_COLOR,
	SRC_ALPHA,
	ONE_MINUS_SRC_ALPHA,
	DST_ALPHA,
	ONE_MINUS_DST_ALPHA,
	CONSTANT_COLOR,
	ONE_MINUS_CONSTANT_COLOR,
	CONSTANT_ALPHA,
	ONE_MINUS_CONSTANT_ALPHA,
	SRC_ALPHA_SATURATE,
	MAX
};

enum class EBlendDest
{
	ZERO = 0,
	ONE,
	SRC_COLOR,
	ONE_MINUS_SRC_COLOR,
	DST_COLOR,
	ONE_MINUS_DST_COLOR,
	SRC_ALPHA,
	ONE_MINUS_SRC_ALPHA,
	DST_ALPHA,
	ONE_MINUS_DST_ALPHA,
	CONSTANT_COLOR,
	ONE_MINUS_CONSTANT_COLOR,
	CONSTANT_ALPHA,
	ONE_MINUS_CONSTANT_ALPHA,
	MAX
};

enum class EBlendEquation
{
	ADD = 0,
	SUBTRACT,
	REVERSE_SUBTRACT,
	MIN_VALUE,
	MAX_VALUE,
	MAX,
};

enum class EFace
{
	FRONT = 0,
	BACK,
	FRONT_AND_BACK,
	MAX,
};

enum class EStencilOp
{
	KEEP = 0,
	ZERO,
	REPLACE,
	INCR,
	INCR_WRAP,
	DECR,
	DECR_WRAP,
	INVERT,
	MAX
};

enum class EComparisonFunc
{
	NEVER = 0,
	LESS,
	EQUAL,
	LEQUAL,
	GREATER,
	NOTEQUAL,
	GEQUAL,
	ALWAYS,
	MAX
};

enum class ERenderBufferType : uint32
{
	NONE = 0,
	COLOR = 0x00000001,
	DEPTH = 0x00000002,
	STENCIL = 0x00000004,
	MAX = 0xffffffff
};

DECLARE_ENUM_BIT_OPERATORS(ERenderBufferType);

enum class EDrawBufferType
{
	COLOR_ATTACHMENT0 = 0,
	COLOR_ATTACHMENT1,
	COLOR_ATTACHMENT2,
	COLOR_ATTACHMENT3,
	COLOR_ATTACHMENT4,
	COLOR_ATTACHMENT5,
	COLOR_ATTACHMENT6,
	COLOR_ATTACHMENT7,
	MAX
};

enum class EDepthBufferType
{
	NONE = 0,
	DEPTH16,
	DEPTH24,
	DEPTH32,
	DEPTH24_STENCIL8,
	MAX,
};

enum class ETextureAddressMode
{
	REPEAT = 0,
	MIRRORED_REPEAT,
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER,
	MAX,
};

enum class EDepthComparionFunc
{
	NEVER = 0,
	LESS,
	EQUAL,
	LESS_EQUAL,
	GREATER,
	NOT_EQUAL,
	GREATER_EQUAL,
	ALWAYS,
	MAX,
};

enum class ETextureComparisonMode
{
	NONE = 0,
	COMPARE_REF_TO_TEXTURE,					// to use PCF filtering by using samplerXXShadow series.
	MAX,
};

enum class EPolygonMode
{
	POINT = 0,
	LINE,
	FILL,
	MAX
};

enum class EImageTextureAccessType
{
	NONE = 0,
	READ_ONLY,
	WRITE_ONLY,
	READ_WRITE,
	MAX,
};

enum class EFrontFace
{
	CW = 0,
	CCW = 1,
	MAX,
};

enum class ECullMode
{
	BACK = 0,
	FRONT,
	FRONT_AND_BACK
};