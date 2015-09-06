#pragma once

namespace vidf
{



	template<typename T>
	struct Vector2
	{
		Vector2() {}

		explicit Vector2(Zero)
			:	x(0), y(0) {}

		Vector2(const Vector2<T>& v)
			:	x(v.x), y(v.y) {}

		template<typename U>
		explicit Vector2(U _x, U _y)
			:	x((T)_x), y((T)_y) {}

		template<typename U>
		explicit Vector2(const Vector2<U>& v)
			:	x((T)v.x), y((T)v.y) {}

		T& operator[] (int idx) {return (&x)[idx];}
		T operator[] (int idx) const {return (&x)[idx];}

		T x, y;
	};




	template<typename T>
	inline Vector2<T> operator + (const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return Vector2<T>(lhv.x+rhv.x, lhv.y+rhv.y);
	}


	template<typename T>
	Vector2<T> operator - (const Vector2<T>& rhv)
	{
		return Vector2<T>(-rhv.x, -rhv.y);
	}


	template<typename T>
	inline Vector2<T> operator - (const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return Vector2<T>(lhv.x-rhv.x, lhv.y-rhv.y);
	}



	template<typename T>
	inline Vector2<T> operator * (const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return Vector2<T>(lhv.x*rhv.x, lhv.y*rhv.y);
	}
	template<typename T, typename U>
	inline Vector2<T> operator * (const Vector2<T>& lhv, U rhv)
	{
		return Vector2<T>(lhv.x*(T)rhv, lhv.y*(T)rhv);
	}
	template<typename T, typename U>
	inline Vector2<T> operator * (U rhv, const Vector2<T>& lhv)
	{
		return lhv * (T)rhv;
	}



	template<typename T>
	inline Vector2<T> operator / (const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return Vector2<T>(lhv.x/rhv.x, lhv.y/rhv.y);
	}
	template<typename T, typename U>
	inline Vector2<T> operator / (const Vector2<T>& lhv, U rhv)
	{
		return Vector2<T>(lhv.x/(T)rhv, lhv.y/(T)rhv);
	}
	template<typename T, typename U>
	inline Vector2<T> operator / (U rhv, const Vector2<T>& lhv)
	{
		return Vector2<T>((T)lhv/rhv.x, (T)lhv/rhv.y);
		return lhv * (T)rhv;
	}



	template<typename T>
	inline bool operator == (const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return lhv.x == rhv.x && lhv.y == rhv.y;
	}

	template<typename T>
	inline bool operator != (const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return lhv.x != rhv.x || lhv.y != rhv.y;
	}




	template<typename T>
	inline Vector2<T> Normalize(const Vector2<T>& v)
	{
		float m = 1.0f / std::sqrt(v.x*v.x + v.y*v.y);
		return Vector2<T>(
			v.x * m,
			v.y * m);
	}



	template<typename T>
	inline Vector2<T> SafeNormalize(const Vector2<T>& v)
	{
		float m = std::sqrt(v.x*v.x + v.y*v.y);
		if (m == 0)
			return Vector2<T>(zero);
		m = 1.0f / m;
		return Vector2<T>(
			v.x * m,
			v.y * m);
	}



	template<typename T>
	inline Vector2<T> Clamp(const Vector2<T>& v, T max)
	{
		float m = std::sqrt(v.x*v.x + v.y*v.y);
		if (m <= max)
			return v;
		m = (1.0f / m) * max;
		return Vector2<T>(
			v.x * m,
			v.y * m);
	}


	template<typename T>
	inline T Dot(const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return lhv.x*rhv.x + lhv.y*rhv.y;
	}



	template<typename T>
	inline T Cross(const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return lhv.x*rhv.y - lhv.y*rhv.x;
	}



	template<typename T>
	inline T Length(const Vector2<T>& lhv)
	{
		return std::sqrt(Dot(lhv, lhv));
	}



	template<typename T>
	inline T LengthSquare(const Vector2<T>& lhv)
	{
		return Dot(lhv, lhv);
	}



	template<typename T>
	inline T Distance(const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return std::sqrt(Dot(lhv-rhv, lhv-rhv));
	}



	template<typename T>
	inline Vector2<T> Min(const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return Vector2<T>(
			Min(lhv.x, rhv.x),
			Min(lhv.y, rhv.y));
	}



	template<typename T>
	inline Vector2<T> Max(const Vector2<T>& lhv, const Vector2<T>& rhv)
	{
		return Vector2<T>(
			Max(lhv.x, rhv.x),
			Max(lhv.y, rhv.y));
	}



	typedef Vector2<int> Vector2i;
	typedef Vector2<float> Vector2f;


}
