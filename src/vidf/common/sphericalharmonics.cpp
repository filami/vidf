#include "pch.h"
#include "utils.h"
#include "vector3.h"



namespace vidf
{


	namespace
	{

		double Legendre(int l, int m, double x)
		{
			double pmm = 1.0;

			if (m > 0)
			{
				double somx2 = std::sqrt((1.0-x)*(1.0+x));
				double fact = 1.0;
				for (int i=1; i<=m; i++)
				{
					pmm *= (-fact) * somx2;
					fact += 2.0;
				}
			}

			if (l == m)
				return pmm;

			double pmmp1 = x * (2.0*m+1.0) * pmm;

			if (l == m+1)
				return pmmp1;

			double pll = 0.0;
			for (int ll = m+2; ll<=l; ++ll) {
				pll = ((2.0*ll-1.0)*x*pmmp1-(ll+m-1.0)*pmm) / (ll-m);
				pmm = pmmp1;
				pmmp1 = pll;
			}

			return pll;
		}



		double SHNormalize(int l, int m)
		{
			const double pi = std::acos(-1.0f);
			double temp = ((2.0*l+1.0)*Factorial(l-m)) / (4.0*pi*Factorial(l+m));
			double result = std::sqrt(temp);
			return result;
		}

	}


	float SphericalHarmonicBand(int l, int m, const Vector3f& normal)
	{
		const double sqrt2 = std::sqrt(2.0f);
		const double thetaCos = (double) normal.z;
		const double phi = std::atan2((double)normal.x, (double)normal.y) - PI*0.5f;
		double result;

		if (m == 0)
			result = SHNormalize(l, 0) * Legendre(l, m, thetaCos);
		else if (m>0)
			result = sqrt2 * SHNormalize(l, m) * std::cos(m*phi) * Legendre(l, m, thetaCos);
		else
			result = sqrt2 * SHNormalize(l, -m) * std::sin(-m*phi) * Legendre(l, -m, thetaCos);

		return (float) result;
	}


}
