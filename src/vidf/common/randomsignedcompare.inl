#pragma once


namespace vidf {


	template<bool signed1, bool signed2>
	struct DoCompare { };

	template<>
	struct DoCompare<false, false> {
		template<class T1, class T2>
		static bool Equal(T1 x, T2 y) { return x == y; }
		template<class T1, class T2>
		static bool LessThan(T1 x, T2 y) { return x < y; }
	};

	template<>
	struct DoCompare<true, true> : DoCompare<false, false> {};

	template<>
	struct DoCompare<true, false> {
		template<class T1, class T2>
		static bool Equal(T1 x, T2 y) { return x >= 0 && static_cast<T2>(x) == y; }
		template<class T1, class T2>
		static bool LessThan(T1 x, T2 y) { return x < 0 || static_cast<T2>(x) < y; }
	};

	template<>
	struct DoCompare<false, true> {
		template<class T1, class T2>
		static bool Equal(T1 x, T2 y) { return y >= 0 && x == static_cast<T1>(y); }
		template<class T1, class T2>
		static bool LessThan(T1 x, T2 y) { return y >= 0 && x < static_cast<T1>(y); }
	};


	template<class T1, class T2>
	int EqualSignedUnsigned(T1 x, T2 y) {
		typedef std::numeric_limits<T1> x_traits;
		typedef std::numeric_limits<T2> y_traits;
		return DoCompare<x_traits::is_signed, y_traits::is_signed>::Equal(x, y);
	}

	template<class T1, class T2>
	int LessThanSignedUnsigned(T1 x, T2 y) {
		typedef std::numeric_limits<T1> x_traits;
		typedef std::numeric_limits<T2> y_traits;
		return DoCompare<x_traits::is_signed, y_traits::is_signed>::LessThan(x, y);
	}

}
