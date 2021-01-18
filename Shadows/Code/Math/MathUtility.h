#pragma once

static constexpr float FLOAT_TOLERANCE = 0.00001f;
static constexpr float PI = 3.141592653f;

template <typename T>
FORCEINLINE void Swap(T& A, T& B)
{
	T t = A;
	A = B;
	B = t;
}

FORCEINLINE bool IsNearlyEqual(float fA, float fB, float fTolerance = FLOAT_TOLERANCE)
{
	return fabs(fA - fB) <= fTolerance;
}

FORCEINLINE bool IsNearlyZero(float fValue, float fTolerance = FLOAT_TOLERANCE)
{
	return fabs(fValue) < FLOAT_TOLERANCE;
}

FORCEINLINE float RadianToDegree(float fRadian)
{
	static constexpr float ToDegree = 180.0f / PI;
	return fRadian * ToDegree;
}

FORCEINLINE float DegreeToRadian(float fDegree)
{
	static constexpr float ToRadian = PI / 180.0f;
	return fDegree * ToRadian;
}

template <typename T>
FORCEINLINE T Clamp(T const& A, T const& Min, T const& Max)
{
	JASSERT(Max >= Min);

	if (Max <= A)
		return Max;
	
	if (Min >= A)
		return Min;

	return A;
}

template <typename T>
FORCEINLINE T Saturate(T const& A)
{
	return Clamp(A, T(0.0f), T(1.0f));
}

template <typename T>
FORCEINLINE T Lerp(const T& A, const T& B, float t)
{
	return (A + (B - A) * t);
}

template <typename T>
FORCEINLINE T Max(T const& A, T const& B)
{
	return (A > B) ? A : B;
}

template <typename T>
FORCEINLINE T Min(T const& A, T const& B)
{
	return (A < B) ? A : B;
}