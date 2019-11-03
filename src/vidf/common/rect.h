#pragma once

#include "Vector2.h"



namespace vidf {


	template<class T>
	class Rect {
	public:
		Rect() {}
		Rect(T width, T height)
			: min(0, 0), max(width, height) {}
		Rect(Vector2<T> _min, Vector2<T> _max)
			: min(_min), max(_max) {}

		Vector2<T> min, max;
	};



	template<typename T>
	bool IsPointInside(Rect<T> bounds, Vector2<T> point)
	{
		return
			(point.x >= bounds.min.x && point.x <= bounds.max.x) &&
			(point.y >= bounds.min.y && point.y <= bounds.max.y);
	}



	template<class T>
	inline Rect<T> Union(const Rect<T>& b, const Vector2<T>& p)
	{
		Rect<T> ret = b;

		ret.min.x = std::min(b.min.x, p.x);
		ret.min.y = std::min(b.min.y, p.y);

		ret.max.x = std::max(b.max.x, p.x);
		ret.max.y = std::max(b.max.y, p.y);

		return ret;
	}


	template<class T>
	inline Rect<T> Union(const Rect<T>& b, const Rect<T>& b2)
	{
		Rect<T> ret = b;

		ret.min.x = std::min(b.min.x, b2.min.x);
		ret.min.y = std::min(b.min.y, b2.min.y);

		ret.max.x = std::max(b.max.x, b2.max.x);
		ret.max.y = std::max(b.max.y, b2.max.y);

		return ret;
	}



	typedef Rect<float> Rectf;
	typedef Rect<int> Recti;


}
