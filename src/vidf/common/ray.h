#pragma once
#include "vector3.h"


namespace vidf
{



	template<typename T>
	struct Ray
	{
		Ray() {}

		explicit Ray(Zero)
			:	origin(zero)
			,	direction(zero)
			,	minimum(0.0f)
			,	maximum(std::numeric_limits<T>::max()) {}

		template<typename U, typename V>
		Ray(Vector3<U> _origin, Vector3<V> _direction)
			:	origin(Vector3<T>(_origin))
			,	direction(Vector3<T>(_direction))
			,	minimum(0.0f)
			,	maximum(std::numeric_limits<T>::max()) {}

		Vector3<T> origin;
		Vector3<T> direction;
		float minimum;
		float maximum;
	};


	typedef Ray<float> Rayf;


}
