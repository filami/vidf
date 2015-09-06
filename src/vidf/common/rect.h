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


	typedef Rect<float> Rectf;
	typedef Rect<int> Recti;


}
