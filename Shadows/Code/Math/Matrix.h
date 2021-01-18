#pragma once

#include "Vector.h"
#include <utility>

struct Matrix3;

struct Matrix
{
	FORCEINLINE Matrix() {}
	FORCEINLINE Matrix(identity_type /*IdentityType*/) { SetIdentity(); }
	FORCEINLINE Matrix(zero_type /*ZeroType*/) { SetZero(); }
	FORCEINLINE Matrix(float fM00, float fM01, float fM02, float fM03
	     , float fM10, float fM11, float fM12, float fM13
	     , float fM20, float fM21, float fM22, float fM23
	     , float fM30, float fM31, float fM32, float fM33)
		: m00(fM00), m01(fM01), m02(fM02), m03(fM03)
		, m10(fM10), m11(fM11), m12(fM12), m13(fM13)
		, m20(fM20), m21(fM21), m22(fM22), m23(fM23)
		, m30(fM30), m31(fM31), m32(fM32), m33(fM33)
	{}

	FORCEINLINE Matrix(Matrix const& matrix)
	{
		*this = matrix;
	}

	FORCEINLINE Matrix(float* pMM)
	{
		JASSERT(pMM);
		memcpy(mm, pMM, sizeof(mm));
	}

	FORCEINLINE Matrix(Matrix3 const& matrix);

	FORCEINLINE Matrix& SetIdentity()
	{
		m00 = m11 = m22 = m33 = 1.0f;
		m01 = m02 = m03 
			= m10 = m12 = m13 
			= m20 = m21 = m23 
			= m30 = m31 = m32 = 0.0f;
		return *this;
	}

	FORCEINLINE Matrix& SetZero()
	{
		memset(mm, 0, sizeof(mm));
		return *this;
	}

	FORCEINLINE Matrix& SetTranspose()
	{
		Swap(m01, m10); Swap(m02, m20); Swap(m03, m30);
		Swap(m12, m21); Swap(m13, m31); Swap(m23, m32);
		return *this;
	}

	FORCEINLINE Matrix GetTranspose() const
	{
		return Matrix(*this).SetTranspose();
	}

	FORCEINLINE Matrix& SetInverse() 
	{
		*this = GetInverse();
		return *this;
	}

	FORCEINLINE float GetDeterminant() const
	{
		// Upper 2 rows's 2x2 Det
		float const A = m11 * m00 - m10 * m01;
		float const B = m12 * m00 - m10 * m02;
		float const C = m13 * m00 - m10 * m03;
		float const D = m12 * m01 - m11 * m02;
		float const E = m13 * m01 - m11 * m03;
		float const F = m13 * m02 - m12 * m03;
		// Lower 2 row's 2x2 Det
		float const G = m31 * m20 - m30 * m21;
		float const H = m32 * m20 - m30 * m22;
		float const I = m33 * m20 - m30 * m23;
		float const J = m32 * m21 - m31 * m22;
		float const K = m33 * m21 - m31 * m23;
		float const L = m33 * m22 - m32 * m23;

		// ��Ľ� ��귮 ����ȭ
		// Det calculate optimize
		//float const det = m00 * (m11 * L - m12 * K + m13 * J) - m10 * (m01 * L - m02 * K + m03 * J)
		//              + m20 * (m31 * F - m32 * E + m33 * D) - m30 * (m21 * F - m22 * E + m23 * D);
		//(m11 * m00 - m10 * m01) * L;
		//-(m12 * m00 - m10 * m02) * K;
		//(m13 * m00 - m10 * m03) * J;
		//(m31 * m20 - m30 * m21) * F;
		//-(m32 * m20 - m30 * m22) * E;
		//(m33 * m20 - m30 * m23) * D;
		return A * L -B * K + C * J + G * F -H * E + I * D;
	}

	FORCEINLINE Matrix GetInverse() const
	{
		// Upper 2 rows's 2x2 Det
		float const A = m11 * m00 - m10 * m01;
		float const B = m12 * m00 - m10 * m02;
		float const C = m13 * m00 - m10 * m03;
		float const D = m12 * m01 - m11 * m02;
		float const E = m13 * m01 - m11 * m03;
		float const F = m13 * m02 - m12 * m03;
		// Lower 2 row's 2x2 Det
		float const G = m31 * m20 - m30 * m21;
		float const H = m32 * m20 - m30 * m22;
		float const I = m33 * m20 - m30 * m23;
		float const J = m32 * m21 - m31 * m22;
		float const K = m33 * m21 - m31 * m23;
		float const L = m33 * m22 - m32 * m23;

		// ��Ľ� ��귮 ����ȭ
		// Det calculate optimize
		//float const det = m00 * (m11 * L - m12 * K + m13 * J) - m10 * (m01 * L - m02 * K + m03 * J)
		//              + m20 * (m31 * F - m32 * E + m33 * D) - m30 * (m21 * F - m22 * E + m23 * D);
		//(m11 * m00 - m10 * m01) * L;
		//-(m12 * m00 - m10 * m02) * K;
		//(m13 * m00 - m10 * m03) * J;
		//(m31 * m20 - m30 * m21) * F;
		//-(m32 * m20 - m30 * m22) * E;
		//(m33 * m20 - m30 * m23) * D;
		float const det = A * L -B * K + C * J + G * F -H * E + I * D;
		if (0.0f == det)
		{
			JASSERT("������� ���� �� �����ϴ�.");
			return *this;
		}

		float const InverseDet = 1.0f / det;
		// �������. (-1)^(i+j) * M[i][j] �� ����� ���� �� ��ġ ��Ų ���
		// Adjoint matrix. Make a matrix (-1)^(i+j) * M[i][j] and then transpose the matrix.
		Matrix inverseMatrix;
		inverseMatrix.m[0][0] = (m11 * L - m12 * K + m13 * J) * InverseDet;
		inverseMatrix.m[1][0] = -(m10 * L - m12 * I + m13 * H) * InverseDet;
		inverseMatrix.m[2][0] = (m10 * K - m11 * I + m13 * G) * InverseDet;
		inverseMatrix.m[3][0] = -(m10 * J - m11 * H + m12 * G) * InverseDet;
		inverseMatrix.m[0][1] = -(m01 * L - m02 * K + m03 * J) * InverseDet;
		inverseMatrix.m[1][1] = (m00 * L - m02 * I + m03 * H) * InverseDet;
		inverseMatrix.m[2][1] = -(m00 * K - m01 * I + m03 * G) * InverseDet;
		inverseMatrix.m[3][1] = (m00 * J - m01 * H + m02 * G) * InverseDet;
		inverseMatrix.m[0][2] = (m31 * F - m32 * E + m33 * D) * InverseDet;
		inverseMatrix.m[1][2] = -(m30 * F - m32 * C + m33 * B) * InverseDet;
		inverseMatrix.m[2][2] = (m30 * E - m31 * C + m33 * A) * InverseDet;
		inverseMatrix.m[3][2] = -(m30 * D - m31 * B + m32 * A) * InverseDet;
		inverseMatrix.m[0][3] = -(m21 * F - m22 * E + m23 * D) * InverseDet;
		inverseMatrix.m[1][3] = (m20 * F - m22 * C + m23 * B) * InverseDet;
		inverseMatrix.m[2][3] = -(m20 * E - m21 * C + m23 * A) * InverseDet;
		inverseMatrix.m[3][3] = (m20 * D - m21 * B + m22 * A) * InverseDet;

		return std::move(inverseMatrix);
	}

	FORCEINLINE bool operator==(Matrix const& matrix) const
	{
		return IsNearlyEqual(m00, matrix.m00) && IsNearlyEqual(m01, matrix.m01) && IsNearlyEqual(m02, matrix.m02) && IsNearlyEqual(m03, matrix.m03)
			&& IsNearlyEqual(m10, matrix.m10) && IsNearlyEqual(m11, matrix.m11) && IsNearlyEqual(m12, matrix.m12) && IsNearlyEqual(m13, matrix.m13)
			&& IsNearlyEqual(m20, matrix.m20) && IsNearlyEqual(m21, matrix.m21) && IsNearlyEqual(m22, matrix.m22) && IsNearlyEqual(m23, matrix.m23)
			&& IsNearlyEqual(m30, matrix.m30) && IsNearlyEqual(m31, matrix.m31) && IsNearlyEqual(m32, matrix.m32) && IsNearlyEqual(m33, matrix.m33);
	}

	FORCEINLINE bool operator!=(Matrix const& matrix) const
	{
		return !(operator==(matrix));
	}

	FORCEINLINE void operator=(Matrix const& matrix)
	{
		memcpy(mm, matrix.mm, sizeof(mm));
	}
	
	FORCEINLINE void operator=(struct Matrix3 const& matrix);

	FORCEINLINE const Matrix operator*(Matrix const& matrix) const
	{
		return Matrix(
			m00 * matrix.m00 + m01 * matrix.m10 + m02 * matrix.m20 + m03 * matrix.m30,
			m00 * matrix.m01 + m01 * matrix.m11 + m02 * matrix.m21 + m03 * matrix.m31,
			m00 * matrix.m02 + m01 * matrix.m12 + m02 * matrix.m22 + m03 * matrix.m32,
			m00 * matrix.m03 + m01 * matrix.m13 + m02 * matrix.m23 + m03 * matrix.m33,

			m10 * matrix.m00 + m11 * matrix.m10 + m12 * matrix.m20 + m13 * matrix.m30,
			m10 * matrix.m01 + m11 * matrix.m11 + m12 * matrix.m21 + m13 * matrix.m31,
			m10 * matrix.m02 + m11 * matrix.m12 + m12 * matrix.m22 + m13 * matrix.m32,
			m10 * matrix.m03 + m11 * matrix.m13 + m12 * matrix.m23 + m13 * matrix.m33,

			m20 * matrix.m00 + m21 * matrix.m10 + m22 * matrix.m20 + m23 * matrix.m30,
			m20 * matrix.m01 + m21 * matrix.m11 + m22 * matrix.m21 + m23 * matrix.m31,
			m20 * matrix.m02 + m21 * matrix.m12 + m22 * matrix.m22 + m23 * matrix.m32,
			m20 * matrix.m03 + m21 * matrix.m13 + m22 * matrix.m23 + m23 * matrix.m33,

			m30 * matrix.m00 + m31 * matrix.m10 + m32 * matrix.m20 + m33 * matrix.m30,
			m30 * matrix.m01 + m31 * matrix.m11 + m32 * matrix.m21 + m33 * matrix.m31,
			m30 * matrix.m02 + m31 * matrix.m12 + m32 * matrix.m22 + m33 * matrix.m32,
			m30 * matrix.m03 + m31 * matrix.m13 + m32 * matrix.m23 + m33 * matrix.m33
		);
	}

	FORCEINLINE const Matrix operator*=(Matrix const& matrix)
	{
		m00 = m00 * matrix.m00 + m01 * matrix.m10 + m02 * matrix.m20 + m03 * matrix.m30;
		m01 = m00 * matrix.m01 + m01 * matrix.m11 + m02 * matrix.m21 + m03 * matrix.m31;
		m02 = m00 * matrix.m02 + m01 * matrix.m12 + m02 * matrix.m22 + m03 * matrix.m32;
		m03 = m00 * matrix.m03 + m01 * matrix.m13 + m02 * matrix.m23 + m03 * matrix.m33;

		m10 = m10 * matrix.m00 + m11 * matrix.m10 + m12 * matrix.m20 + m13 * matrix.m30;
		m11 = m10 * matrix.m01 + m11 * matrix.m11 + m12 * matrix.m21 + m13 * matrix.m31;
		m12 = m10 * matrix.m02 + m11 * matrix.m12 + m12 * matrix.m22 + m13 * matrix.m32;
		m13 = m10 * matrix.m03 + m11 * matrix.m13 + m12 * matrix.m23 + m13 * matrix.m33;

		m20 = m20 * matrix.m00 + m21 * matrix.m10 + m22 * matrix.m20 + m23 * matrix.m30;
		m21 = m20 * matrix.m01 + m21 * matrix.m11 + m22 * matrix.m21 + m23 * matrix.m31;
		m22 = m20 * matrix.m02 + m21 * matrix.m12 + m22 * matrix.m22 + m23 * matrix.m32;
		m23 = m20 * matrix.m03 + m21 * matrix.m13 + m22 * matrix.m23 + m23 * matrix.m33;

		m30 = m30 * matrix.m00 + m31 * matrix.m10 + m32 * matrix.m20 + m33 * matrix.m30;
		m31 = m30 * matrix.m01 + m31 * matrix.m11 + m32 * matrix.m21 + m33 * matrix.m31;
		m32 = m30 * matrix.m02 + m31 * matrix.m12 + m32 * matrix.m22 + m33 * matrix.m32;
		m33 = m30 * matrix.m03 + m31 * matrix.m13 + m32 * matrix.m23 + m33 * matrix.m33;
	}

	// Transform
	FORCEINLINE Vector Transform(Vector const& vector) const
	{
		Vector4 result = Transform(Vector4(vector, 1.0f));
		if (IsNearlyZero(result.w))
			return result;
		return result / result.w;
	}

	FORCEINLINE Vector4 Transform(Vector4 const& vector) const
	{
		return Vector4(
			vector.x * m00 + vector.y * m01 + vector.z * m02 + vector.w * m03,
			vector.x * m10 + vector.y * m11 + vector.z * m12 + vector.w * m13,
			vector.x * m20 + vector.y * m21 + vector.z * m22 + vector.w * m23,
			vector.x * m30 + vector.y * m31 + vector.z * m32 + vector.w * m33
		);
	}

	FORCEINLINE Vector InverseTransform(Vector const& vector) const
	{
		return GetInverse().Transform(Vector4(vector, 1.0f));
	}

	FORCEINLINE Vector InverseTransform(Vector4 const& vector) const
	{
		return GetInverse().Transform(vector);
	}
	//////////////////////////////////////////////////////////////////////////


	// Translate
	FORCEINLINE Vector GetTranslateVector() const
	{
		return Vector(m03, m13, m23);
	}

	FORCEINLINE Matrix& Translate(Vector const& vector)
	{
		return Translate(vector.x, vector.y, vector.z);
	}

	FORCEINLINE Matrix& Translate(float fX, float fY, float fZ)
	{
		m03 += fX; m13 += fY; m23 += fZ;
		return *this;
	}

	FORCEINLINE Matrix& SetTranslate(Vector const& vector)
	{
		return SetTranslate(vector.x, vector.y, vector.z);
	}

	FORCEINLINE Matrix& SetTranslate(float fX, float fY, float fZ)
	{
		m03 = fX; m13 = fY; m23 = fZ;
		return *this;
	}

	FORCEINLINE Matrix GetTranslate(float fX, float fY, float fZ) const
	{
		return Matrix(*this).Translate(fX, fY, fZ);
	}

	FORCEINLINE Matrix GetTranslate(Vector const& vector) const
	{
		return GetTranslate(vector.x, vector.y, vector.z);
	}

	FORCEINLINE Matrix GetMatrixWithoutTranslate() const
	{
		return Matrix(m00, m01, m02, 0.0f, m10, m11, m12, 0.0f, m20, m21, m22, 0.0f, m30, m31, m32, m33);
	}

	static FORCEINLINE Matrix MakeTranslate(Vector const& vector)
	{
		return Matrix(
			1.0f,		0.0f,		0.0f,		vector.x,
			0.0f,		1.0f,		0.0f,		vector.y,
			0.0f,		0.0f,		1.0f,		vector.z,
			0.0f,		0.0f,		0.0f,		1.0f
		);
	}

	static FORCEINLINE Matrix MakeTranslate(float fX, float fY, float fZ)
	{
		return Matrix(
			1.0f,	0.0f,	0.0f,	fX,
			0.0f,	1.0f,	0.0f,	fY,
			0.0f,	0.0f,	1.0f,	fZ,
			0.0f,	0.0f,	0.0f,	1.0f
		);
	}
	//////////////////////////////////////////////////////////////////////////

	// Rotation
	FORCEINLINE static Vector GetRotateVector(const Matrix& matrix)
	{
		// Rotation Order : Z->X->Y
		return matrix.GetRotateVector();
	}

	FORCEINLINE Vector GetRotateVector() const
	{
		// Rotation Order : Z->X->Y
		return Vector(asinf(-m[1][2]), atanf(m[0][2] / m[2][2]), atanf(m[1][0] / m[1][1]));
	}

	FORCEINLINE Matrix& Rotate(Vector const& vector)
	{
		return Rotate(vector.x, vector.y, vector.z);
	}

	FORCEINLINE Matrix& Rotate(float fX, float fY, float fZ)
	{
		*this *= Matrix::MakeRotate(fX, fY, fZ);
		return *this;
	}

	FORCEINLINE Matrix GetRotate(float fX, float fY, float fZ) const
	{
		return Matrix(*this).Rotate(fX, fY, fZ);
	}

	FORCEINLINE Matrix GetRotate(Vector const& vector) const
	{
		return GetRotate(vector.x, vector.y, vector.z);
	}

	FORCEINLINE Matrix GetMatrixWithoutRotate() const
	{
		auto const scale = GetScaleVector();
		return Matrix(
			scale.x,		0.0f,		0.0f,		m03,
			0.0f,			scale.y,	0.0f,		m13,
			0.0f,			0.0f,		scale.z,	m23,
			0.0f,			0.0f,		0.0f,		1.0f
		);
	}

	FORCEINLINE static Matrix MakeRotateX(float fRadian) 
	{
		return Matrix(
			1.0f, 0.0f,				0.0f,			0.0f,
			0.0f, cosf(fRadian),	-sinf(fRadian),	0.0f,
			0.0f, sinf(fRadian),	cosf(fRadian),	0.0f,
			0.0f, 0.0f,				0.0f,			1.0f
		);
	}
	
	FORCEINLINE static Matrix MakeRotateY(float fRadian)
	{
		return Matrix(
			cosf(fRadian),	0.0f,	sinf(fRadian),	0.0f,
			0.0f,			1.0f,	0.0f,			0.0f,
			-sinf(fRadian),	0.0f,	cosf(fRadian),	0.0f,
			0.0f,			0.0f,	0.0f,			1.0f
		);
	}

	FORCEINLINE static Matrix MakeRotateZ(float fRadian) 
	{
		return Matrix(
			cosf(fRadian),	-sinf(fRadian),	0.0f, 0.0f,
			sinf(fRadian),	cosf(fRadian),	0.0f, 0.0f,
			0.0f,			0.0f,			1.0f, 0.0f,
			0.0f,			0.0f,			0.0f, 1.0f
		);
	}

	FORCEINLINE static Matrix MakeRotate(Vector const& vector)
	{
		return MakeRotate(vector.x, vector.y, vector.z);
	}

	FORCEINLINE static Matrix MakeRotate(float fRadianX, float fRadianY, float fRadianZ)
	{
		// Rotation Order : Z->X->Y
		float const SX = sinf(fRadianX);
		float const CX = cosf(fRadianX);
		float const SY = sinf(fRadianY);
		float const CY = cosf(fRadianY);
		float const SZ = sinf(fRadianZ);
		float const CZ = cosf(fRadianZ);

		return Matrix(
			CY * CZ + SY * SX * SZ, -CY * SZ + SY * SX * CZ, SY * CX, 0.0f
			, CX * SZ, CX * CZ, -SX, 0.0f
			, -SY * CZ + CY * SX * SZ, SY * SZ + CY * SX * CZ, CY * CX, 0.0f
			, 0.0f, 0.0f, 0.0f, 1.0f);
	}

	FORCEINLINE static Matrix MakeRotate(Vector axis, float fRadian)
	{ 
		float const fCos = cosf(fRadian);
		float const fSin = sinf(fRadian);
		float const fOneMinusCos = 1.0f - fCos;

		axis.SetNormalize();
		return Matrix(fOneMinusCos * axis.x * axis.x + fCos,
			fOneMinusCos * axis.x * axis.y - fSin * axis.z,
			fOneMinusCos * axis.x * axis.z + fSin * axis.y,
			0.0f,
			fOneMinusCos * axis.y * axis.x + fSin * axis.z,
			fOneMinusCos * axis.y * axis.y + fCos,
			fOneMinusCos * axis.y * axis.z - fSin * axis.x,
			0.0f,
			fOneMinusCos * axis.z * axis.x - fSin * axis.y,
			fOneMinusCos * axis.z * axis.y + fSin * axis.x,
			fOneMinusCos * axis.z * axis.z + fCos,
			0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
	}
	//////////////////////////////////////////////////////////////////////////

	// Scale
	FORCEINLINE Vector GetScaleVector() const
	{
		return Vector(
			sqrtf(m00 * m00 + m01 * m01 + m02 * m02)
			, sqrtf(m10 * m10 + m11 * m11 + m12 * m12)
			, sqrtf(m20 * m20 + m21 * m21 + m22 * m22)
		);
	}

	FORCEINLINE Matrix& Scale(float fValue)
	{
		m00 *= fValue; m11 *= fValue; m22 *= fValue;
		return *this;
	}

	FORCEINLINE Matrix& Scale(float fX, float fY, float fZ)
	{
		m00 *= fX; m11 *= fY; m22 *= fZ;
		return *this;
	}

	FORCEINLINE Matrix GetScale(float fValue) const
	{
		return Matrix(*this).Scale(fValue);
	}

	FORCEINLINE Matrix GetScale(float fX, float fY, float fZ) const
	{
		return Matrix(*this).Scale(fX, fY, fZ);
	}

	FORCEINLINE Matrix GetScale(Vector const& vector) const
	{
		return GetScale(vector.x, vector.y, vector.z);
	}
	
	FORCEINLINE Matrix GetMatrixWithoutScale() const
	{
		auto const scaleInv = 1.0f / GetScaleVector();
		return Matrix(
			m00 * scaleInv.x,	m01 * scaleInv.x,	m02 * scaleInv.x,	m03,
			m10 * scaleInv.y,	m11 * scaleInv.y,	m12 * scaleInv.y,	m13,
			m20 * scaleInv.z,	m21 * scaleInv.z,	m22 * scaleInv.z,	m23,
			m30,				m31,				m32,				m33
		);
	}

	FORCEINLINE static Matrix MakeScale(Vector const& vector)
	{
		return Matrix(
			vector.x,	0.0f,		0.0f,		0.0f,
			0.0f,		vector.y,	0.0f,		0.0f,
			0.0f,		0.0f,		vector.z,	0.0f,
			0.0f,		0.0f,		0.0f,		1.0f
		);
	}

	FORCEINLINE static Matrix MakeScale(float fX, float fY, float fZ)
	{
		return Matrix(
			fX,		0.0f,	0.0f,	0.0f,
			0.0f,	fY,		0.0f,	0.0f,
			0.0f,	0.0f,	fZ,		0.0f,
			0.0f,	0.0f,	0.0f,	1.0f
		);
	}
	//////////////////////////////////////////////////////////////////////////


	FORCEINLINE Vector4 GetColumn(uint32 iIndex) const
	{
		JASSERT(iIndex >= 0 && iIndex < 4);
		switch (iIndex)
		{
		case 0:	return Vector4(m00, m10, m20, m30);
		case 1:	return Vector4(m01, m11, m21, m31);
		case 2:	return Vector4(m02, m12, m22, m32);
		case 3:	return Vector4(m03, m13, m23, m33);
		}

		return Vector4(ZeroType);
	}

	FORCEINLINE Vector4 const& GetRow(unsigned int iIndex) const
	{
		JASSERT(iIndex >= 0 && iIndex < 4);
		return mRow[iIndex];
	}

	FORCEINLINE void SetColumn(uint32 iIndex, Vector4 const& data)
	{
		JASSERT(iIndex >= 0 && iIndex < 4);
		switch (iIndex)
		{
		case 0: m00 = data.x; m10 = data.y; m20 = data.z; m30 = data.w;		break;
		case 1: m01 = data.x; m11 = data.y; m21 = data.z; m31 = data.w;		break;
		case 2: m02 = data.x; m12 = data.y; m22 = data.z; m32 = data.w;		break;
		case 3: m03 = data.x; m13 = data.y; m23 = data.z; m33 = data.w;		break;
		}
	}

	FORCEINLINE void SetRow(uint32 iIndex, Vector4 const& data)
	{
		JASSERT(iIndex >= 0 && iIndex < 4);
		mRow[iIndex] = data;
	}

	FORCEINLINE void SetXBasis(Vector4 const& xAxis) { SetRow(0, xAxis); }
	FORCEINLINE void SetYBasis(Vector4 const& yAxis) { SetRow(1, yAxis); }
	FORCEINLINE void SetZBasis(Vector4 const& zAxis) { SetRow(2, zAxis); }

	union
	{
		struct  
		{
			float m00, m01, m02, m03;
			float m10, m11, m12, m13;
			float m20, m21, m22, m23;
			float m30, m31, m32, m33;
		};
		float m[4][4];
		float mm[16];
		Vector4 mRow[4];
	};
};

struct Matrix3
{
	FORCEINLINE Matrix3() {}
	FORCEINLINE Matrix3(identity_type /*IdentityType*/) { SetIdentity(); }
	FORCEINLINE Matrix3(zero_type /*ZeroType*/) { SetZero(); }
	FORCEINLINE Matrix3(float fM00, float fM01, float fM02
		, float fM10, float fM11, float fM12
		, float fM20, float fM21, float fM22)
		: m00(fM00), m01(fM01), m02(fM02)
		, m10(fM10), m11(fM11), m12(fM12)
		, m20(fM20), m21(fM21), m22(fM22)
	{}

	FORCEINLINE Matrix3(Matrix3 const& matrix)
	{
		*this = matrix;
	}

	FORCEINLINE Matrix3(float* pMM)
	{
		JASSERT(pMM);
		memcpy(mm, pMM, sizeof(mm));
	}

	FORCEINLINE Matrix3(Matrix const& matrix)
		: m00(matrix.m00), m01(matrix.m01), m02(matrix.m02)
		, m10(matrix.m10), m11(matrix.m11), m12(matrix.m12)
		, m20(matrix.m20), m21(matrix.m21), m22(matrix.m22) 
	{ }

	FORCEINLINE Matrix3& SetIdentity()
	{
		m00 = m11 = m22 = 1.0f;
		m01 = m02 = m10 = m12 = m20 = m21 = 0.0f;
		return *this;
	}

	FORCEINLINE Matrix3& SetZero()
	{
		memset(mm, 0, sizeof(mm));
		return *this;
	}

	FORCEINLINE Matrix3& SetTranspose()
	{
		Swap(m01, m10); Swap(m02, m20); Swap(m12, m21); 
		return *this;
	}

	FORCEINLINE Matrix3 GetTranspose() const
	{
		return Matrix3(*this).SetTranspose();
	}

	FORCEINLINE Matrix3& SetInverse() 
	{
		*this = GetInverse();
		return *this;
	}

	FORCEINLINE float GetDeterminant() const
	{
		// Rmove lower row
		float const A = m11 * m00 - m10 * m01;
		float const B = m12 * m00 - m10 * m02;
		float const C = m12 * m01 - m11 * m02;
		// Remove upper row
		float const D = m21 * m10 - m20 * m11;
		float const E = m22 * m10 - m20 * m12;
		float const F = m22 * m11 - m21 * m12;
		// Remove middle row
		float const G = m21 * m00 - m20 * m01;
		float const H = m22 * m00 - m20 * m02;
		float const I = m22 * m01 - m21 * m02;

		return m00 * F - m01 * E + m02 * D;
	}

	FORCEINLINE Matrix3 GetInverse() const
	{
		// Rmove lower row
		float const A = m11 * m00 - m10 * m01;
		float const B = m12 * m00 - m10 * m02;
		float const C = m12 * m01 - m11 * m02;
		// Remove upper row
		float const D = m21 * m10 - m20 * m11;
		float const E = m22 * m10 - m20 * m12;
		float const F = m22 * m11 - m21 * m12;
		// Remove middle row
		float const G = m21 * m00 - m20 * m01;
		float const H = m22 * m00 - m20 * m02;
		float const I = m22 * m01 - m21 * m02;

		float const det = m00 * F - m01 * E + m02 * D;
		if (0.0f == det)
		{
			JASSERT("������� ���� �� �����ϴ�.");
			return *this;
		}

		float const InverseDet = 1.0f / det;
		// �������. (-1)^(i+j) * M[i][j] �� ����� ���� �� ��ġ ��Ų ���
		// Adjoint matrix. Make a matrix (-1)^(i+j) * M[i][j] and then transpose the matrix.
		Matrix3 inverseMatrix;
		inverseMatrix.m[0][0] = F * InverseDet;
		inverseMatrix.m[1][0] = -E * InverseDet;
		inverseMatrix.m[2][0] = D * InverseDet;
		inverseMatrix.m[0][1] = -I * InverseDet;
		inverseMatrix.m[1][1] = H * InverseDet;
		inverseMatrix.m[2][1] = -G * InverseDet;
		inverseMatrix.m[0][2] = C * InverseDet;
		inverseMatrix.m[1][2] = -B * InverseDet;
		inverseMatrix.m[2][2] = A * InverseDet;

		return std::move(inverseMatrix);
	}

	FORCEINLINE bool operator==(Matrix3 const& matrix) const
	{
		return IsNearlyEqual(m00, matrix.m00) && IsNearlyEqual(m01, matrix.m01) && IsNearlyEqual(m02, matrix.m02)
			&& IsNearlyEqual(m10, matrix.m10) && IsNearlyEqual(m11, matrix.m11) && IsNearlyEqual(m12, matrix.m12)
			&& IsNearlyEqual(m20, matrix.m20) && IsNearlyEqual(m21, matrix.m21) && IsNearlyEqual(m22, matrix.m22);
	}

	FORCEINLINE bool operator!=(Matrix3 const& matrix) const
	{
		return !(operator==(matrix));
	}

	FORCEINLINE void operator=(Matrix3 const& matrix)
	{
		memcpy(mm, matrix.mm, sizeof(mm));
	}

	FORCEINLINE void operator=(Matrix const& matrix)
	{
		m00 = matrix.m00; m01 = matrix.m01; m02 = matrix.m02;
		m10 = matrix.m10; m11 = matrix.m11; m12 = matrix.m12;
		m20 = matrix.m20; m21 = matrix.m21; m22 = matrix.m22;
	}

	FORCEINLINE Matrix3 operator*(Matrix3 const& matrix) const
	{
		return Matrix3(
			m00 * matrix.m00 + m01 * matrix.m10 + m02 * matrix.m20,
			m00 * matrix.m01 + m01 * matrix.m11 + m02 * matrix.m21,
			m00 * matrix.m02 + m01 * matrix.m12 + m02 * matrix.m22,

			m10 * matrix.m00 + m11 * matrix.m10 + m12 * matrix.m20,
			m10 * matrix.m01 + m11 * matrix.m11 + m12 * matrix.m21,
			m10 * matrix.m02 + m11 * matrix.m12 + m12 * matrix.m22,

			m20 * matrix.m00 + m21 * matrix.m10 + m22 * matrix.m20,
			m20 * matrix.m01 + m21 * matrix.m11 + m22 * matrix.m21,
			m20 * matrix.m02 + m21 * matrix.m12 + m22 * matrix.m22
		);
	}

	FORCEINLINE Matrix3 operator*=(Matrix3 const& matrix)
	{
		m00 = m00 * matrix.m00 + m01 * matrix.m10 + m02 * matrix.m20;
		m01 = m00 * matrix.m01 + m01 * matrix.m11 + m02 * matrix.m21;
		m02 = m00 * matrix.m02 + m01 * matrix.m12 + m02 * matrix.m22;

		m10 = m10 * matrix.m00 + m11 * matrix.m10 + m12 * matrix.m20;
		m11 = m10 * matrix.m01 + m11 * matrix.m11 + m12 * matrix.m21;
		m12 = m10 * matrix.m02 + m11 * matrix.m12 + m12 * matrix.m22;

		m20 = m20 * matrix.m00 + m21 * matrix.m10 + m22 * matrix.m20;
		m21 = m20 * matrix.m01 + m21 * matrix.m11 + m22 * matrix.m21;
		m22 = m20 * matrix.m02 + m21 * matrix.m12 + m22 * matrix.m22;
		
		return *this;
	}


	// Transfrom
	FORCEINLINE Vector Transform(Vector const& vector) const
	{
		return Vector(
			vector.x * m00 + vector.y * m01 + vector.z * m02,
			vector.x * m10 + vector.y * m11 + vector.z * m12,
			vector.x * m20 + vector.y * m21 + vector.z * m22
		);
	}

	FORCEINLINE Vector InverseTransform(Vector const& vector) const
	{
		return GetInverse().Transform(vector);
	}
	//////////////////////////////////////////////////////////////////////////

	// Rotation
	FORCEINLINE static Vector GetRotateVector(const Matrix3& matrix)
	{
		// Rotation Order : Z->X->Y
		return matrix.GetRotateVector();
	}

	FORCEINLINE Vector GetRotateVector() const 
	{
		// Rotation Order : Z->X->Y
		return Vector(asinf(-m[1][2]), atanf(m[0][2] / m[2][2]), atanf(m[1][0] / m[1][1]));
	}

	FORCEINLINE Matrix3& Rotate(Vector const& vector)
	{
		return Rotate(vector.x, vector.y, vector.z);
	}

	FORCEINLINE Matrix3& Rotate(float fX, float fY, float fZ)
	{
		*this *= Matrix3::MakeRotate(fX, fY, fZ);
		return *this;
	}

	FORCEINLINE Matrix3 GetRotate(float fX, float fY, float fZ) const
	{
		return Matrix3(*this).Rotate(fX, fY, fZ);
	}

	FORCEINLINE Matrix3 GetRotate(Vector const& vector) const
	{
		return GetRotate(vector.x, vector.y, vector.z);
	}

	FORCEINLINE Matrix3 GetMatrixWithoutRotate() const 
	{ 
		auto const scale = GetScaleVector();
		return Matrix3(
			scale.x,		0.0f,		0.0f,
			0.0f,			scale.y,	0.0f,
			0.0f,			0.0f,		scale.z
		);
	}

	FORCEINLINE static Matrix3 MakeRotateX(float fRadian) 
	{
		return Matrix3(
			1.0f, 0.0f,				0.0f,
			0.0f, cosf(fRadian),	-sinf(fRadian),
			0.0f, sinf(fRadian),	cosf(fRadian)
		);
	}
	
	FORCEINLINE static Matrix3 MakeRotateY(float fRadian)
	{
		return Matrix3(
			cosf(fRadian),	0.0f,	sinf(fRadian),
			0.0f,			1.0f,	0.0f,
			-sinf(fRadian),	0.0f,	cosf(fRadian)
		);
	}

	FORCEINLINE static Matrix3 MakeRotateZ(float fRadian) 
	{
		return Matrix3(
			cosf(fRadian),	-sinf(fRadian),	0.0f,
			sinf(fRadian),	cosf(fRadian),	0.0f,
			0.0f,			0.0f,			1.0f
		);
	}

	FORCEINLINE static Matrix3 MakeRotate(Vector const& vector)
	{
		return MakeRotate(vector.x, vector.y, vector.z);
	}

	FORCEINLINE static Matrix3 MakeRotate(float fRadianX, float fRadianY, float fRadianZ)
	{
		// Rotation Order : Z->X->Y
		float const SX = sinf(fRadianX);
		float const CX = cosf(fRadianX);
		float const SY = sinf(fRadianY);
		float const CY = cosf(fRadianY);
		float const SZ = sinf(fRadianZ);
		float const CZ = cosf(fRadianZ);

		return Matrix3(
			CY * CZ + SY * SX * SZ, -CY * SZ + SY * SX * CZ, SY * CX
			, CX * SZ, CX * CZ, -SX
			, -SY * CZ + CY * SX * SZ, SY * SZ + CY * SX * CZ, CY * CX);
	}


	FORCEINLINE static Matrix3 MakeRotate(Vector axis, float fRadian) 
	{ 
		float const fCos = cosf(fRadian);
		float const fSin = sinf(fRadian);
		float const fOneMinusCos = 1.0f - fCos;

		axis.SetNormalize();
		return Matrix3(fOneMinusCos * axis.x * axis.x + fCos,
			fOneMinusCos * axis.x * axis.y - fSin * axis.z,
			fOneMinusCos * axis.x * axis.z + fSin * axis.y,
			fOneMinusCos * axis.y * axis.x + fSin * axis.z,
			fOneMinusCos * axis.y * axis.y + fCos,
			fOneMinusCos * axis.y * axis.z - fSin * axis.x,
			fOneMinusCos * axis.z * axis.x - fSin * axis.y,
			fOneMinusCos * axis.z * axis.y + fSin * axis.x,
			fOneMinusCos * axis.z * axis.z + fCos
			);
	}

	//////////////////////////////////////////////////////////////////////////

	// Scale
	FORCEINLINE Vector GetScaleVector() const
	{
		return Vector(
			sqrtf(m00 * m00 + m01 * m01 + m02 * m02)
			, sqrtf(m10 * m10 + m11 * m11 + m12 * m12)
			, sqrtf(m20 * m20 + m21 * m21 + m22 * m22)
		);
	}

	FORCEINLINE Matrix3& Scale(float fValue)
	{
		m00 *= fValue; m11 *= fValue; m22 *= fValue;
		return *this;
	}

	FORCEINLINE Matrix3& Scale(float fX, float fY, float fZ)
	{
		m00 *= fX; m11 *= fY; m22 *= fZ;
		return *this;
	}

	FORCEINLINE Matrix3 GetScale(float fValue) const
	{
		return Matrix3(*this).Scale(fValue);
	}

	FORCEINLINE Matrix3 GetScale(float fX, float fY, float fZ) const
	{
		return Matrix3(*this).Scale(fX, fY, fZ);
	}

	FORCEINLINE Matrix3 GetScale(Vector const& vector) const
	{
		return GetScale(vector.x, vector.y, vector.z);
	}

	FORCEINLINE Matrix3 GetMatrixWithoutScale() const
	{
		auto const scaleInv = 1.0f / GetScaleVector();
		return Matrix3(
			m00 * scaleInv.x, m01 * scaleInv.x, m02 * scaleInv.x,
			m10 * scaleInv.y, m11 * scaleInv.y, m12 * scaleInv.y,
			m20 * scaleInv.z, m21 * scaleInv.z, m22 * scaleInv.z
		);
	}

	FORCEINLINE static Matrix3 MakeScale(Vector const& vector)
	{
		return Matrix3(
			vector.x,	0.0f,		0.0f,
			0.0f,		vector.y,	0.0f,
			0.0f,		0.0f,		vector.z
		);
	}

	FORCEINLINE static Matrix3 MakeScale(float fX, float fY, float fZ)
	{
		return Matrix3(
			fX,		0.0f,	0.0f,
			0.0f,	fY,		0.0f,
			0.0f,	0.0f,	fZ
		);
	}
	//////////////////////////////////////////////////////////////////////////

	FORCEINLINE Vector GetCol(unsigned int iIndex) const
	{
		JASSERT(iIndex >= 0 && iIndex < 3);
		switch (iIndex)
		{
		case 0:	return Vector(m00, m10, m20);
		case 1:	return Vector(m01, m11, m21);
		case 2:	return Vector(m02, m12, m22);
		}

		return Vector(ZeroType);
	}

	FORCEINLINE Vector const& GetRow(unsigned int iIndex) const
	{
		JASSERT(iIndex >= 0 && iIndex < 3);
		return mRow[iIndex];
	}

	FORCEINLINE void SetColumn(uint32 iIndex, Vector const& data)
	{
		JASSERT(iIndex >= 0 && iIndex < 3);
		switch (iIndex)
		{
		case 0: m00 = data.x; m10 = data.y; m20 = data.z;		break;
		case 1: m01 = data.x; m11 = data.y; m21 = data.z;		break;
		case 2: m02 = data.x; m12 = data.y; m22 = data.z;		break;
		}
	}

	FORCEINLINE void SetRow(uint32 iIndex, Vector const& data)
	{
		JASSERT(iIndex >= 0 && iIndex < 3);
		mRow[iIndex] = data;
	}

	FORCEINLINE void SetXBasis(Vector const& xAxis) { SetRow(0, xAxis); }
	FORCEINLINE void SetYBasis(Vector const& yAxis) { SetRow(1, yAxis); }
	FORCEINLINE void SetZBasis(Vector const& zAxis) { SetRow(2, zAxis); }

	union
	{
		struct
		{
			float m00, m01, m02;
			float m10, m11, m12;
			float m20, m21, m22;
		};
		float m[3][3];
		float mm[9];
		Vector mRow[3];
	};
};

FORCEINLINE Matrix::Matrix(Matrix3 const& matrix)
	: m00(matrix.m00),	m01(matrix.m01),	m02(matrix.m02),	m03(0.0f)
	, m10(matrix.m10),	m11(matrix.m11),	m12(matrix.m12),	m13(0.0f)
	, m20(matrix.m20),	m21(matrix.m21),	m22(matrix.m22),	m23(0.0f)
	, m30(0.0f),		m31(0.0f),			m32(0.0f),			m33(1.0f)
{

}

FORCEINLINE void Matrix::operator=(struct Matrix3 const& matrix)
{
	m00 = matrix.m00;	m01 = matrix.m01;	m02 = matrix.m02;	m03 = 0.0f;
	m10 = matrix.m10;	m11 = matrix.m11;	m12 = matrix.m12;	m13 = 0.0f;
	m20 = matrix.m20;	m21 = matrix.m21;	m22 = matrix.m22;	m23 = 0.0f;
	m30 = 0.0f;			m31 = 0.0f;			m32 = 0.0f;			m33 = 0.0f;
}

//FORCEINLINE Vector4 operator*(Vector4 const& vector, Matrix const& matrix)
//{
//	return matrix.Transform(vector);
//}
//
//FORCEINLINE Vector4& operator*=(Vector4& vector, Matrix const& matrix)
//{
//	vector = matrix.Transform(vector);
//	return vector;
//}
//
//FORCEINLINE Vector operator*(Vector const& vector, Matrix3 const& matrix)
//{
//	return matrix.Transform(vector);
//}
//
//FORCEINLINE Vector& operator*=(Vector& vector, Matrix3 const& matrix)
//{
//	vector = matrix.Transform(vector);
//	return vector;
//}



