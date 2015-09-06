#include "pch.h"
#include "half.h"


namespace vidf
{


	union halfType
	{
		unsigned short bits;
		struct
		{
			unsigned long m : 10;
			unsigned long e : 5;
			unsigned long s : 1;
		} ieee;
	};



	union ieee_single
	{
		float f;
		struct
		{
			unsigned long m : 23;
			unsigned long e : 8;
			unsigned long s : 1;
		} ieee;
	};



	void Half::ToHalf(float r)
	{
		ieee_single	f;
		f.f = r;
		halfType h;

		h.ieee.s = f.ieee.s;

		if ((f.ieee.e==0) && (f.ieee.m==0))
		{
			h.ieee.m = 0;
			h.ieee.e = 0;
			v = h.bits;
			return;
		}

		if ((f.ieee.e==0) && (f.ieee.m!=0))
		{
			h.ieee.m = 0;
			h.ieee.e = 0;
			v = h.bits;
			return;
		}

		if ((f.ieee.e==0xff) &&	(f.ieee.m==0))
		{
			h.ieee.m = 0;
			h.ieee.e = 31;
			v = h.bits;
			return;
		}

		if ((f.ieee.e==0xff) &&	(f.ieee.m!=0))
		{
			h.ieee.m = 1;
			h.ieee.e = 31;
			v = h.bits;
			return;
		}

		int new_exp = f.ieee.e-127;
		if (new_exp<-24)
		{
			h.ieee.m = 0;
			h.ieee.e = 0;
		}

		if (new_exp<-14)
		{
			h.ieee.e = 0;
			unsigned int exp_val = (unsigned int)	(-14 - new_exp);
			switch (exp_val) {
				case 0: h.ieee.m =	0; break;
				case 1: h.ieee.m =	512	+ (f.ieee.m>>14); break;
				case 2: h.ieee.m =	256	+ (f.ieee.m>>15); break;
				case 3: h.ieee.m =	128	+ (f.ieee.m>>16); break;
				case 4: h.ieee.m =	64 + (f.ieee.m>>17); break;
				case 5: h.ieee.m =	32 + (f.ieee.m>>18); break;
				case 6: h.ieee.m =	16 + (f.ieee.m>>19); break;
				case 7: h.ieee.m =	8 +	(f.ieee.m>>20);	break;
				case 8: h.ieee.m =	4 +	(f.ieee.m>>21);	break;
				case 9: h.ieee.m =	2 +	(f.ieee.m>>22);	break;
				case 10: h.ieee.m = 1;	break;
			}
			v = h.bits;
			return;
		}

		if (new_exp>15)
		{
			h.ieee.m = 0;
			h.ieee.e = 31;
			v = h.bits;
			return;
		}

		h.ieee.e	= new_exp+15;
		h.ieee.m	= (f.ieee.m	>> 13);
		v =	h.bits;
	}



	float Half::ToFloat() const
	{
		halfType h;
		ieee_single sng;

		h.bits = v;
		sng.ieee.s = h.ieee.s;

		if ((h.ieee.e==0) && (h.ieee.m==0))
		{
			sng.ieee.m=0;
			sng.ieee.e=0;
			return sng.f;
		}

		if ((h.ieee.e==0) && (h.ieee.m!=0))
		{
			const float half_denorm = (1.0f/16384.0f);
			float mantissa = ((float)(h.ieee.m)) / 1024.0f;
			float sgn = (h.ieee.s)? -1.0f :1.0f;
			sng.f = sgn*mantissa*half_denorm;
			return sng.f;
		}

		if ((h.ieee.e==31) && (h.ieee.m==0))
		{
			sng.ieee.e = 0xff;
			sng.ieee.m = 0;
			return sng.f;
		}

		if ((h.ieee.e==31) && (h.ieee.m!=0))
		{
			sng.ieee.e = 0xff;
			sng.ieee.m = 1;
			return sng.f;
		}

		sng.ieee.e = h.ieee.e+112;
		sng.ieee.m = (h.ieee.m << 13);
		return sng.f;
	}


}
