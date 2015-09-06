#pragma once

#include "rect.h"
#include "utils.h"

namespace vidf {



	class AspectRatio {
	public:
		AspectRatio(): numerator(1), denominator(1) {}

		AspectRatio(unsigned width, unsigned height) {
			int mdc = MDC(width, height);
			numerator = width / mdc;
			denominator = height / mdc;
		}

		AspectRatio(const Recti& rect) {
			int width = (int)rect.max.x - (int)rect.min.x;
			int height = (int)rect.max.y - (int)rect.min.y;
			int mdc = MDC(width, height);
			numerator = width / mdc;
			denominator = height / mdc;
		}

		float GetAsFloat() const {return numerator / (float)denominator;}
		operator float() const {return numerator / (float)denominator;}

	private:
		unsigned numerator, denominator;
	};


}
