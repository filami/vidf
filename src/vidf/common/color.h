#pragma once

namespace vidf
{


	struct Color
	{
	public:
		Color() {}
		explicit Color(float _r, float _g, float _b, float _a=1.0f) : r(_r), g(_g), b(_b), a(_a) {}
		explicit Color(Zero) : r(0), g(0), b(0), a(0) {}

		float r, g, b, a;

		float R() const {return r;}
		float G() const {return g;}
		float B() const {return b;}
		float A() const {return a;}


		inline Color operator += (const Color& rhv)
		{
			r += rhv.r;
			g += rhv.g;
			b += rhv.b;
			a += rhv.a;
			return *this;
		}

		uint32 AsRGBA8() const
		{
			return
				(uint8(a * 255) << 24) |
				(uint8(b * 255) << 16) |
				(uint8(g * 255) << 8) |
				uint8(r * 255);
		}
	};


	inline Color operator + (const Color& lhv, const Color& rhv)
	{
		return Color(lhv.r+rhv.r, lhv.g+rhv.g, lhv.b+rhv.b, lhv.a+rhv.a);
	}

	inline Color operator - (const Color& lhv, const Color& rhv)
	{
		return Color(lhv.r - rhv.r, lhv.g - rhv.g, lhv.b - rhv.b, lhv.a - rhv.a);
	}


	template<typename T>
	inline Color operator * (const Color& lhv, T rhv)
	{
		return Color(lhv.r*(T)rhv, lhv.g*(T)rhv, lhv.b*(T)rhv, lhv.a*(T)rhv);
	}
	template<typename T>
	inline Color operator * (T rhv, const Color& lhv)
	{
		return lhv * (T)rhv;
	}

	template<typename T>
	inline Color operator / (const Color& lhv, T rhv)
	{
		return Color(lhv.r/(T)rhv, lhv.g/(T)rhv, lhv.b/(T)rhv, lhv.a/(T)rhv);
	}
	template<typename T>
	inline Color operator / (T rhv, const Color& lhv)
	{
		return lhv / (T)rhv;
	}

	template<typename T>
	inline Color operator += (Color& lhv, T rhv)
	{
		lhv.r += rhv;
		lhv.g += rhv;
		lhv.b += rhv;
		lhv.a += rhv;
		return lhv;
	}



	inline Color operator * (const Color& rhv, const Color& lhv)
	{
		return Color(lhv.r*rhv.r, lhv.g*rhv.g, lhv.b*rhv.b, lhv.a*rhv.a);
	}


	inline Color operator / (const Color& rhv, const Color& lhv)
	{
		return Color(lhv.r/rhv.r, lhv.g/rhv.g, lhv.b/rhv.b, lhv.a/rhv.a);
	}


}
