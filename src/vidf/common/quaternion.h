#pragma once

#include "matrix44.h"

namespace vidf
{


	template<typename T>
	struct Quaternion
	{
		Quaternion() {}

		Quaternion(const Quaternion& q)
			: x(q.x), y(q.y), z(q.z), w(q.w) {}

		Quaternion(Zero)
			: x(0), y(0), z(0), w((T)1) {}

		template<typename U>
		explicit Quaternion(U _x, U _y, U _z, U _w)
			: x((T)_x), y((T)_y), z((T)_z), w((T)_w) {}

		template<typename U>
		explicit Quaternion(const Quaternion<U>& q)
			: x((T)q.x), y((T)q.y), z((T)q.z), w((T)q.w) {}

		template<typename U>
		explicit Quaternion(const Vector3<U>& v, U _w=0)
			: x((T)v.x), y((T)v.y), z((T)v.z), w(_w) {}

		float& operator[] (int idx) {return (&x)[idx];}
		float operator[] (int idx) const {return (&x)[idx];}

		T x, y, z, w;
	};



	template<typename T>
	inline Quaternion<T> operator + (const Quaternion<T>& lhv, const Quaternion<T>& rhv)
	{
		return Quaternion<T>(lhv.x+rhv.x, lhv.y+rhv.y, lhv.z+rhv.z, lhv.w+rhv.w);
	}



	template<typename T>
	Quaternion<T> operator - (const Quaternion<T>& rhv)
	{
		return Quaternion<T>(-rhv.x, -rhv.y, -rhv.z, -rhv.w);
	}


	template<typename T>
	inline Quaternion<T> operator - (const Quaternion<T>& lhv, const Quaternion<T>& rhv)
	{
		return Quaternion<T>(lhv.x-rhv.x, lhv.y-rhv.y, lhv.z-rhv.z, lhv.w-rhv.w);
	}



	template<typename T, typename U>
	inline Quaternion<T> operator * (const Quaternion<T>& lhv, U rhv)
	{
		return Quaternion<T>(lhv.x*(T)rhv, lhv.y*(T)rhv, lhv.z*(T)rhv, lhv.w*(T)rhv);
	}



	template<typename T>
	inline Quaternion<T> operator * (const Quaternion<T>& lhv, const Quaternion<T>& rhv)
	{
		T e = (lhv.x + lhv.z)*(rhv.x + rhv.y);
		T f = (lhv.z - lhv.x)*(rhv.x - rhv.y);
		T g = (lhv.w + lhv.y)*(rhv.w - rhv.z);
		T h = (lhv.w - lhv.y)*(rhv.w + rhv.z);
		T a = f - e;
		T b = f + e;
		return Quaternion<T>(
			(lhv.w + lhv.x)*(rhv.w + rhv.x) + (a - g - h) * (T)0.5,
			(lhv.w - lhv.x)*(rhv.y + rhv.z) + (b + g - h) * (T)0.5,
			(lhv.y + lhv.z)*(rhv.w - rhv.x) + (b - g + h) * (T)0.5,
			(lhv.z - lhv.y)*(rhv.y - rhv.z) + (a + g + h) * (T)0.5);

	}



	template<typename T>
	inline T Dot(const Quaternion<T>& lhv, const Quaternion<T>& rhv)
	{
		return lhv.x*rhv.x + lhv.y*rhv.y + lhv.z*rhv.z + lhv.w*rhv.w;
	}



	template<typename T>
	inline Quaternion<T> Normalize(const Quaternion<T>& v)
	{
		float m = 1.0f / std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
		return Quaternion<T>(
			v.x * m,
			v.y * m,
			v.z * m,
			v.w * m);
	}



	template<typename T>
	inline Quaternion<T> Conjugate(const Quaternion<T>& quat)
	{
		return Quaternion<T>(-quat.x, -quat.y, -quat.z, quat.w);
	}



	template<typename T>
	inline Vector3<T> Rotate(const Quaternion<T>& quat, const Vector3<T>& vec)
	{
		Quaternion<T> result = quat * Quaternion<T>(vec) * Conjugate(quat);
		return Vector3<T>(result.x, result.y, result.z);
	}



	template<typename T>
	inline Quaternion<T> QuaternionAxisAngle(const Vector3<T>& axis, T angle)
	{
		T d = angle * (T)0.5;
		return Quaternion<T>(axis * std::sin(d), std::cos(d));
	}



	template<typename T>
	inline Quaternion<T> QuaternionVectorDirection(const Vector3<T>& forward, const Vector3<T>& direction)
	{
		return QuaternionAxisAngle(
			Normalize(Cross(forward, direction)),
			std::acosf(Dot(forward, direction)));
	}



	template<typename T>
	inline Quaternion<T> Nlerp(const Quaternion<T>& from, const Quaternion<T>& to, T stride)
	{
		T sign = Sign(Dot(from, to));
		return Normalize(from*(1.0f-stride) + to*stride*sign);
	}



	template<typename T>
	inline Quaternion<T> Slerp(const Quaternion<T>& from, const Quaternion<T>& to, T stride)
	{
		T cos = from.w*to.w + from.x*to.x + from.y*to.y + from.z*to.z;
		T angle = std::acos(cos);

		if (std::abs(angle) < (T)0.001)
			return from;

		T sin = std::sin(angle);
		T invSin = ((T)1.0)/sin;
		T coeff0 = std::sin(((T)1.0-stride)*angle)*invSin;
		T coeff1 = std::sin(stride*angle)*invSin;
		if(cos < (T)0.0)
			coeff0 = -coeff0;
		Quaternion<T> t(from*coeff0 + to*coeff1);
		t = Normalize(t);
		return t;
	}



	template<typename T>
	inline Quaternion<T> Inverse(const Quaternion<T>& quat)
	{
		T d = (T)1.0 / (quat.x*quat.x + quat.y*quat.y + quat.z*quat.z + quat.w*quat.w);
		Quaternion<T> t(-quat.x*d, -quat.y*d, -quat.z*d, quat.w*d);
		return t;
	}



	template<typename T>
	inline Matrix44<T> ToMatrix44(const Quaternion<T>& quat)
	{
		Matrix44<T> out(zero);

		T fTx  = T(2)*quat.x;
		T fTy  = T(2)*quat.y;
		T fTz  = T(2)*quat.z;
		T fTwx = fTx*quat.w;
		T fTwy = fTy*quat.w;
		T fTwz = fTz*quat.w;
		T fTxx = fTx*quat.x;
		T fTxy = fTy*quat.x;
		T fTxz = fTz*quat.x;
		T fTyy = fTy*quat.y;
		T fTyz = fTz*quat.y;
		T fTzz = fTz*quat.z;
		out.m00 = T(1) - (fTyy + fTzz);
		out.m01 = fTxy - fTwz;
		out.m02 = fTxz + fTwy;
		out.m10 = fTxy + fTwz;		
		out.m11 = T(1) - (fTxx + fTzz);
		out.m12 = fTyz - fTwx;
		out.m20 = fTxz - fTwy;
		out.m21 = fTyz + fTwx;
		out.m22 = T(1) - (fTxx + fTyy);

		return out;
	}


	typedef Quaternion<float> Quaternionf;


}
