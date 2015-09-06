#pragma once
#include "types.h"
#include "color.h"

namespace vidf
{



	template<int order>
	class SHCoeffs
	{
	public:
		SHCoeffs() {}
		SHCoeffs(Zero)
		{
			for (unsigned int i = 0; i < order*order; ++i)
				values[i] = 0.0f;
		}

		float& operator[] (unsigned int index) {return values[index];}
		const float& operator[] (unsigned int index) const {return values[index];}

	private:
		float values[order*order];
	};



	template<int order>
	class ColorSHCoeffs
	{
	public:
		ColorSHCoeffs() {}
		ColorSHCoeffs(Zero)
			:	redBands(zero)
			,	greenBands(zero)
			,	blueBands(zero) {}

		void SetColor (unsigned int index, const Color& color)
		{
			redBands[index] = color.r;
			greenBands[index] = color.g;
			blueBands[index] = color.b;
		}

		const Color operator[] (unsigned int index) const
		{
			return Color(
				redBands[index],
				greenBands[index],
				blueBands[index]);
		}

		const SHCoeffs<order>& GetSphericalHarmonicCoefficients(int index) const {
			if (index == 0) return redBands;
			if (index == 1) return greenBands;
			if (index == 2) return blueBands;
		}

	private:
		SHCoeffs<order> redBands;
		SHCoeffs<order> greenBands;
		SHCoeffs<order> blueBands;
	};



	float SphericalHarmonicBand(int l, int m, const Vector3f& normal);


}
