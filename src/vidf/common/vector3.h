#pragma once

#include "utils.h"


namespace vidf
{


	template<typename T>
	struct Vector3
	{
		Vector3() {}

		Vector3(const Vector3<T>& v)
			:	x(v.x), y(v.y), z(v.z) {}

		explicit Vector3(Zero)
			:	x(0), y(0), z(0) {}

		template<typename U>
		explicit Vector3(U _x, U _y, U _z)
			:	x((T)_x), y((T)_y), z((T)_z)
		{
		/*	assert(IsNumber(_x));
			assert(IsNumber(_y));
			assert(IsNumber(_z));*/
		}

		template<typename U>
		explicit Vector3(const Vector3<U>& v)
			:	x((T)v.x), y((T)v.y), z((T)v.z) {}

		T& operator[] (int idx) {return (&x)[idx];}
		T operator[] (int idx) const {return (&x)[idx];}

		T x, y, z;
	};



	template<typename T>
	inline Vector3<T> operator + (const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return Vector3<T>(lhv.x+rhv.x, lhv.y+rhv.y, lhv.z+rhv.z);
	}


	template<typename T>
	Vector3<T> operator - (const Vector3<T>& rhv)
	{
		return Vector3<T>(-rhv.x, -rhv.y, -rhv.z);
	}


	template<typename T>
	inline Vector3<T> operator - (const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return Vector3<T>(lhv.x-rhv.x, lhv.y-rhv.y, lhv.z-rhv.z);
	}



	template<typename T>
	inline Vector3<T> operator * (const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return Vector3<T>(lhv.x*rhv.x, lhv.y*rhv.y, lhv.z*rhv.z);
	}
	template<typename T, typename U>
	inline Vector3<T> operator * (const Vector3<T>& lhv, U rhv)
	{
		return Vector3<T>(lhv.x*(T)rhv, lhv.y*(T)rhv, lhv.z*(T)rhv);
	}
	template<typename T, typename U>
	inline Vector3<T> operator * (U rhv, const Vector3<T>& lhv)
	{
		return lhv * (T)rhv;
	}



	template<typename T>
	inline Vector3<T> operator / (const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return Vector3<T>(lhv.x/rhv.x, lhv.y/rhv.y, lhv.z/rhv.z);
	}
	template<typename T, typename U>
	inline Vector3<T> operator / (const Vector3<T>& lhv, U rhv)
	{
		return Vector3<T>(lhv.x/(T)rhv, lhv.y/(T)rhv, lhv.z/(T)rhv);
	}
	template<typename T, typename U>
	inline Vector3<T> operator / (U lhv, const Vector3<T>& rhv)
	{
		return Vector3<T>((T)lhv/rhv.x, (T)lhv/rhv.y, (T)lhv/rhv.z);
	}



	template<typename T>
	inline bool operator == (const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return lhv.x == rhv.x && lhv.y == rhv.y && lhv.z == rhv.z;
	}

	template<typename T>
	inline bool operator != (const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return lhv.x != rhv.x || lhv.y != rhv.y || lhv.z != rhv.z;
	}



	template<typename T>
	inline Vector3<T> Normalize(const Vector3<T>& v)
	{
		float m = 1.0f / std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
		return Vector3<T>(
			v.x * m,
			v.y * m,
			v.z * m);
	}



	template<typename T>
	inline Vector3<T> SafeNormalize(const Vector3<T>& v, const Vector3<T>& defaultValue=Vector3f(zero))
	{
		float m = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
		if (m == 0)
			return defaultValue;
		m = 1.0f / m;
		return Vector3<T>(
			v.x * m,
			v.y * m,
			v.z * m);
	}



	template<typename T>
	inline Vector3<T> Clamp(const Vector3<T>& v, T max)
	{
		float m = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
		if (m <= max)
			return v;
		m = (1.0f / m) * max;
		return Vector3<T>(
			v.x * m,
			v.y * m,
			v.z * m);
	}



	template<typename T>
	inline Vector3<T> Cross(const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return Vector3<T>(
				lhv.y*rhv.z - lhv.z*rhv.y,
				lhv.z*rhv.x - lhv.x*rhv.z,
				lhv.x*rhv.y - lhv.y*rhv.x);
	}



	template<typename T>
	inline T Dot(const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return lhv.x*rhv.x + lhv.y*rhv.y + lhv.z*rhv.z;
	}



	template<typename T>
	inline T Length(const Vector3<T>& lhv)
	{
		return std::sqrt(Dot(lhv, lhv));
	}



	template<typename T>
	inline T LengthSquare(const Vector3<T>& lhv)
	{
		return Dot(lhv, lhv);
	}



	template<typename T>
	inline T Distance(const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return std::sqrt(Dot(lhv-rhv, lhv-rhv));
	}



	template<typename T>
	inline Vector3<T> Min(const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return Vector3<T>(
			Min(lhv.x, rhv.x),
			Min(lhv.y, rhv.y),
			Min(lhv.z, rhv.z));
	}



	template<typename T>
	inline Vector3<T> Max(const Vector3<T>& lhv, const Vector3<T>& rhv)
	{
		return Vector3<T>(
			Max(lhv.x, rhv.x),
			Max(lhv.y, rhv.y),
			Max(lhv.z, rhv.z));
	}



	template<typename T>
	inline std::ostream& operator << (std::ostream& lhv, const Vector3<T>& rhv)
	{
		lhv << "[" << rhv.x << ", " << rhv.y << ", " << rhv.z << "]";
		return lhv;
	}



	typedef Vector3<float> Vector3f;
	typedef Vector3<int> Vector3i;


}
