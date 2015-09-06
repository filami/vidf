#pragma once

#include "vector3.h"

namespace vidf
{


	template<typename T>
	struct Matrix33
	{
		Matrix33() {}

		explicit Matrix33(Zero)
			:	m00((T)1), m01(0), m02(0)
			,	m10(0), m11((T)1), m12(0)
			,	m20(0), m21(0), m22((T)1) {}

		T& operator[] (int idx) {return (&m00)[idx];}
		T operator[] (int idx) const {return (&m00)[idx];}

		T m00, m01, m02;
		T m10, m11, m12;
		T m20, m21, m22;
	};


	template<typename T>
	Matrix33<T> Transpose(const Matrix33<T>& m)
	{
		Matrix33<T> r;
		r.m00=m.m00; r.m01=m.m10; r.m02=m.m20;
		r.m10=m.m01; r.m11=m.m11; r.m12=m.m21;
		r.m20=m.m02; r.m21=m.m12; r.m22=m.m22;
		return r;
	}



	template<typename T>
	Matrix33<T> Mul(const Matrix33<T>& lhv, const Matrix33<T>& rhv)
	{
		Matrix44<T> r;

		r.m00 = lhv.m00*rhv.m00 + lhv.m01*rhv.m10 + lhv.m02*rhv.m20;
		r.m01 = lhv.m00*rhv.m01 + lhv.m01*rhv.m11 + lhv.m02*rhv.m21;
		r.m02 = lhv.m00*rhv.m02 + lhv.m01*rhv.m12 + lhv.m02*rhv.m22;
		r.m03 = lhv.m00*rhv.m03 + lhv.m01*rhv.m13 + lhv.m02*rhv.m23;

		r.m10 = lhv.m10*rhv.m00 + lhv.m11*rhv.m10 + lhv.m12*rhv.m20;
		r.m11 = lhv.m10*rhv.m01 + lhv.m11*rhv.m11 + lhv.m12*rhv.m21;
		r.m12 = lhv.m10*rhv.m02 + lhv.m11*rhv.m12 + lhv.m12*rhv.m22;
		r.m13 = lhv.m10*rhv.m03 + lhv.m11*rhv.m13 + lhv.m12*rhv.m23;

		r.m20 = lhv.m20*rhv.m00 + lhv.m21*rhv.m10 + lhv.m22*rhv.m20;
		r.m21 = lhv.m20*rhv.m01 + lhv.m21*rhv.m11 + lhv.m22*rhv.m21;
		r.m22 = lhv.m20*rhv.m02 + lhv.m21*rhv.m12 + lhv.m22*rhv.m22;
		r.m23 = lhv.m20*rhv.m03 + lhv.m21*rhv.m13 + lhv.m22*rhv.m23;

		return r;
	}



	template<typename T>
	Vector3<T> Mul(const Matrix33<T>& lhv, const Vector3<T>& rhv)
	{
		Vector3<T> r;
		r.x = lhv.m00*rhv.x + lhv.m01*rhv.y + lhv.m02*rhv.z;
		r.y = lhv.m10*rhv.x + lhv.m11*rhv.y + lhv.m12*rhv.z;
		r.z = lhv.m20*rhv.x + lhv.m21*rhv.y + lhv.m22*rhv.z;
		return r;
	}

	template<typename T>
	Vector3<T> Mul(const Vector3<T>& lhv, const Matrix33<T>& rhv)
	{
		Vector3<T> r;
		r.x = lhv.x*rhv.m00 + lhv.y*rhv.m10 + lhv.z*rhv.m20;
		r.y = lhv.x*rhv.m01 + lhv.y*rhv.m11 + lhv.z*rhv.m21;
		r.z = lhv.x*rhv.m02 + lhv.y*rhv.m12 + lhv.z*rhv.m22;
		return r;
	}

	template<typename T>
	T Determinant(const Matrix33<T>& m)
	{
		T result =
			+m.m00*(m.m11*m.m22-m.m21*m.m12)
            -m.m01*(m.m10*m.m22-m.m12*m.m20)
			+m.m02*(m.m10*m.m21-m.m11*m.m20);
		return result;
	}



	typedef Matrix33<float> Matrix33f;

}
