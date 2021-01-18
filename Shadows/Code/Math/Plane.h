#pragma once

#include "Vector.h"

struct jPlane
{
	jPlane() = default;

	jPlane(float nx, float ny, float nz, float distance)
		: n(nx, ny, nz), d(distance)
	{}

	jPlane(Vector normal, float distance)
		: n(normal), d(distance)
	{}

	static jPlane CreateFrustumFromThreePoints(const Vector& p0, const Vector& p1, const Vector& p2)
	{
		const auto direction0 = p1 - p0;
		const auto direction1 = p2 - p0;
		const auto normal = direction0.CrossProduct(direction1).GetNormalize();
		const auto distance = p2.DotProduct(normal);
		return jPlane(normal, distance);
	}

	Vector n;
	float d;
};
