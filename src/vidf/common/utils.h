#pragma once

#include <cmath>
#include <limits>
#include "interpolate.h"
#include "vector2.h"
#include "vector3.h"
#include "string.h"


#define ZeroStruct(str) ZeroMemory(&str, sizeof(str))



namespace vidf
{


	const float PI = std::acos(-1.0f);
	const float HALF_PI = PI/2.0f;
	const float TWO_PI = PI*2.0f;


#	define BIT(x) 1<<(x)



	template<typename T>
	inline T Max(T v1, T v2){
		return v1 > v2 ? v1 : v2;
	}

	template<typename T>
	inline T Min(T v1, T v2){
		return v1 > v2 ? v2 : v1;
	}


	template<typename T>
	inline T Square(T v1)
	{
		return v1 * v1;
	}


	template<typename T>
	inline T Cube(T v1)
	{
		return v1 * v1 * v1;
	}


	template<typename A0, typename A1>
	A0 DivUp(A0 quocient, A1 divider)
	{
		return (quocient % A0(divider) != 0) ? ((quocient + A0(divider)) / A0(divider)) : (quocient / A0(divider));
	}

	
	template<typename T>
	void Swap(T& a, T& b)
	{
		T t = a;
		a = b;
		b = t;
	}


	template<typename Tv, typename Tmin, typename Tmax>
	inline Tv Clamp(Tv v, Tmin n, Tmax m){
		return v<n?n:v>m?m:v;
	}



	template<typename T>
	inline T Saturate(T v){
		return v<0?0:v>(T)1?(T)1:v;
	}



	template<typename T>
	inline T SignSaturate(T v)
	{
		return v < T(-1) ? T(-1) : v > T(1) ? T(1) : v;
	}



	template<typename T>
	T SmoothStep(T min, T max, T x)
	{
		return Hermite2(Saturate((x-min)/(max-min)));
	}



	template<typename T>
	inline T LoopTime(T time, T start, T end) {
		return std::fmod(time-start, end-start);
	}



	template<typename T>
	inline T Lerp(T n, T m, float l){
		return m*l + n*(1-l);
	}



	template<typename T>
	inline T Sign(T v){
		return v<0?(T)-1:v>0?(T)1:0;
	}



	inline float Degrees2Radians(float d){
		return (d*PI)/180.0f;
	}
	inline float Radians2Degrees(float d){
		return (d*180.0f)/PI;
	}



	template<typename T>
	inline T cotg(T v){
		return -std::tan(v+HALF_PI);
	}



	inline bool isPowerOfTwo(unsigned int i){
		if(i == 0) return true;
		int c=0;
		for(int j = 1; j!=0; j=j<<1)
			if(i&j)
				c++;
		return c==1;
	}



	template<typename T>
	inline T logn(T base, T value){
		return std::logf(value)/std::log(base);
	}



	inline int MDC(int v1, int v2) {
		for(;;)
		{
			int rem = v1%v2;
			if(rem==0)
				return v2;
			v1 = v2;
			v2 = rem;
		}
	}



	inline int Factorial(int n)
	{
		if (n <= 1)
			return 1;
		return n*Factorial(n-1);
	}



	inline float IsNumber(float v)
	{
		return !(v != v);
	}



	inline void Interpolate(float* actual, float goal, float speed, float frameTime, float limit=0.0f)
	{
		assert(actual);

		float delta = goal - *actual;

		if (limit > 0.001f)
			delta = Max(Min(delta, limit), -limit);

		*actual += delta * Min(frameTime * speed, 1.0f);
	}



	inline void Interpolate(Vector2f* actual, Vector2f goal, float speed, float frameTime, float limit=0.0f)
	{
		assert(actual);

		Vector2f delta = goal - *actual;
		
		if (limit > 0.001f)
		{
			float len = Length(delta);

			if (len > limit)
			{
				delta = delta / len;
				delta = delta * limit;
			}
		}

		*actual = *actual + delta*Min(frameTime * speed, 1.0f);
	}



	inline void Interpolate(Vector3f* actual, Vector3f goal, float speed, float frameTime, float limit=0.0f)
	{
		assert(actual);

		Vector3f delta = goal - *actual;
		
		if (limit > 0.001f)
		{
			float len = Length(delta);

			if (len > limit)
			{
				delta = delta / len;
				delta = delta * limit;
			}
		}

		*actual = *actual + delta*Min(frameTime * speed, 1.0f);
	}



}
