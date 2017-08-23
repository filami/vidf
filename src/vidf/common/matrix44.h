#pragma once

#include "vector3.h"
#include "vector4.h"

namespace vidf
{


	template<typename T>
	struct Matrix44
	{
		Matrix44() {}

		explicit Matrix44(Zero)
			:	m00((T)1), m01(0), m02(0), m03(0)
			,	m10(0), m11((T)1), m12(0), m13(0)
			,	m20(0), m21(0), m22((T)1), m23(0)
			,	m30(0), m31(0), m32(0), m33((T)1) {}

		T& operator[] (int idx) {return (&m00)[idx];}
		T operator[] (int idx) const {return (&m00)[idx];}

		T m00, m01, m02, m03;
		T m10, m11, m12, m13;
		T m20, m21, m22, m23;
		T m30, m31, m32, m33;
	};


	template<typename T>
	Matrix44<T> Transpose(const Matrix44<T>& m)
	{
		Matrix44<T> r;
		r.m00=m.m00; r.m01=m.m10; r.m02=m.m20; r.m03=m.m30;
		r.m10=m.m01; r.m11=m.m11; r.m12=m.m21; r.m13=m.m31;
		r.m20=m.m02; r.m21=m.m12; r.m22=m.m22; r.m23=m.m32;
		r.m30=m.m03; r.m31=m.m13; r.m32=m.m23; r.m33=m.m33;
		return r;
	}



	template<typename T>
	Matrix44<T> Mul(const Matrix44<T>& lhv, const Matrix44<T>& rhv)
	{
		Matrix44<T> r;

		r.m00 = lhv.m00*rhv.m00 + lhv.m01*rhv.m10 + lhv.m02*rhv.m20 + lhv.m03*rhv.m30;
		r.m01 = lhv.m00*rhv.m01 + lhv.m01*rhv.m11 + lhv.m02*rhv.m21 + lhv.m03*rhv.m31;
		r.m02 = lhv.m00*rhv.m02 + lhv.m01*rhv.m12 + lhv.m02*rhv.m22 + lhv.m03*rhv.m32;
		r.m03 = lhv.m00*rhv.m03 + lhv.m01*rhv.m13 + lhv.m02*rhv.m23 + lhv.m03*rhv.m33;

		r.m10 = lhv.m10*rhv.m00 + lhv.m11*rhv.m10 + lhv.m12*rhv.m20 + lhv.m13*rhv.m30;
		r.m11 = lhv.m10*rhv.m01 + lhv.m11*rhv.m11 + lhv.m12*rhv.m21 + lhv.m13*rhv.m31;
		r.m12 = lhv.m10*rhv.m02 + lhv.m11*rhv.m12 + lhv.m12*rhv.m22 + lhv.m13*rhv.m32;
		r.m13 = lhv.m10*rhv.m03 + lhv.m11*rhv.m13 + lhv.m12*rhv.m23 + lhv.m13*rhv.m33;

		r.m20 = lhv.m20*rhv.m00 + lhv.m21*rhv.m10 + lhv.m22*rhv.m20 + lhv.m23*rhv.m30;
		r.m21 = lhv.m20*rhv.m01 + lhv.m21*rhv.m11 + lhv.m22*rhv.m21 + lhv.m23*rhv.m31;
		r.m22 = lhv.m20*rhv.m02 + lhv.m21*rhv.m12 + lhv.m22*rhv.m22 + lhv.m23*rhv.m32;
		r.m23 = lhv.m20*rhv.m03 + lhv.m21*rhv.m13 + lhv.m22*rhv.m23 + lhv.m23*rhv.m33;

		r.m30 = lhv.m30*rhv.m00 + lhv.m31*rhv.m10 + lhv.m32*rhv.m20 + lhv.m33*rhv.m30;
		r.m31 = lhv.m30*rhv.m01 + lhv.m31*rhv.m11 + lhv.m32*rhv.m21 + lhv.m33*rhv.m31;
		r.m32 = lhv.m30*rhv.m02 + lhv.m31*rhv.m12 + lhv.m32*rhv.m22 + lhv.m33*rhv.m32;
		r.m33 = lhv.m30*rhv.m03 + lhv.m31*rhv.m13 + lhv.m32*rhv.m23 + lhv.m33*rhv.m33;

		return r;
	}



	template<typename T>
	Vector3<T> Mul(const Matrix44<T>& lhv, const Vector3<T>& rhv)
	{
		Vector3<T> r;
		r.x = lhv.m00*rhv.x + lhv.m01*rhv.y + lhv.m02*rhv.z + lhv.m03;
		r.y = lhv.m10*rhv.x + lhv.m11*rhv.y + lhv.m12*rhv.z + lhv.m13;
		r.z = lhv.m20*rhv.x + lhv.m21*rhv.y + lhv.m22*rhv.z + lhv.m23;
		return r;
	}

	template<typename T>
	Vector3<T> Mul(const Vector3<T>& lhv, const Matrix44<T>& rhv)
	{
		Vector3<T> r;
		r.x = lhv.x*rhv.m00 + lhv.y*rhv.m10 + lhv.z*rhv.m20 + rhv.m30;
		r.y = lhv.x*rhv.m01 + lhv.y*rhv.m11 + lhv.z*rhv.m21 + rhv.m31;
		r.z = lhv.x*rhv.m02 + lhv.y*rhv.m12 + lhv.z*rhv.m22 + rhv.m32;
		return r;
	}



	template<typename T>
	Vector4<T> Mul(const Matrix44<T>& lhv, const Vector4<T>& rhv)
	{
		Vector4<T> r;
		r.x = lhv.m00*rhv.x + lhv.m01*rhv.y + lhv.m02*rhv.z + lhv.m03*rhv.w;
		r.y = lhv.m10*rhv.x + lhv.m11*rhv.y + lhv.m12*rhv.z + lhv.m13*rhv.w;
		r.z = lhv.m20*rhv.x + lhv.m21*rhv.y + lhv.m22*rhv.z + lhv.m23*rhv.w;
		r.w = lhv.m30*rhv.x + lhv.m31*rhv.y + lhv.m32*rhv.z + lhv.m33*rhv.w;
		return r;
	}

	template<typename T>
	Vector4<T> Mul(const Vector4<T>& lhv, const Matrix44<T>& rhv)
	{
		Vector4<T> r;
		r.x = lhv.x*rhv.m00 + lhv.y*rhv.m10 + lhv.z*rhv.m20 + lhv.w*rhv.m30;
		r.y = lhv.x*rhv.m01 + lhv.y*rhv.m11 + lhv.z*rhv.m21 + lhv.w*rhv.m31;
		r.z = lhv.x*rhv.m02 + lhv.y*rhv.m12 + lhv.z*rhv.m22 + lhv.w*rhv.m32;
		r.w = lhv.x*rhv.m03 + lhv.y*rhv.m13 + lhv.z*rhv.m23 + lhv.w*rhv.m33;
		return r;
	}


	template<typename T>
	Matrix44<T> Inverse(const Matrix44<T>& m)
	{
		/*
		T inv[16];
		T det;

		inv[0] =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15]
		+ m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
		inv[4] =  -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15]
		- m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
		inv[8] =   m[4]*m[9]*m[15] - m[4]*m[11]*m[13] - m[8]*m[5]*m[15]
		+ m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
		inv[12] = -m[4]*m[9]*m[14] + m[4]*m[10]*m[13] + m[8]*m[5]*m[14]
		- m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
		inv[1] =  -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15]
		- m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
		inv[5] =   m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15]
		+ m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
		inv[9] =  -m[0]*m[9]*m[15] + m[0]*m[11]*m[13] + m[8]*m[1]*m[15]
		- m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
		inv[13] =  m[0]*m[9]*m[14] - m[0]*m[10]*m[13] - m[8]*m[1]*m[14]
		+ m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
		inv[2] =   m[1]*m[6]*m[15] - m[1]*m[7]*m[14] - m[5]*m[2]*m[15]
		+ m[5]*m[3]*m[14] + m[13]*m[2]*m[7] - m[13]*m[3]*m[6];
		inv[6] =  -m[0]*m[6]*m[15] + m[0]*m[7]*m[14] + m[4]*m[2]*m[15]
		- m[4]*m[3]*m[14] - m[12]*m[2]*m[7] + m[12]*m[3]*m[6];
		inv[10] =  m[0]*m[5]*m[15] - m[0]*m[7]*m[13] - m[4]*m[1]*m[15]
		+ m[4]*m[3]*m[13] + m[12]*m[1]*m[7] - m[12]*m[3]*m[5];
		inv[14] = -m[0]*m[5]*m[14] + m[0]*m[6]*m[13] + m[4]*m[1]*m[14]
		- m[4]*m[2]*m[13] - m[12]*m[1]*m[6] + m[12]*m[2]*m[5];
		inv[3] =  -m[1]*m[6]*m[11] + m[1]*m[7]*m[10] + m[5]*m[2]*m[11]
		- m[5]*m[3]*m[10] - m[9]*m[2]*m[7] + m[9]*m[3]*m[6];
		inv[7] =   m[0]*m[6]*m[11] - m[0]*m[7]*m[10] - m[4]*m[2]*m[11]
		+ m[4]*m[3]*m[10] + m[8]*m[2]*m[7] - m[8]*m[3]*m[6];
		inv[11] = -m[0]*m[5]*m[11] + m[0]*m[7]*m[9] + m[4]*m[1]*m[11]
		- m[4]*m[3]*m[9] - m[8]*m[1]*m[7] + m[8]*m[3]*m[5];
		inv[15] =  m[0]*m[5]*m[10] - m[0]*m[6]*m[9] - m[4]*m[1]*m[10]
		+ m[4]*m[2]*m[9] + m[8]*m[1]*m[6] - m[8]*m[2]*m[5];

		det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];
		if (det == 0)
			return Matrix44<T>(zero);

		det = (T)1 / det;

		Matrix44<T> result;
		for (int i = 0; i < 16; i++)
			result[i] = inv[i] * det;

		return result;
		*/

		T inv[16];
		int i;

		inv[0] = m[5] * m[10] * m[15] -
			m[5] * m[11] * m[14] -
			m[9] * m[6] * m[15] +
			m[9] * m[7] * m[14] +
			m[13] * m[6] * m[11] -
			m[13] * m[7] * m[10];

		inv[4] = -m[4] * m[10] * m[15] +
			m[4] * m[11] * m[14] +
			m[8] * m[6] * m[15] -
			m[8] * m[7] * m[14] -
			m[12] * m[6] * m[11] +
			m[12] * m[7] * m[10];

		inv[8] = m[4] * m[9] * m[15] -
			m[4] * m[11] * m[13] -
			m[8] * m[5] * m[15] +
			m[8] * m[7] * m[13] +
			m[12] * m[5] * m[11] -
			m[12] * m[7] * m[9];

		inv[12] = -m[4] * m[9] * m[14] +
			m[4] * m[10] * m[13] +
			m[8] * m[5] * m[14] -
			m[8] * m[6] * m[13] -
			m[12] * m[5] * m[10] +
			m[12] * m[6] * m[9];

		inv[1] = -m[1] * m[10] * m[15] +
			m[1] * m[11] * m[14] +
			m[9] * m[2] * m[15] -
			m[9] * m[3] * m[14] -
			m[13] * m[2] * m[11] +
			m[13] * m[3] * m[10];

		inv[5] = m[0] * m[10] * m[15] -
			m[0] * m[11] * m[14] -
			m[8] * m[2] * m[15] +
			m[8] * m[3] * m[14] +
			m[12] * m[2] * m[11] -
			m[12] * m[3] * m[10];

		inv[9] = -m[0] * m[9] * m[15] +
			m[0] * m[11] * m[13] +
			m[8] * m[1] * m[15] -
			m[8] * m[3] * m[13] -
			m[12] * m[1] * m[11] +
			m[12] * m[3] * m[9];

		inv[13] = m[0] * m[9] * m[14] -
			m[0] * m[10] * m[13] -
			m[8] * m[1] * m[14] +
			m[8] * m[2] * m[13] +
			m[12] * m[1] * m[10] -
			m[12] * m[2] * m[9];

		inv[2] = m[1] * m[6] * m[15] -
			m[1] * m[7] * m[14] -
			m[5] * m[2] * m[15] +
			m[5] * m[3] * m[14] +
			m[13] * m[2] * m[7] -
			m[13] * m[3] * m[6];

		inv[6] = -m[0] * m[6] * m[15] +
			m[0] * m[7] * m[14] +
			m[4] * m[2] * m[15] -
			m[4] * m[3] * m[14] -
			m[12] * m[2] * m[7] +
			m[12] * m[3] * m[6];

		inv[10] = m[0] * m[5] * m[15] -
			m[0] * m[7] * m[13] -
			m[4] * m[1] * m[15] +
			m[4] * m[3] * m[13] +
			m[12] * m[1] * m[7] -
			m[12] * m[3] * m[5];

		inv[14] = -m[0] * m[5] * m[14] +
			m[0] * m[6] * m[13] +
			m[4] * m[1] * m[14] -
			m[4] * m[2] * m[13] -
			m[12] * m[1] * m[6] +
			m[12] * m[2] * m[5];

		inv[3] = -m[1] * m[6] * m[11] +
			m[1] * m[7] * m[10] +
			m[5] * m[2] * m[11] -
			m[5] * m[3] * m[10] -
			m[9] * m[2] * m[7] +
			m[9] * m[3] * m[6];

		inv[7] = m[0] * m[6] * m[11] -
			m[0] * m[7] * m[10] -
			m[4] * m[2] * m[11] +
			m[4] * m[3] * m[10] +
			m[8] * m[2] * m[7] -
			m[8] * m[3] * m[6];

		inv[11] = -m[0] * m[5] * m[11] +
			m[0] * m[7] * m[9] +
			m[4] * m[1] * m[11] -
			m[4] * m[3] * m[9] -
			m[8] * m[1] * m[7] +
			m[8] * m[3] * m[5];

		inv[15] = m[0] * m[5] * m[10] -
			m[0] * m[6] * m[9] -
			m[4] * m[1] * m[10] +
			m[4] * m[2] * m[9] +
			m[8] * m[1] * m[6] -
			m[8] * m[2] * m[5];

		T det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];
		/*
		if (det == 0)
			return false;*/

		det = T(1) / det;

		Matrix44<T> invOut;
		for (i = 0; i < 16; i++)
			invOut[i] = inv[i] * det;

		return invOut;
	}



	typedef Matrix44<float> Matrix44f;

}
