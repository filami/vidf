#pragma once

#include "types.h"



namespace vidf
{



	class Half {
	public:
		Half() {}
		Half(float value) {ToHalf(value);}
		inline operator float() const {return ToFloat();}
		void ToHalf(float r);
		float ToFloat() const;

	private:
		uint16 v;
	};


}
