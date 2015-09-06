#pragma once


namespace vidf {


	template<class IntType, IntType a, IntType c, IntType m, IntType val>
	class LinearCongruential {
	public:
		static const bool mHasFixedRange = true;
		static const IntType mMinValue = ( c == 0 ? 1 : 0 );
		static const IntType mMaxValue = m-1;

		explicit LinearCongruential(IntType x0 = 1)
			: mModulus(m)
			, mX(mModulus ? (x0 % mModulus) : x0) { 
			assert(c || x0); // LinearCongruential failed
		}

		template<class It>
		LinearCongruential(It& first, It last) {
			seed(first, last);
		}

		void Seed(IntType x0 = 1) {
			assert(c || x0); // LinearCongruential failed
			mX = (mModulus ? (x0 % mModulus) : x0);
		}

		template<class It>
		void Seed(It& first, It last) {
			IntType value = *first++;
			mX = (mModulus ? (value % mModulus) : value);
		}

		IntType Min() const { return c == 0 ? 1 : 0; }
		IntType Max() const { return mModulus-1; }

		IntType operator()() {
			mX = MultAdd(a, mX, c);
			return mX;
		}

		static bool Validation(IntType x) { return val == x; }

	private:
		IntType mModulus;
		IntType mX;

		static IntType Add(IntType x, IntType c) { return c == 0 ? x : Mod(x+c); }
		static IntType Mult(IntType a, IntType x) { return Mod(a*x); }
		static IntType MultAdd(IntType a, IntType x, IntType c) { return Mod(a*x+c); }
		static IntType Mod(IntType x) { return x &= (m-1); }
	};


}
