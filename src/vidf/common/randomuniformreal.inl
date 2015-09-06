#pragma once


namespace vidf {


	template<class RealType = real32>
	class UniformReal {
	public:
		explicit UniformReal(RealType min = RealType(0), RealType max = RealType(1))
			: mMin(min), mMax(max) {
			assert(min <= max); // min major than max
		}

		RealType Min() const { return mMin; }
		RealType Max() const { return mMax; }
		void SetMin(RealType v) {mMin = v;}
		void SetMax(RealType v) {mMax = v;}

		template<class Engine>
		RealType operator()(Engine& eng) {
			return static_cast<RealType>(eng() - eng.Min())
				/ static_cast<RealType>(eng.Max() - eng.Min())
				* (mMax - mMin) + mMin;
		}
	private:
		RealType mMin, mMax;
	};

}
