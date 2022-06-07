#include <pch.h>
#include "Vector.h"
#include "Matrix.h"

const Vector Vector::OneVector = Vector(1.0f);
const Vector Vector::ZeroVector = Vector(ZeroType);
const Vector Vector::FowardVector = Vector(0.0f, 0.0f, 1.0f);
const Vector Vector::RightVector = Vector(1.0f, 0.0f, 0.0f);
const Vector Vector::UpVector = Vector(0.0f, 1.0f, 0.0f);

Vector Vector::GetEulerAngleFrom_Deprecated() const
{
	return Vector(acosf(y), atan2f(x, z), 0.0f);
}

Vector Vector::GetEulerAngleFrom() const
{
	// https://math.stackexchange.com/questions/1791209/euler-angle-to-direction-vector-which-is-right
	// Vector(0, 0, 1) 이 기본 벡터 방향으로 가정함.
	constexpr float YawOffset = DegreeToRadian(90.0f);
	return Vector(-atan2f(y, z), acosf(x) - YawOffset, 0.0f);
}

Vector Vector::GetDirectionFromEulerAngle() const
{
	return Matrix::MakeRotate(x, y, z).TransformDirection(Vector::FowardVector);
}

// Vector
void Vector::operator=(struct Vector2 const& vector)
{
	x = vector.x;
	y = vector.y;
	z = 0.0f;
}

void Vector::operator=(struct Vector4 const& vector)
{
	x = vector.x;
	y = vector.y;
	z = vector.y;
}

//////////////////////////////////////////////////////////////////////////

// Vector4
const Vector4 Vector4::OneVector = Vector4(1.0f);
const Vector4 Vector4::ZeroVector = Vector4(ZeroType);
const Vector4 Vector4::FowardVector = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
const Vector4 Vector4::RightVector = Vector4(1.0f, 0.0f, 0.0f, 0.0f);
const Vector4 Vector4::UpVector = Vector4(0.0f, 1.0f, 0.0f, 0.0f);
const Vector4 Vector4::ColorRed = Vector4(1.0f, 0.0f, 0.0f, 1.0f);
const Vector4 Vector4::ColorWhite = Vector4(1.0f, 1.0f, 1.0f, 1.0f);

void Vector4::operator=(struct Vector2 const& vector)
{
	x = vector.x;
	y = vector.y;
	z = 0.0f;
	w = 0.0f;
}

void Vector4::operator=(struct Vector const& vector)
{
	x = vector.x;
	y = vector.y;
	z = vector.z;
	w = 0.0f;
}

//////////////////////////////////////////////////////////////////////////


// Vector2
void Vector2::operator=(struct Vector const& vector)
{
	x = vector.x;
	y = vector.y;
}

void Vector2::operator=(struct Vector4 const& vector)
{
	x = vector.x;
	y = vector.y;
}
//////////////////////////////////////////////////////////////////////////
