#pragma once

#include "Vector3.h"



namespace vidf {


	template<class T>
	class Box {
	public:
		Box() {}
		Box(T width, T height, T depth)
			: min(0, 0, 0), max(width, height, depth) {}

		Vector3<T> min, max;
	};



	template<class T>
	inline Box<T> Union(const Box<T>& b, const Vector3<T>& p) {
		Box<T> ret = b;

		ret.min.x = std::min(b.min.x, p.x);
		ret.min.y = std::min(b.min.y, p.y);
		ret.min.z = std::min(b.min.z, p.z);

		ret.max.x = std::max(b.max.x, p.x);
		ret.max.y = std::max(b.max.y, p.y);
		ret.max.z = std::max(b.max.z, p.z);

		return ret;
	}


	template<class T>
	inline Box<T> Union(const Box<T>& b, const Box<T>& b2) {
		Box<T> ret = b;

		ret.min.x = std::min(b.min.x, b2.min.x);
		ret.min.y = std::min(b.min.y, b2.min.y);
		ret.min.z = std::min(b.min.z, b2.min.z);

		ret.max.x = std::max(b.max.x, b2.max.x);
		ret.max.y = std::max(b.max.y, b2.max.y);
		ret.max.z = std::max(b.max.z, b2.max.z);

		return ret;
	}


	typedef Box<float> Boxf;
	typedef Box<int> Boxi;


}
