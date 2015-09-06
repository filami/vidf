#pragma once


namespace vidf {


	template<class IntType = int>
	class UniformSmallInt {
	public:
		explicit UniformSmallInt(IntType min = 0, IntType max = 9)
			: mMin(min), mMax(max) {
		}

		IntType Min() const { return mMin; }
		IntType Max() const { return mMax; }
		void SetMin(IntType v) {mMin = v;}
		void SetMax(IntType v) {mMax = v;}

		template<class Engine>
		IntType operator()(Engine& eng) {
			typedef typename Engine::ResultType BaseResult;
			BaseResult _range = static_cast<BaseResult>(mMax-mMin)+1;
			BaseResult _factor = 1;

			BaseResult r_base = eng.Max() - eng.Min();
			if(r_base == std::numeric_limits<BaseResult>::max()) {
				_factor = 2;
				r_base /= 2;
			}
			r_base += 1;
			if(r_base % _range == 0) {
				_factor = r_base / _range;
			}
			else {
				for( ; r_base/_range/32 >= _range; _factor *= 2)
					r_base /= 2;
			}

			return ((eng() - eng.Min()) / _factor) % _range + mMin;
		}

	private:
		IntType mMin;
		IntType mMax;
	};



	template<class IntType = int32>
	class UniformInt {
	public:
		explicit UniformInt(IntType min = 0, IntType max = 9)
			: mMin(min), mMax(max) {
			// assert(min <= max, "min major than max");
			Init();
		}

		IntType Min() const { return mMin; }
		IntType Max() const { return mMax; }
		void SetMin(IntType v) {mMin = v; Init();}
		void SetMax(IntType v) {mMax = v; Init();}

		template<class Engine>
		IntType operator()(Engine& eng) {
			typedef typename Engine::ResultType BaseResult;
			BaseResult bmin = eng.Min();
			BaseResult brange = eng.Max() - eng.Min();

			if(mRange == 0) {
				return mMin;
			}
			else if(EqualSignedUnsigned(brange, mRange)) {
				return static_cast<IntType>(eng() - bmin) + mMin;
			}
			else if(LessThanSignedUnsigned(brange, mRange)) {
				for(;;) {
					IntType limit;
					if(mRange == std::numeric_limits<IntType>::max()) {
						limit = mRange/(IntType(brange)+1);
						if(mRange % IntType(brange)+1 == IntType(brange))
							++limit;
					}
					else {
						limit = (mRange+1)/(IntType(brange)+1);
					}
					IntType result = IntType(0);
					IntType mult = IntType(1);
					while (mult <= limit) {
						result += (eng() - bmin) * mult;
						mult *= IntType(brange)+IntType(1);
					}
					if (mult == limit)
						return result;
					result += UniformInt<IntType>(0, mRange/mult)(eng) * mult;
					if(result <= mRange)
						return result + mMin;
				}
			}
			else {
				if(brange / mRange > 4) {
					return UniformSmallInt<IntType>(mMin, mMax)(eng);
				}
				else {
					for(;;) {
						BaseResult result = eng() - bmin;
						if(result <= static_cast<BaseResult>(mRange))
							return result + mMin;
					}
				}
			}
		}

	private:
		void Init() {
			mRange = mMax - mMin;
		}

		IntType mMin, mMax, mRange;
	};

}
