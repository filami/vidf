#pragma once

namespace vidf
{


	template<typename T>
	struct Vector4
	{
		Vector4() {}

		Vector4(const Vector4<T>& v)
			:	x(v.x), y(v.y), z(v.z), w(v.w) {}

		explicit Vector4(Zero)
			:	x(0), y(0), z(0), w(0) {}

		template<typename U>
		explicit Vector4(U _x, U _y, U _z, T _w)
			:	x((T)_x), y((T)_y), z((T)_z), w((T)_w) {}

		template<typename U>
		explicit Vector4(const Vector4<U>& v)
			:	x((T)v.x), y((T)v.y), z((T)v.z), w((T)v.w) {}

		T& operator[] (int idx) {return (&x)[idx];}
		T operator[] (int idx) const {return (&x)[idx];}

		T x, y, z, w;
	};



	template<typename T>
	inline Vector4<T> operator + (const Vector4<T>& lhv, const Vector4<T>& rhv)
	{
		return Vector4<T>(lhv.x+rhv.x, lhv.y+rhv.y, lhv.z+rhv.z, lhv.w+rhv.w);
	}


	template<typename T>
	Vector4<T> operator - (const Vector4<T>& rhv)
	{
		return Vector4<T>(-rhv.x, -rhv.y, -rhv.z, -rhv.w);
	}


	template<typename T>
	inline Vector4<T> operator - (const Vector4<T>& lhv, const Vector4<T>& rhv)
	{
		return Vector4<T>(lhv.x-rhv.x, lhv.y-rhv.y, lhv.z-rhv.z, lhv.w-rhv.w);
	}



	template<typename T>
	inline Vector4<T> operator * (const Vector4<T>& lhv, const Vector4<T>& rhv)
	{
		return Vector4<T>(lhv.x*rhv.x, lhv.y*rhv.y, lhv.z*rhv.z, lhv.w*rhv.w);
	}
	template<typename T, typename U>
	inline Vector4<T> operator * (const Vector4<T>& lhv, U rhv)
	{
		return Vector4<T>(lhv.x*(T)rhv, lhv.y*(T)rhv, lhv.z*(T)rhv, lhv.w*(T)rhv);
	}
	template<typename T, typename U>
	inline Vector4<T> operator * (U rhv, const Vector4<T>& lhv)
	{
		return lhv * (T)rhv;
	}



	template<typename T>
	inline Vector4<T> operator / (const Vector4<T>& lhv, const Vector4<T>& rhv)
	{
		return Vector4<T>(lhv.x/rhv.x, lhv.y/rhv.y, lhv.z/rhv.z, lhv.w/rhv.w);
	}
	template<typename T, typename U>
	inline Vector4<T> operator / (const Vector4<T>& lhv, U rhv)
	{
		return Vector4<T>(lhv.x/(T)rhv, lhv.y/(T)rhv, lhv.z/(T)rhv, lhv.w/(T)rhv);
	}
	template<typename T, typename U>
	inline Vector4<T> operator / (U rhv, const Vector4<T>& lhv)
	{
		return Vector4<T>((T)lhv/rhv.x, (T)lhv/rhv.y, (T)lhv/rhv.z, (T)lhv/rhv.w);
		return lhv * (T)rhv;
	}



	template<typename T>
	inline bool operator == (const Vector4<T>& lhv, const Vector4<T>& rhv)
	{
		return lhv.x == rhv.x && lhv.y == rhv.y && lhv.z == rhv.z && lhv.w == rhv.w;
	}

	template<typename T>
	inline bool operator != (const Vector4<T>& lhv, const Vector4<T>& rhv)
	{
		return lhv.x != rhv.x || lhv.y != rhv.y || lhv.z != rhv.z || lhv.w != rhv.w;
	}



	template<typename T>
	inline Vector4<T> Normalize(const Vector4<T>& v)
	{
		float m = 1.0f / std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
		return Vector4<T>(
			v.x * m,
			v.y * m,
			v.z * m,
			v.w * m);
	}



	template<typename T>
	inline Vector4<T> SafeNormalize(const Vector4<T>& v)
	{
		float m = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
		if (m == 0)
			return Vector4<T>(zero);
		m = 1.0f / m;
		return Vector4<T>(
			v.x * m,
			v.y * m,
			v.z * m,
			v.w * m);
	}



	template<typename T>
	inline Vector4<T> Clamp(const Vector4<T>& v, T max)
	{
		float m = std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z + v.w*v.w);
		if (m <= max)
			return v;
		m = (1.0f / m) * max;
		return Vector4<T>(
			v.x * m,
			v.y * m,
			v.z * m,
			v.z * m);
	}



	template<typename T>
	inline T Dot(const Vector4<T>& lhv, const Vector4<T>& rhv)
	{
		return lhv.x*rhv.x + lhv.y*rhv.y + lhv.z*rhv.z + lhv.w*rhv.w;
	}



	template<typename T>
	inline T Length(const Vector4<T>& lhv)
	{
		return std::sqrt(Dot(lhv, lhv));
	}



	template<typename T>
	inline T Distance(const Vector4<T>& lhv, const Vector4<T>& rhv)
	{
		return std::sqrt(Dot(lhv-rhv, lhv-rhv));
	}



	template<typename T>
	inline Vector4<T> Min(const Vector4<T>& lhv, const Vector4<T>& rhv)
	{
		return Vector4<T>(
			Min(lhv.x, rhv.x),
			Min(lhv.y, rhv.y),
			Min(lhv.z, rhv.z),
			Min(lhv.w, rhv.w));
	}



	template<typename T>
	inline Vector4<T> Max(const Vector4<T>& lhv, const Vector4<T>& rhv)
	{
		return Vector4<T>(
			Max(lhv.x, rhv.x),
			Max(lhv.y, rhv.y),
			Max(lhv.z, rhv.z),
			Max(lhv.w, rhv.w));
	}



	typedef Vector4<float> Vector4f;
	typedef Vector4<int> Vector4i;


}
