#pragma once
#include "vector3.h"


namespace vidf
{



	template<typename T>
	struct Sphere
	{
		Sphere() {}

		explicit Sphere(Zero)
			:	center(zero)
			,	radius((T)0) {}

		Sphere(const Sphere<T>& v)
			:	center(v.center)
			,	radius(v.radius) {}

		template<typename U, typename V>
		Sphere(Vector3<U> _center, V _radius)
			:	center(Vector3<T>(_center))
			,	radius((T)_radius) {}

		Vector3<T> center;
		T radius;
	};


	typedef Sphere<float> Spheref;


}
