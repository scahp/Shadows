#pragma once

template <int T>
struct Int2Type
{
	enum { value = T };
};

using true_type = Int2Type<1>;
using false_type = Int2Type<0>;

using identity_type = true_type;
#define IdentityType identity_type()

using zero_type = false_type;
#define ZeroType zero_type()

template <typename T>
FORCEINLINE T Aligned(T A, T Align)
{
	return (A + (Align - 1)) & ~(Align - 1);
}