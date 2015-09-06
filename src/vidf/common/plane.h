#pragma once
#include "vector3.h"


namespace vidf
{


	template<typename T>
	struct Plane
	{
		Plane() {}

		explicit Plane(Zero)
			:	normal(zero)
			,	distance((T)0) {}

		template<typename U, typename V>
		Plane(Vector3<U> _normal, V _distance)
			:	normal(Vector3<T>(_normal))
			,	distance((T)_distance) {}

		Vector3<T> normal;
		T distance;
	};



	template<typename T>
	Plane<T> NormalPointPlane(const Vector3<T>& normal, const Vector3<T>& point)
	{
		Plane<T> result;
		result.normal = normal;
		result.distance = Dot(normal, point);
		return result;
	}



	template<typename T>
	Plane<T> TwoVectorPointPlane(const Vector3<T>& vector1, const Vector3<T>& vector2, const Vector3<T>& point)
	{
		return NormalPointPlane(SafeNormalize(Cross(vector1, vector2)), point);
	}


	template<typename T>
	T Distance(const Plane<T>& plane, const Vector3<T>& point)
	{
		return Dot(plane.normal, point) - plane.distance;
	}


	typedef Plane<float> Planef;


}
