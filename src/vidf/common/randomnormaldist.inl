#pragma once


namespace vidf {



	template<class RealType = real32>
	class NormalDistribution {
	public:
		explicit NormalDistribution(RealType mean = RealType(0), RealType sigma = RealType(1))
			: mMean(mean), mSigma(sigma), mValid(false) {
			assert(sigma >= 0); // Sigma is not positive.
		}

		RealType Mean() const { return mMean; }
		RealType Sigma() const { return mSigma; }
		void SetMean(RealType v) {mMean = v;}
		void SetSigma(RealType v) {mSigma = v;}

		template<class Engine>
		RealType operator()(Engine& eng) {
			using std::sqrt; using std::log; using std::sin; using std::cos; using std::acos;
			if(!mValid) {
				RealType iv = RealType(1) / static_cast<RealType>(eng.Max() - eng.Min());
				mR1 = eng() * iv;
				mR2 = eng() * iv;
				mCachedRho = sqrt(-RealType(2) * log(RealType(1)-mR2));
				mValid = true;
			}
			else {
				mValid = false;
			}
			const RealType pi = acos(RealType(-1));

			return mCachedRho * (mValid ?
				cos(RealType(2)*pi*mR1) :
				sin(RealType(2)*pi*mR1))
				* mSigma + mMean;
		}

	private:
		RealType mMean, mSigma;
		RealType mR1, mR2, mCachedRho;
		bool mValid;
	};

}
