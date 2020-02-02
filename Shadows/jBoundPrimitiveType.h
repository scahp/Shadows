#pragma once
#include "Math/Vector.h"

struct jBoundBox
{
	static jBoundBox GenerateBoundBox(const std::vector<float>& vertices)
	{
		auto min = Vector(FLT_MAX);
		auto max = Vector(FLT_MIN);
		for (size_t i = 0; i < vertices.size() / 3; ++i)
		{
			auto curIndex = i * 3;
			auto x = vertices[curIndex];
			auto y = vertices[curIndex + 1];
			auto z = vertices[curIndex + 2];
			if (max.x < x)
				max.x = x;
			if (max.y < y)
				max.y = y;
			if (max.z < z)
				max.z = z;

			if (min.x > x)
				min.x = x;
			if (min.y > y)
				min.y = y;
			if (min.z > z)
				min.z = z;
		}

		return { min, max };
	}

	FORCEINLINE void CreateBoundBox(const std::vector<float>& vertices) { *this = jBoundBox::GenerateBoundBox(vertices); }
	FORCEINLINE Vector GetExtent() const { return (Max - Min); }
	FORCEINLINE Vector GetHalfExtent() const { return GetExtent() / 0.5f; }

	Vector Min;
	Vector Max;
};

struct jBoundSphere
{
	float Radius = 0.0f;
};
