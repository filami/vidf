#pragma once
#include "sphere.h"
#include "plane.h"
#include "ray.h"
#include "triangle.h"


namespace vidf
{


	template<typename T>
	struct RayIntersectResult
	{
		RayIntersectResult() {}

		explicit RayIntersectResult(Zero)
			:	ray(zero)
			,	distance(0)
			,	intersected(false) {}

		RayIntersectResult(const Ray<T>& _ray, bool _intersected, T _distance)
			:	ray(_ray)
			,	distance(_distance)
			,	intersected(intersected) {}

		operator bool () const {return intersected;}

		Vector3<T> GetPoint() const {return ray.origin + ray.direction * distance;}

		Ray<T> ray;
		T distance;
		bool intersected;
	};



	typedef RayIntersectResult<float> RayIntersectResultf;



	template<typename T>
	inline RayIntersectResult<T> RayPlaneIntersect(const Ray<T>& ray, const Plane<T>& plane)
	{
		Vector3<T> origin = ray.origin - plane.normal * plane.distance;
		T a = Dot(plane.normal, ray.direction);
		if (a == 0)
			return RayIntersectResult<T>(zero);
		T b = -Dot(plane.normal, origin) / a;
		if (b < 0)
			return RayIntersectResult<T>(zero);
		return RayIntersectResult<T>(ray, true, b);
	}



	template<typename T>
	inline RayIntersectResult<T> RaySphereIntersect(const Ray<T>& ray, const Sphere<T>& sphere)
	{
		Vector3<T> q = sphere.center - ray.origin;
		T c = Length(q);
		T v = Dot(q, ray.direction);
		T d = sphere.radius * sphere.radius - (c*c - v*v);
		if (d < 0)
			return RayIntersectResult<T>(zero);
		return RayIntersectResult<T>(ray, true, v - std::sqrt(d));
	}



	template<typename T>
	inline bool RayRayIntersection(const Vector2<T>& orig1, const Vector2<T>& dir1, const Vector2<T>& orig2, const Vector2<T>& dir2, Vector2<T>* result)
	{
		T x1 = orig1.x, x2 = orig1.x+dir1.x, x3 = orig2.x, x4 = orig2.x+dir2.x;
		T y1 = orig1.y, y2 = orig1.x+dir1.x, y3 = orig2.y, y4 = orig2.x+dir2.x;
		 
		T d = (x1-x2) * (y3-y4) - (y1-y2) * (x3-x4);
		if (d==0)
			return false;

		T pre = (x1*y2 - y1*x2);
		T post = (x3*y4 - y3*x4);
		T x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
		T y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

		if (result)
		{
			result->x = x;
			result->y = y;
		}

		return true;
	}



	template<typename T>
	inline bool LineLineIntersection(const Vector2<T>& p1, const Vector2<T>& p2, const Vector2<T>& p3, const Vector2<T>& p4, Vector2<T>* result)
	{
		T x1 = p1.x, x2 = p2.x, x3 = p3.x, x4 = p4.x;
		T y1 = p1.y, y2 = p2.y, y3 = p3.y, y4 = p4.y;
		 
		T d = (x1-x2) * (y3-y4) - (y1-y2) * (x3-x4);
		if (d==0)
			return false;

		T pre = (x1*y2 - y1*x2);
		T post = (x3*y4 - y3*x4);
		
		if (x < Min(x1, x2) || x > Max(x1, x2) ||
			x < Min(x3, x4) || x > Max(x3, x4))
			return false;
		if (y < Min(y1, y2) || y > Max(y1, y2) ||
			y < Min(y3, y4) || y > Max(y3, y4))
			return false;

		T x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
		T y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

		if (result)
		{
			result->x = x;
			result->y = y;
		}

		return true;
	}



	inline bool LineCircleIntersection(Vector2f center, float radius, Vector2f linePoint0, Vector2f linePoint1)
	{
		Vector2f origin = linePoint0;
		Vector2f direction = linePoint1 - linePoint0;

		Vector2f d = origin - center;
		float a = Dot(direction, direction);
		float b = Dot(d, direction);
		float c = Dot(d, d) - radius*radius;

		float disc = b * b - a * c;

		if (disc < 0.0f)
			return false;

		float sqrtDisc = std::sqrt(disc);
		float invA = 1.0f / a;
		float t0 = (-b - sqrtDisc) * invA;
		float t1 = (-b + sqrtDisc) * invA;

		bool collided = (t0 >= 0.0f && t0 <= 1.0f) || (t1 >= 0.0f && t1 <= 1.0f);

		return collided;
	}



	template<typename T>
	inline bool BoxTriangleIntersect(const Box<T>& box, const Triangle<T>& triangle)
	{
		const Plane<T> planes[6] =
		{
			Plane<T>(Vector3f(T(1), T(0), T(0)),  box.min.x),
			Plane<T>(Vector3f(T(-1), T(0), T(0)), -box.max.x),
			Plane<T>(Vector3f(T(0), T(1), T(0)),  box.min.y),
			Plane<T>(Vector3f(T(0), T(-1), T(0)), -box.max.y),
			Plane<T>(Vector3f(T(0), T(0), T(1)),  box.min.z),
			Plane<T>(Vector3f(T(0), T(0), T(-1)), -box.max.z),
		};
		uint flags[3] = {};
		for (uint i = 0; i < 6; ++i)
		{
			const Plane<T> plane = planes[i];
			uint planeFlags = 1 << i;
			for (uint j = 0; j < 3; ++j)
			{
				const Vector3<T> vertex = triangle[j];
				if (Distance(plane, vertex) >= 0.0f)
					flags[j] |= planeFlags;
			}
		}
		return (flags[0] | flags[1] | flags[2]) == 0x3f;
	}


}
