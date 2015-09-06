#pragma once
#include "sphericalharmonics.h"

namespace vidf
{


	template<int order>
	class SHMonteCarlo
	{
	public:
		template<typename RandomEngine>
		SHMonteCarlo(int quality, RandomEngine& randomEngine)
		{
			UniformReal<float> canonicalDist;

			const int sqrtNumSamples = quality;

			int numSamples = sqrtNumSamples * sqrtNumSamples;
			samples.resize(numSamples);
			sphericalHarmonicBands.resize(numSamples);

			int i = 0;
			for(int a = 0, i = 0; a < sqrtNumSamples; ++a)
			{
				for(int b = 0; b < sqrtNumSamples; ++b, ++i)
				{
					float x = (a + canonicalDist(randomEngine)) * (1.0f/sqrtNumSamples);
					float y = (b + canonicalDist(randomEngine)) * (1.0f/sqrtNumSamples);
					float theta = 2.0f * std::acos(std::sqrt(1.0f - x));
					float phi = 2.0f * PI * y;
					Vector3f sample(std::sin(theta)*std::cos(phi), std::sin(theta)*std::sin(phi), std::cos(theta));
					samples[i] = sample;

					for(int l = 0; l < order; ++l)
					{
						for(int m=-l; m<=l; ++m)
						{
							int index = l*(l+1)+m;
							sphericalHarmonicBands[i][index] = SphericalHarmonicBand(l,m,sample);
						}
					}
				}
			}
		}

		unsigned int GetNumSamples() const
		{
			return unsigned int(samples.size());
		}

		const Vector3f& GetSample(unsigned int index) const
		{
			return samples[size_t(index)];
		}

		const SHCoeffs<order>& GetSphericalHarmonicBands(unsigned int index) const
		{
			return sphericalHarmonicBands[size_t(index)];
		}

	private:
		std::vector<Vector3f> samples;
		std::vector<SHCoeffs<order>> sphericalHarmonicBands;
	};



	template<typename Integrator, int order>
	ColorSHCoeffs<order> IntegrateSphericalHarmonic(const SHMonteCarlo<order>& monteCarlo, Integrator integrator)
	{
		const float weight = 4.0f*PI;
		const int numCoefficients = order*order;

		ColorSHCoeffs<order> coefficients(zero);

		unsigned int numSamples = monteCarlo.GetNumSamples();
		for (unsigned int i = 0; i < numSamples; ++i)
		{
			Vector3f sample = monteCarlo.GetSample(i);
			Color value = integrator(sample);
			for (int n=0; n < numCoefficients; ++n)
			{
				coefficients.SetColor(
					n,
					coefficients[n] + value * monteCarlo.GetSphericalHarmonicBands(i)[n]);
			}
		}

		float factor = weight / numSamples;
		for (unsigned int i = 0; i < numCoefficients; ++i)
		{
			coefficients.SetColor(
				i,
				coefficients[i] * factor);
		}

		return coefficients;
	}


}
