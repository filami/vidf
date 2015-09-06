#pragma once

#include "interpolate.h"


namespace vidf
{


	template<typename T>
	T Noise(int x)
	{
		x = (x<<13) ^ x;
		return (1 - ( (x * (x * x * 15731 + 789221) + 1376312589) & 0x7fffffff) / T(1073741824));
	}



	template<typename T>
	T Noise(int x, int y)
	{
		int n = x + y * 57;
		n = (n<<13) ^ n;
		return (1 - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / T(1073741824));
	}



	template<typename T>
	T Noise(int x, int y, int z)
	{
		int n = x + y * 57 + z * 593;
		n = (n<<13) ^ n;
		return (1 - ( (n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / T(1073741824));
	}



	template<typename T>
	T Noise(T x, T y)
	{
		;

		T fx = std::fmod(x, T(1));
		T fy = std::fmod(y, T(1));

		int ix = int(x);
		int iy = int(y);

		T n00 = Noise<T>(ix-1, iy-1);
		T n10 = Noise<T>(ix, iy-1);
		T n20 = Noise<T>(ix+1, iy-1);
		T n30 = Noise<T>(ix+2, iy-1);

		T n01 = Noise<T>(ix-1, iy);
		T n11 = Noise<T>(ix, iy);
		T n21 = Noise<T>(ix+1, iy);
		T n31 = Noise<T>(ix+2, iy);

		T n02 = Noise<T>(ix-1, iy+1);
		T n12 = Noise<T>(ix, iy+1);
		T n22 = Noise<T>(ix+1, iy+1);
		T n32 = Noise<T>(ix+2, iy+1);

		T n03 = Noise<T>(ix-1, iy+2);
		T n13 = Noise<T>(ix, iy+2);
		T n23 = Noise<T>(ix+1, iy+2);
		T n33 = Noise<T>(ix+2, iy+2);

		T c0 = Cubic(n00, n10, n20, n30, fx);
		T c1 = Cubic(n01, n11, n21, n31, fx);
		T c2 = Cubic(n02, n12, n22, n32, fx);
		T c3 = Cubic(n03, n13, n23, n33, fx);

		T r = Cubic(c0, c1, c2, c3, fy);

		return r;
	}



	template<typename T>
	T Noise(T x, T y, T z)
	{
		T fx = Hermite2(std::fmod(x, T(1)));
		T fy = Hermite2(std::fmod(y, T(1)));
		T fz = Hermite2(std::fmod(z, T(1)));

		int ix = int(x);
		int iy = int(y);
		int iz = int(z);

		T n000 = Noise<T>(ix, iy, iz);
		T n010 = Noise<T>(ix, iy+1, iz);
		T n110 = Noise<T>(ix+1, iy+1, iz);
		T n100 = Noise<T>(ix+1, iy, iz);
		T n001 = Noise<T>(ix, iy, iz+1);
		T n011 = Noise<T>(ix, iy+1, iz+1);
		T n111 = Noise<T>(ix+1, iy+1, iz+1);
		T n101 = Noise<T>(ix+1, iy, iz+1);

		T n0 = Lerp(Lerp(n000, n010, fy), Lerp(n100, n110, fy), fx);
		T n1 = Lerp(Lerp(n001, n011, fy), Lerp(n101, n111, fy), fx);
		T r = Lerp(n0, n1, fz);

		return r;
	}



	template<typename T>
	T PerlinNoise(T x, T y, int octaves)
	{
		T result = 0;
		T octave = T(1);
		T size = T(1);
		T feedback = 0;

		for (int i = 0; i < octaves; ++i)
		{
			result += Noise(x*octave, y*octave) * size;
			feedback += size;
			octave *= T(2);
			size *= T(0.5);
		}

		return result / feedback;
	}



	template<typename T>
	T PerlinNoise(T x, T y, T z, int octaves)
	{
		T result = 0;
		T octave = T(1);
		T size = T(1);
		T feedback = 0;

		for (int i = 0; i < octaves; ++i)
		{
			result += Noise(x*octave, y*octave, z*octave) * size;
			feedback += size;
			octave *= T(2);
			size *= T(0.5);
		}

		return result / feedback;
	}


}
