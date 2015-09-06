#pragma once

namespace vidf
{


	template<typename T>
	float Cubic(T y0, T y1, T y2, T y3, T mu)
	{
		T a0,a1,a2,a3,mu2;
		mu2 = mu*mu;
		a0 = y3 - y2 - y0 + y1;
		a1 = y0 - y1 - a0;
		a2 = y2 - y0;
		a3 = y1;
		return a0*mu*mu2 + a1*mu2 + a2*mu + a3;
	}



	template<typename T>
	T Hermite1(T x)
	{
		return 2 * x*x*x - 3 * x*x + 1;
	}


	template<typename T>
	T Hermite2(T x)
	{
		return -2 * x*x*x + 3 * x*x;
	}


	template<typename T>
	T Hermite3(T x)
	{
		return x*x*x - 2 * x*x + x;
	}


	template<typename T>
	T Hermite4(T x)
	{
		return x*x*x - x*x;
	}


	template<typename VT, typename XT>
	VT Hermite(VT vertex0, VT vertex1, VT control0, VT control1, XT x)
	{
		return
			vertex0*Hermite1(x) +
			vertex1*Hermite2(x) +
			control0*Hermite3(x) +
			control1*Hermite4(x);
	}


}
