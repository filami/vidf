#pragma once

#include "Vector3.h"



namespace vidf
{


	template<class T>
	class Triangle
	{
	public:
		Vector3<T> v0, v1, v2;
		Vector3<T> operator[] (uint idx) const { assert(idx < 3); return static_cast<const Vector3<T>*>(&v0)[idx]; }
		Vector3<T>& operator[] (uint idx) { assert(idx < 3); return static_cast<Vector3<T>*>(&v0)[idx]; }
	};



	template<typename T>
	Vector3<T> Normal(Triangle<T> triangle)
	{
		const Vector3<T> edge0 = triangle.v1 - triangle.v0;
		const Vector3<T> edge1 = triangle.v2 - triangle.v0;
		return Normalize(Cross(edge0, edge1));
	}


	typedef Triangle<float> Trianglef;
	typedef Triangle<int> Trianglei;


}
