#pragma once

#include "types.h"


namespace vidf {


	class Rand48 {
	public:
		typedef int32 ResultType;

		explicit Rand48(int32 x0 = 1) : mLCF(Cnv(x0)) { }
		explicit Rand48(uint64 x0) : mLCF(x0) { }
		template<class It> 
		Rand48(It& first, It last) : mLCF(first, last) { }

		void Seed(int32 x0 = 1) { mLCF.Seed(Cnv(x0)); }
		void Seed(uint64 x0) { mLCF.Seed(x0); }
		template<class It>
		void Seed(It& first, It last) {mLCF.seed(first,last);}
		int32 operator()() { return static_cast<int32>(mLCF() >> 17); }
		static bool Validation(int32 x) { return x == 1993516219; }

		int Min() const {return 0;}
		int Max() const {return std::numeric_limits<int32>::max(); }

	private:
		 LinearCongruential<uint64,
			uint64(0xDEECE66DUL) | (uint64(0x5) << 32),
			0xB, uint64(1)<<48, 0> mLCF;

		 static uint64 Cnv(int32 x) {
			 return (static_cast<uint64>(x) << 16) | 0x330e;
		 }
	};


}
