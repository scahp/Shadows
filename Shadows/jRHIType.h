#pragma once

#include <type_traits>

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
	INT,
	FLOAT,
	MAX,
};

struct IStreamParam
{
	IStreamParam() = default;
	IStreamParam(const std::string& name, const EBufferType& bufferType, int32 stride, EBufferElementType elementType, int32 elementTypeSize, int32 instanceDivisor = 0)
		: Name(name), BufferType(bufferType), Stride(stride), ElementType(elementType), ElementTypeSize(elementTypeSize), InstanceDivisor(instanceDivisor)
	{}
	virtual ~IStreamParam() {}

	std::string Name;
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
};

enum class ETextureFilter
{
	NEAREST = 0,
	LINEAR,
	NEAREST_MIPMAP_NEAREST,
	LINEAR_MIPMAP_NEAREST,
	NEAREST_MIPMAP_LINEAR,
	LINEAR_MIPMAP_LINEAR,
};

enum class ETextureType
{
	TEXTURE_2D = 0,
	TEXTURE_2D_ARRAY,
	TEXTURE_CUBE,
	TEXTURE_2D_ARRAY_OMNISHADOW,
	MAX,
};

enum class ETextureFormat
{
	RGB = 0,
	RGBA = 1,
	RG,
	R,
	R32F,
	RG32F,
	RGBA16F,
	RGBA32F,
	R11G11B10F,
	RGB16F,
	RGB32F,
	DEPTH,
	MAX,
};

enum class EFormatType
{
	BYTE = 0,
	UNSIGNED_BYTE,
	INT,
	FLOAT,
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
	ALWAYS
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
	COMPARE_REF_TO_TEXTURE,
	MAX,
};

enum class EImageTextureAccessType
{
	NONE = 0,
	READ_ONLY,
	WRITE_ONLY,
	READ_WRITE,
	MAX,
};
