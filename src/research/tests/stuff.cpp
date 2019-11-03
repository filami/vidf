#include "pch.h"
#include <complex>
#include <json11.hpp>
#include "vidf/common/intersect.h"
#include "vidf/common/random.h"
#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"
#include "vidf/proto/text.h"
#include "renderpasses/averagebrightness.h"
#include "Box2D/Box2D.h"


using namespace vidf;
using namespace proto;


namespace
{






	struct LensElement
	{
		float offset;
		float d;
		float r1;
		float r2;
		float diam;
		float n;
	};



	void DrawCircle(Vector2f center, float radius, bool solid)
	{
		const uint segments = 64;
		glBegin(solid ? GL_POLYGON : GL_LINE_LOOP);
		for (uint i = 0; i < segments; ++i)
		{
			const float t = i / float(segments) * 2.0f * PI;
			const float x = std::cos(t) * radius + center.x;
			const float y = std::sin(t) * radius + center.y;
			glVertex2f(x, y);
		}
		glEnd();
	}



	void DrawArc(Vector2f center, float radius, float t0, float t1, uint segments)
	{
		glBegin(GL_LINE_STRIP);
		for (uint i = 0; i < segments; ++i)
		{
			const float t = Lerp(t0, t1, i / float(segments-1));
			const float x = std::cos(t) * radius + center.x;
			const float y = std::sin(t) * radius + center.y;
			glVertex2f(x, y);
		}
		glEnd();
	}


	void Draw(const LensElement& lens)
	{
		const Vector2f c1 = Vector2f( lens.r1 - lens.d * 0.5f + lens.offset, 0.0f);
		const Vector2f c2 = Vector2f(-lens.r2 + lens.d * 0.5f + lens.offset, 0.0f);

		const float t1 = std::asin(lens.diam * 0.5f / lens.r1);
		const float t2 = std::asin(lens.diam * 0.5f / lens.r2);

		glLineWidth(1.0f);
		glColor4ub(255, 255, 255, 255);
		DrawArc(c1, lens.r1, -t1 + PI, t1 + PI, 64);
		DrawArc(c2, lens.r2, -t2, t2, 64);
	}



	float GetFocusDistance(const LensElement& lens)
	{
		return 1.0f / ((lens.n - 1.0f)*(1.0f / lens.r1 - 1.0f*lens.r2 + (lens.n - 1.0f)*lens.d / (lens.n*lens.r1*lens.r2)));
	}



	struct Intersect
	{
		Vector3f normal = Vector3f(zero);
		float distance = std::numeric_limits<float>::max();
		float n = 1.0f;
	};



	Vector3f Refract(Vector3f I, Vector3f N, float eta)
	{
		const float k = 1.0 - eta * eta * (1.0 - Dot(N, I) * Dot(N, I));
		if (k < 0.0)
			return Vector3f(zero);
		else
			return eta * I - (eta * Dot(N, I) + std::sqrt(k)) * N;
	}



	bool RaySphereIntersect(const Rayf& ray, const Spheref& sphere, const float n, Intersect* intersect, bool inside)
	{
		const Vector3f q = sphere.center - ray.origin;
		const float c = Length(q);
		const float v = Dot(q, ray.direction);
		const float d = sphere.radius * sphere.radius - (c*c - v*v);
		if (d < 0.0f)
			return false;
		const float d0 = v - std::sqrt(d);
		const float d1 = v + std::sqrt(d);
		if (d0 < 0.0f && d1 < 0.0f)
			return false;
		const float dist = d0 < 0.0f ? d1 : d0;
		if (dist < intersect->distance)
		{
			intersect->distance = dist;
			intersect->normal = Normalize((ray.origin + ray.direction * dist) - sphere.center);
			intersect->normal = intersect->normal * (d0 < 0.0f ? -1.0f : 1.0f);
			intersect->n = inside ? 1.0f : n;
		}
		return true;
	}



	bool RaySphereIntersect(const Rayf& ray, const Spheref& sphere, Vector3f axis, float diam, float theta, const float n, Intersect* intersect, bool inside)
	{
		Intersect _intersect;
		const bool _result = RaySphereIntersect(ray, sphere, n, &_intersect, inside);
		if (!_result)
			return false;
		
		const Vector3f p = ray.origin + ray.direction * _intersect.distance;
		const Vector3f dir = Normalize(p - sphere.center);
		const float dt = theta;
		const float d = std::acos(Dot(dir, axis * Sign(sphere.radius)));

		if (d > dt)
			return false;

		if (_intersect.distance < intersect->distance)
			*intersect = _intersect;

		return true;
	}



	bool RayCylinderIntersect(const Rayf& ray, Vector3f axis, float radius, const float n, Intersect* intersect)
	{
		const float a = ray.direction.y * ray.direction.y + ray.direction.z * ray.direction.z;
		const float b = 2.0f * (ray.origin.y * ray.direction.y + ray.origin.z * ray.direction.z);
		const float c = ray.origin.y * ray.origin.y + ray.origin.z * ray.origin.z - radius * radius;

		if (std::abs(a) <= 1.0f / 1024.0f / 1024.0f)
			return false;

		const float discr = b*b - 4*a*c;
		if (discr < 0)
			return false;

		const float x1 = (-b + std::sqrt(discr)) / (2.0f * a);
		const float x2 = (-b - std::sqrt(discr)) / (2.0f * a);

		float t = 0.0f;
		if (x1 > x2)
			t = x2;
		if (t < 0)
			t = x1;
		if (t < 0)
			return false;
				
		const Vector3f point = ray.origin + ray.direction * t;
		const Vector3f normal = Vector3f(0.0f, 2.0f * point.y, 2.0f * point.z);
		
		if (t < intersect->distance)
		{
			intersect->distance = t;
			intersect->normal = Normalize(normal) * (c < 0.0f ? -1.0f : 1.0f);
			intersect->n = (c < 0.0f) ? 1.0f : n;
		}

		return true;
	}


		
	inline bool RayLensIntersect(const Rayf& ray, const LensElement& lens, Intersect* intersect)
	{
		Spheref s1{ Vector3f( lens.r1 - lens.d * 0.5f + lens.offset, 0.0f, 0.0f), lens.r1 };
		Spheref s2{ Vector3f(-lens.r2 + lens.d * 0.5f + lens.offset, 0.0f, 0.0f), lens.r2 };
		const float t1 = std::asin(lens.diam * 0.5f / std::abs(lens.r1));
		const float t2 = std::asin(lens.diam * 0.5f / std::abs(lens.r2));

		bool result = false;
		result |= RaySphereIntersect(ray, s1, Vector3f(-1.0f, 0.0f, 0.0f), lens.diam, t1, lens.n, intersect, false);
		result |= RaySphereIntersect(ray, s2, Vector3f( 1.0f, 0.0f, 0.0f), lens.diam, t2, lens.n, intersect, true);

		// result |= RayCylinderIntersect(ray, Vector3f(1.0f, 0.0f, 0.0f), lens.diam * 0.5f, lens.n, intersect);
		
		/*
		bool result = false;
		bool _result = false;
		Intersect _intersect;

		_result = RaySphereIntersect(ray, s1, lens.n, &_intersect);

		float d = Dot((ray.origin + ray.direction * _intersect.distance) - s1.center, Vector3f(-1.0f, 0.0f, 0.0f));
		if 

		// Dot(Dot(Vector3f(1.0f, 0.0f, 0.0f), _intersect.distance s1.center

		if (_result && _intersect.distance < intersect->distance)
		{
			*intersect = _intersect;
			result |= _result;
		}
		*/

		return result;
	}



	inline bool RayWorldIntersect(const LensElement* elements, uint elementCount, const Rayf ray, Intersect* intersect)
	{
		bool result = false;
		for (uint i = 0; i < elementCount; ++i)
			result |= RayLensIntersect(ray, elements[i], intersect);
		return result;
	}




	void TraceRay(const LensElement* elements, uint elementCount, const Rayf ray)
	{
		auto glVertex = [](Vector3f v) { glVertex2f(v.x, v.y); };
		Rayf curRay = ray;
		float curN = 1.0f;

		std::vector<Vector3f> points;
		std::vector<float> ns;

		points.push_back(curRay.origin);
		ns.push_back(1.0f);

		for (uint i = 0; i < 8; ++i)
		{
			Intersect intersect;
			if (!RayWorldIntersect(elements, elementCount, curRay, &intersect))
				break;
			curRay.origin = curRay.origin + curRay.direction * (intersect.distance + 1.0f / 1024.0f / 1024.0f);
			curRay.direction = Normalize(Refract(curRay.direction, intersect.normal, curN / intersect.n));
			curN = intersect.n;
			points.push_back(curRay.origin);
			ns.push_back(curN);
		};
		if (points.size() == 1)
			return;
		points.push_back(curRay.origin + curRay.direction * 25.0f);

		glLineWidth(1.5f);
		glBegin(GL_LINES);
		for (uint i = 0; i < points.size() - 1; ++i)
		{
			float n = ns[i];
		//	glColor4f(ns[i] - 1.0f, ns[i] - 0.5f, ns[i] * 2.0f - 1.0f, 1.0f);
			glVertex(points[i]);
			glVertex(points[i+1]);
		}
		glEnd();
		/*
		glPointSize(5.0f);
		glColor4ub(255, 128, 0, 255);
		glBegin(GL_POINTS);
		for (uint i = 0; i < points.size() - 1; ++i)
			glVertex(points[i]);
		glEnd();
		*/
	}



	Vector3f _SampleCosineHemisphere(Vector2f sample)
	{
		float u = sample.x * PI * 2.0f;
		float r = std::sqrt(sample.y);
		Vector3f d;
		d.x = std::cos(u) * r;
		d.y = std::sin(u) * r;
		d.z = std::sqrt(1.0f - d.x * d.x - d.y * d.y);
		return d;
	}



	void Lens()
	{
		std::vector<float> samplesX;
		std::vector<float> samplesY;

		for (uint j = 1; j <= 256 * 16; ++j)
		{
			const uint base = 2;
			float f = 1.0f;
			float r = 0.0f;
			int i = j;
			while (i > 0)
			{
				f = f / float(base);
				r = r + f * float(i % base);
				i = i / base;
			}
			samplesX.push_back(r);
		}
		for (uint j = 1; j <= 256 * 16; ++j)
		{
			const uint base = 3;
			float f = 1.0f;
			float r = 0.0f;
			int i = j;
			while (i > 0)
			{
				f = f / float(base);
				r = r + f * float(i % base);
				i = i / base;
			}
			samplesY.push_back(r);
		}

		ProtoGL protoGL;
		protoGL.Initialize(ProtoGLDesc(1280, 720));
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);

		CameraOrtho2D camera(protoGL.GetCanvas(), CameraOrtho2D::Upward);
		camera.SetCamera(Vector2f(zero), 0.5f);

		const float sensorSize = 0.025f;

		std::vector<LensElement> lenses;
		/*
		{
			LensElement lens;
			lens.offset = 0.035f;
			lens.d = 0.025f;
			lens.r1 = 0.07f;
			lens.r2 = 0.07f;
			lens.diam = 0.065f;
			lens.n = 1.5f;
			lenses.push_back(lens);
		}
		*/
		
		/*
		{
			LensElement lens;
			lens.offset = 0.25f;
			lens.d = 0.025f;
			lens.r1 = 0.07f;
			lens.r2 = 0.07f;
			lens.diam = 0.065f;
			lens.n = 1.5f;
			lenses.push_back(lens);
		}
		*/
		/*
		{
			LensElement lens;
			lens.offset = 0.035f;
			lens.d = 0.005f;
			lens.r1 = -0.07f;
			lens.r2 = -0.07f;
			lens.diam = 0.065f;
		//	lens.n = 1.5f;
			lens.n = 1.75f;
			lenses.push_back(lens);
		}
		*/
		/*
		{
			LensElement lens;
		//	lens.offset = 0.075f;
			lens.offset = 0.115f;
			lens.d = 0.025f;
		//	lens.r1 = 0.07f;
		//	lens.r2 = 0.07f;
			lens.r1 = 0.055f;
			lens.r2 = 2.0f;
			lens.diam = 0.065f;
			lens.n = 1.5f;
			lenses.push_back(lens);
		}
		*/

		const float loc = 0.11f;
		const float off[] = { 0.005f, 0.0f };
		{
			LensElement lens;
			lens.offset = loc + off[0];
			lens.d = 0.0075f;
			lens.r1 = -0.15f;
			lens.r2 = 0.075f;
			lens.diam = 0.065f;
			lens.n = 1.5f;
			lenses.push_back(lens);
		}
		{
			LensElement lens;
			lens.offset = loc + off[1];
			lens.d = 0.0015f;
			lens.r1 = -0.075f;
			lens.r2 = 0.1f;
			lens.diam = 0.065f;
			lens.n = 1.5f;
			lenses.push_back(lens);
		}

		float emitter = 0.0f;
		

		while (protoGL.Update())
		{
			Vector2f cp = camera.CursorPosition();

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			camera.CommitToGL();

			glEnable(GL_LINE_SMOOTH);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glLineWidth(1.5f);
			glColor4ub(0, 0, 255, 0255);
			glBegin(GL_LINES);
			glVertex2f(-25.0f, 0.0f);
			glVertex2f(25.0f, 0.0f);
			glEnd();

			glLineWidth(3.0f);
			glColor4ub(255, 255, 255, 255);
			glBegin(GL_LINES);
			glVertex2f(0.0f, -sensorSize * 0.5f);
			glVertex2f(0.0f,  sensorSize * 0.5f);
			glEnd();

			for (const auto& lens : lenses)
				Draw(lens);

			/*
			Rayf ray;
			ray.origin = Vector3f(cp.x, cp.y, 0.0f);
			ray.direction = Vector3f(1.0f, 0.0f, 0.0f);
			TraceRay(&lens, 1, ray);
			*/

		//	lenses[1].offset = cp.x;
			if (GetAsyncKeyState('1') & 0x8000)
				emitter = cp.y;
			if (GetAsyncKeyState('2') & 0x8000)
			{
				for (uint i = 0; i < lenses.size(); ++i)
					lenses[i].offset = cp.x + off[i];
			}

			/*
			Rayf ray;
			ray.origin = Vector3f(0.0f, cp.y, 0.0f);
			ray.direction = Vector3f(1.0f, 0.0f, 0.0f);
			TraceRay(lenses.data(), lenses.size(), ray);
			*/
			/*
			ray.origin = Vector3f(0.0f, cp.y, 0.0f);
			ray.direction = Normalize(Vector3f(lenses[0].offset, lenses[0].diam * 0.25f, 0.0f) - ray.origin);
			TraceRay(lenses.data(), lenses.size(), ray);

			ray.origin = Vector3f(0.0f, cp.y, 0.0f);
			ray.direction = Normalize(Vector3f(lenses[0].offset,-lenses[0].diam * 0.25f, 0.0f) - ray.origin);
			TraceRay(lenses.data(), lenses.size(), ray);
			*/

			glDisable(GL_LINE_SMOOTH);
			glBlendFunc(GL_ONE, GL_ONE);

			vidf::Rand48 rand48;
			vidf::UniformReal<float> unorm{ 0.0f, 1.0f };
			
			Rayf ray;
			for (uint i = 0; i < samplesX.size(); ++i)
			{
				ray.origin = Vector3f(0.0f, 0.0f, 0.0f);
				ray.direction = _SampleCosineHemisphere(Vector2f(samplesX[i], samplesY[i]));
				std::swap(ray.direction.x, ray.direction.z);
				ray.direction.z = 0.0f;

				const float a = 0.075f;
				glColor4f(0.2f * a, 0.4f * a, 1.0f * a, 1.0f);
				TraceRay(lenses.data(), lenses.size(), ray);
			}
			for (uint i = 0; i < samplesX.size(); ++i)
			{
				ray.origin = Vector3f(0.0f, emitter, 0.0f);
				ray.direction = _SampleCosineHemisphere(Vector2f(samplesX[i], samplesY[i]));
				std::swap(ray.direction.x, ray.direction.z);
				ray.direction.z = 0.0f;

				const float a = 0.075f;
				glColor4f(1.0f * a, 0.4f * a, 0.2f * a, 1.0f);
				TraceRay(lenses.data(), lenses.size(), ray);
			}

			protoGL.Swap();
		}
	}


}


namespace
{
}


namespace
{


	template<typename Int, uint precision>
	struct Fixed
	{
		Fixed() = default;
		
		Fixed(const Fixed& value)
			: data(value.data) {}

		explicit Fixed(Int value)
			: data(value << precision) {}

		Int GetMax() const
		{
			return std::numeric_limits<Int>::max() >> precision;
		}

		Int data;
	};


	template<typename Int, uint precision>
	Fixed<Int, precision> operator+ (Fixed<Int, precision> a, Fixed<Int, precision> b)
	{
		Fixed<Int, precision> r;
		r.data = a.data + b.data;
		return r;
	}



	template<typename Int, uint precision>
	Fixed<Int, precision> operator- (Fixed<Int, precision> a, Fixed<Int, precision> b)
	{
		Fixed<Int, precision> r;
		r.data = a.data - b.data;
		return r;
	}



	template<typename Int, uint precision>
	Fixed<Int, precision> Frac(const Fixed<Int, precision> value)
	{
		Fixed<Int, precision> r;
		r.data = value.data - ((value.data >> precision) << precision);
		return r;
	}



	template<typename Int, uint precision>
	void ToString(const Fixed<Int, precision> value, char* buffer, const uint bufferSize)
	{
		const Int maxInt = value.GetMax();
		Int maxInt10 = 1;
		while (maxInt10 < maxInt)
			maxInt10 *= 10;
		Int intPart = value.data >> precision;
		if (intPart < 0)
		{
			intPart = -intPart;
			*buffer++ = '-';
		}
		while (intPart / maxInt10 == 0)
			maxInt10 /= 10;
		for (; maxInt10 != 0; maxInt10 /= 10, ++buffer)
			*buffer = intPart / maxInt10 % 10 + '0';

		Int fracPart = Frac(value).data;
	}



	template<typename Value>
	class PIDController
	{
	public:
		template<typename Time>
		Value Compute(Value current, Time deltaTime)
		{
			const Value error = target - current;

			integral += error * deltaTime;
			const Value derivative = error / deltaTime;

			const Value result =
				error * kPropotional +
				integral * kIntegral +
				derivative * kDerivative;

			return result;
		}

		void SetTarget(Value _target)
		{
			target = _target;
		}
		void SetPID(Value _kPropotional, Value _kIntegral, Value _kDerivative)
		{
			kPropotional = _kPropotional;
			kIntegral = _kIntegral;
			kDerivative = _kDerivative;
		}

		void Reset()
		{
			integral = 0;
		}

	private:
		Value target       = Value(0);
		Value kPropotional = Value(1);
		Value kIntegral    = Value(0);
		Value kDerivative  = Value(0);

		Value integral   = Value(0);
	};


	typedef double Real;

	typedef PIDController<Real> PIDControllerReal;



	struct TunedPIDDesc
	{
		Real maxForce    = 1.0;
		Real maxVelocity = 1.0;
		Real inertia     = 1.0;
		Real kP          = 1.0;
		Real kI          = 0.0;
		Real kD          = 0.0;
	};

	class TunnedPID
	{
	public:
		TunnedPID(TunedPIDDesc _desc = TunedPIDDesc())
		{
			SetDesc(_desc);
		}

		void SetDesc(TunedPIDDesc _desc)
		{
			desc = _desc;
			const Real invInertia = 1.0 / desc.inertia;
			timeToStop = desc.maxVelocity / (desc.maxForce * invInertia);
			invDistanceToStop = 1.0 / ((desc.maxForce * invInertia) * timeToStop * timeToStop * 0.5f);
			invMaxVelocity = 1.0 / desc.maxVelocity;
			pid.SetPID(desc.kP, desc.kI, desc.kD);
		}

		void SetTargetPosition(Real _target)
		{
			targetPosition = _target;
		}

		Real Curve(Real x, Real curve) const
		{
			return x;

			x /= curve;
			if (x > 1.0f)
				x -= 2 / 3.0;
			else if (x < -1.0f)
				x += 2 / 3.0;
			else
				x = x * x * x / 3.0;
			return x * curve;
		}

		Real Compute(Real currentPosition, Real currentVelocity, Real deltaTime)
		{
#if 0
			const Real targetVelocity = Clamp(
				(targetPosition - currentPosition) * invDistanceToStop * desc.maxVelocity,
				-desc.maxVelocity, desc.maxVelocity);

			pid.SetTarget(targetVelocity * invMaxVelocity);
			const Real pidValue = pid.Compute(currentVelocity * invMaxVelocity, deltaTime);

			Real targetForce = Clamp(pidValue * desc.maxForce, -desc.maxForce, desc.maxForce);
			if (std::abs(targetForce) < desc.maxForce * 0.1)
				targetForce = 0.0;
			force = Lerp(targetForce, force, std::exp(-deltaTime  *16.0));
#endif

			const Real diff = Curve(targetPosition - currentPosition, 0.05);

			const Real targetVelocity = Clamp(
				diff * invDistanceToStop * desc.maxVelocity,
				-desc.maxVelocity, desc.maxVelocity);

			pid.SetTarget(targetVelocity * invMaxVelocity);
			const Real pidValue = pid.Compute(currentVelocity * invMaxVelocity, deltaTime);

			Real targetForce = Clamp(pidValue * desc.maxForce, -desc.maxForce, desc.maxForce);
			targetForce = Curve(targetForce, 0.025);
			force = Lerp(targetForce, force, std::exp(-deltaTime * 16.0));

			if (abs(force) < desc.maxForce * 0.025)
				force = 0.0;

			return force;
		}

	private:
		TunedPIDDesc      desc;
		PIDControllerReal pid;
		Real timeToStop;
		Real invDistanceToStop;
		Real invMaxVelocity;
		Real targetPosition = 0.0;
		Real force = 0.0;
	};



	template<typename Type>
	struct Range
	{
		Range() = default;
		Range(Type __begin, Type __end)
			: _begin(__begin)
			, _end(__end) {}
		template<typename ADT>
		explicit Range(const ADT& adt)
			: _begin(adt.begin())
			, _end(adt.end()) {}

		Type begin() const { return _begin; }
		Type end() const { return _end; }

		Type _begin;
		Type _end;
	};



	template<typename Type, bool clamp=true>
	Type Map(const Range<Type>& srcRange, const Range<Type>& destRange, Type value)
	{
		const Type slope0 = Type(1) / (srcRange.end() - srcRange.begin());
		Type normal = value * slope0 - slope0 * srcRange.begin();
		if (clamp)
			normal = Saturate(normal);
		const Type slope1 = destRange.end() - destRange.begin();
		return normal * slope1 + destRange.begin();
	}


	typedef Range<float> Rangef;



	void glVertex(const Vector2f v)
	{
		glVertex2f(v.x, v.y);
	}



	void glVertex(const Vector3f v)
	{
		glVertex3f(v.x, v.y, v.z);
	}



	struct SliderDesc
	{
		Rangef range = Rangef{0.0f, 1.0f};
		bool drawBorders = true;
		bool drawZero = true;
		enum Orientation
		{
			Horizontal,
			Vertical
		} orientation = Horizontal;
	};


	class Slider
	{
	public:
		Slider(SliderDesc _desc, Rectf _rect, float _value)
			: desc(_desc)
			, rect(_rect)
			, value(_value) {}

		void glDraw(const Vector2f offset = Vector2f{zero})
		{
			const Rangef rectRange =
				(desc.orientation == SliderDesc::Horizontal) ?
					Rangef{rect.min.x, rect.max.x} :
					Rangef{ rect.min.y, rect.max.y };

			glLineWidth(3.0f);
			if (std::abs(value) < 1.0f / (1024.0f * 1024.0f))
				glColor4ub(64, 255, 64, 255);
			else
				glColor4ub(255, 192, 0, 255);				
			float markerPos = Map(desc.range, rectRange, value);
			glBegin(GL_LINES);
			if (desc.orientation == SliderDesc::Horizontal)
			{
				glVertex2f(markerPos, rect.min.y);
				glVertex2f(markerPos, rect.max.y);
			}
			else
			{
				glVertex2f(rect.min.x, markerPos);
				glVertex2f(rect.max.x, markerPos);
			}
			glEnd();

			glLineWidth(1.0f);
			if (desc.drawBorders)
			{
				glBegin(GL_LINE_LOOP);
				glColor4ub(192, 192, 192, 255);
				glVertex(rect.min + (rect.max - rect.min) * Vector2f(0, 0) + offset);
				glVertex(rect.min + (rect.max - rect.min) * Vector2f(1, 0) + offset);
				glVertex(rect.min + (rect.max - rect.min) * Vector2f(1, 1) + offset);
				glVertex(rect.min + (rect.max - rect.min) * Vector2f(0, 1) + offset);
				glEnd();
			}

			if (desc.drawZero && desc.range.begin() < 0.0f && desc.range.end() > 0.0f)
			{
				float zeroPos = Map(desc.range, rectRange, 0.0f);
				glBegin(GL_LINES);
				glColor4ub(192, 192, 192, 255);
				if (desc.orientation == SliderDesc::Horizontal)
				{
					glVertex2f(zeroPos, rect.min.y);
					glVertex2f(zeroPos, rect.max.y);
				}
				else
				{
					glVertex2f(rect.min.x, zeroPos);
					glVertex2f(rect.max.x, zeroPos);
				}
				glEnd();
			}
		}

		void SetValue(float _value) { value = _value; }

	private:
		SliderDesc desc;
		Rectf rect;
		float value;
	};



	void PID()
	{
		using namespace std;

		typedef Fixed<int64, 20> Fixed64;
		Fixed64 c = Fixed64(4) - Fixed64(6);
		char buffer[256] = {};
		ToString(c, buffer, 256);
		std::cout << buffer << std::endl;

		ProtoGL protoGL;
		protoGL.Initialize(ProtoGLDesc(1280, 720));
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);

		CameraOrtho2D camera(protoGL.GetCanvas(), CameraOrtho2D::Upward);
		camera.SetCamera(Vector2f(zero), 20.0f);

		Text text;
		text.Init(0.2f);

		Real target = 5.0f;
		const Real mass = 10.0;
		const float maxSpeed = 0.5;
		const float maxForce = 2.0;
	//	Real target = 0.21706687198051303;
	//	const Real mass = 1.0;
	//	const float maxSpeed = Degrees2Radians(5.0f);
	//	const float maxForce = 0.15;

		TunedPIDDesc pidDesc;
		pidDesc.maxForce = maxForce;
		pidDesc.maxVelocity = maxSpeed;
		pidDesc.inertia = mass;
		pidDesc.kP = 2.0;
		pidDesc.kI = 0.5;
		pidDesc.kD = 0.5;

		TimeCounter timer;
		
		while (protoGL.Update())
		{
			Time deltaTime = timer.GetElapsed();

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

#if 0
			const Real mass = 10.0;
			const Real target = camera.CursorPosition().y;
			{
				const Real delta = deltaTime.AsFloat();
				pid.SetTarget(target);
				const Real force = pid.Compute(position, delta);
				std::cout << force << std::endl;
				const Real acceleration = force / mass;
				velocity += acceleration * delta;
				position += velocity * delta;
			}

			{
				camera.CommitToGL();

				glBegin(GL_LINES);
				glColor4ub(255, 0, 0, 255);
				glVertex2f(0.0f, -10.0f);
				glVertex2f(0.0f,  10.0f);
				glColor4ub(0, 255, 0, 255);
				glVertex2f(-0.05f, target);
				glVertex2f( 0.05f, target);
				glColor4ub(0, 128, 255, 255);
				glVertex2f(-0.05f, position);
				glVertex2f( 0.05f, position);
				glEnd();
			}
#endif
			const Vector2f cursor = camera.CursorPosition();
			const Real delta = 1.0 / 60.0;
			const Real simulationTime = 100.0;

			// 0.025384786831174348
			Real position = 0.0;
			Real velocity = 0.0;
		//	Real position = 0.025384786831174348;
		//	Real velocity = -maxSpeed;
			PIDControllerReal pid;
			static Real i = 0;
			if (GetAsyncKeyState('A') && 0x8000)
				i += 0.001f;
			if (GetAsyncKeyState('Z') && 0x8000)
				i -= 0.001f;
			pid.SetPID(1, 0.25, 0.5);
			TunnedPID controller{ pidDesc };
						
			if (GetAsyncKeyState('S') && 0x8000)
				target += 0.01f;
			if (GetAsyncKeyState('X') && 0x8000)
				target -= 0.01f;

			camera.CommitToGL();
			glLoadIdentity();
		//	glScalef(1.0f, 10.0f, 1.0f);

			glBegin(GL_LINES);
			glColor4ub(255, 0, 0, 255);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(simulationTime, 0.0f);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(0.0f, target * 4.0f);

			glColor4ub(0, 255, 0, 255);
			glVertex2f(0.0f, target);
			glVertex2f(simulationTime, target);
			glVertex2f(0.0f, target - 0.025384786831174348);
			glVertex2f(simulationTime, target - 0.025384786831174348);

			glColor4ub(0, 128, 0, 255);
			glVertex2f(0.0f, maxSpeed);
			glVertex2f(simulationTime, maxSpeed);

			glColor4ub(0, 128, 128, 255);
			glVertex2f(0.0f, maxForce);
			glVertex2f(simulationTime, maxForce);

			glColor4ub(255, 255, 0, 255);
			glVertex2f(cursor.x, cursor.y - 20.0f); glVertex2f(cursor.x, cursor.y + 20.0f);
			glVertex2f(cursor.x - 20.0f, cursor.y); glVertex2f(cursor.x + 20.0f, cursor.y);
			glEnd();

			enum State
			{
				Accelerating,
				Moving,
				Decelerating,
				Parking,
			};
			static State state;
			static Real time = 0;
			state = Parking;

			Real forceIntegral = 0;

			glBegin(GL_LINES);
			float t0 = 0;
			float t1 = 0;
			float p00 = 0;
			float p01 = 0;
			float p10 = 0;
			float p11 = 0;
			float p20 = 0;
			float p21 = 0;
			for (Real t = 0; t <= simulationTime; t += delta)
			{
				/*
				Physical based:
					time = 11.5
					fuel = 8

				Untuned PID
					time = 26
					fueld = 12

				Tuned PID
					time = 15
					fueld = 10
				*/


#if 0
				// Untuned PID
				const Real targetVelocity = Clamp((target - position) / 2.0f, -maxSpeed, maxSpeed);
				pid.SetTarget(targetVelocity);
				const Real force = Clamp(pid.Compute(velocity, delta), -maxForce, maxForce);
#endif

#if 0
				// Physical based
				/*
				F = m a
				a = F / m
				v = a t
				v = (F / m) t
				t = v / (F / m)
				*/
				const Real timeToStop = maxSpeed / (maxForce / mass);
				/*
				p = (a t2) / 2
				p = ((F/m) t2) / 2
				*/
				const Real distanceToStop = (maxForce / mass) * timeToStop * timeToStop * 0.5f;
				const Real error = target - position;
				Real force = 0;
				Real expectedVelocity = 0;
				if (abs(error) > distanceToStop && velocity < maxSpeed)
				{
					if (state != Accelerating)
						time = 0;
					state = Accelerating;
				}
				else if (abs(error) < distanceToStop * 0.2)
				{
					if (state != Parking)
						pid.Reset();
					state = Parking;
				}
				else if (abs(error) < distanceToStop)
				{
					if (state != Decelerating)
						time = 0;
					state = Decelerating;
				}
				else
				{
					state = Moving;
				}

				switch (state)
				{
				case Accelerating:
					force = Sign(error) * maxForce;
					expectedVelocity = (maxForce / mass) * time;
					break;
				case Decelerating:
					force = -Sign(velocity) * maxForce;
					expectedVelocity = maxSpeed - (maxForce / mass) * time;
					break;
				case Moving:
					expectedVelocity = maxSpeed;
					break;
				case Parking:
					force = 0;
					expectedVelocity = 0;

					{
						const Real targetVelocity = 0;
						pid.SetTarget(targetVelocity);
						force = Clamp(pid.Compute(velocity, delta), -maxForce, maxForce);
					}

					break;
				}
				const Real velocityError = velocity - expectedVelocity;
				force = Clamp(force, -maxForce, maxForce);
				time += delta;
#endif

#if 1
				// Tuned PID
				const Real timeToStop = maxSpeed / (maxForce / mass);
				const Real distanceToStop = (maxForce / mass) * timeToStop * timeToStop * 0.5f;
				const Real targetVelocity = Clamp((target - position) * distanceToStop, -maxSpeed, maxSpeed);
				pid.SetPID(10, 1, 1);
				// pid.SetPID(0.5, 2, 1);
				pid.SetTarget(targetVelocity / maxSpeed);
				const Real pidValue = pid.Compute(velocity / maxSpeed, delta);
				const Real force1 = Clamp(pidValue * maxForce, -maxForce, maxForce);
				
			//	if (position > target - 0.025384786831174348)
			//		__debugbreak();
				controller.SetTargetPosition(target);
				const Real force = controller.Compute(position, velocity, delta);
				// const Real force = maxForce;
#endif

				Real acceleration = force / mass;
				velocity += acceleration * delta;
				position += velocity * delta;
				forceIntegral += abs(force) * delta;

				t1 = t;
				p01 = position;
				p11 = velocity;
				// p21 = forceIntegral;
				p21 = force;
				glColor4ub(0, 128, 255, 255);
				glVertex2f(t0, p00); glVertex2f(t1, p01);
				glColor4ub(128, 128, 0, 255);
				glVertex2f(t0, p10); glVertex2f(t1, p11);
				glColor4ub(128, 0, 128, 255);
				glVertex2f(t0, p20); glVertex2f(t1, p21);
				t0 = t1;
				p00 = p01;
				p10 = p11;
				p20 = p21;
			}
			glEnd();

			text.size = 0.5f;
			glColor4ub(255, 255, 0, 255);
			text.OutputText(cursor, "%.3f %.3f", cursor.x, cursor.y);

			protoGL.Swap();
		}
	}



	void NavBall()
	{
		using namespace std;

		const Real PI = acos(-1.0);
		const Real iconSize = 0.035;

		ProtoGL protoGL;
		protoGL.Initialize(ProtoGLDesc(1280, 720));
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);

		CameraOrtho2D camera(protoGL.GetCanvas(), CameraOrtho2D::Upward);
		camera.SetCamera(Vector2f(zero), 2.5f);

		Text text;
		text.Init(0.2f);
				
		SliderDesc sliderDesc;
		sliderDesc.range = Rangef{ -5.0f, 5.0f };
		Slider rollSlider{ sliderDesc, Rectf(Vector2f(-1.0f, 1.0f), Vector2f(1.0f, 1.2f)), 0.0f };
		Slider yawSlider{ sliderDesc, Rectf(Vector2f(-1.0f, -1.0f), Vector2f(1.0f, -1.2f)), 0.0f };
		sliderDesc.orientation = SliderDesc::Vertical;
		Slider pitchSlider{ sliderDesc, Rectf(Vector2f(1.0f, -1.0f), Vector2f(1.2f, 1.0f)), 0.0f };
		sliderDesc.range = Rangef{ 0.0, Radians2Degrees(0.15) };
		Slider accSlider{ sliderDesc, Rectf(Vector2f(-1.0f, -1.0f), Vector2f(-1.2f, 1.0f)), 0.0f };
				
		sliderDesc.range = Rangef{ -6.0f, 6.0f };
		sliderDesc.drawBorders = false;
		sliderDesc.orientation = SliderDesc::Horizontal;
		Slider yawErrorSlider{ sliderDesc, Rectf(Vector2f(-1.0f, -1.0f), Vector2f(1.0f, -0.5f)), 0.0f };
		sliderDesc.orientation = SliderDesc::Vertical;
		Slider pitchErrorSlider{ sliderDesc, Rectf(Vector2f(1.0f, -1.0f), Vector2f(0.5f, 1.0f)), 0.0f };

		typedef Vector3<Real> Vector3R;
		typedef Quaternion<Real> QuaternionR;
		// QuaternionR orientation = QuaternionAxisAngle(Normalize(Vector3R(1, 2, 3)), 1.5);
		QuaternionR orientation{ zero };
		Vector3R localAngVel{zero};
		Vector3R prograde = Normalize(Vector3R(-2, 4, -5));

		vector<Vector2f> progradeMarker;
		progradeMarker.push_back(Vector2f(-1,  0)); progradeMarker.push_back(Vector2f( 0, -1));
		progradeMarker.push_back(progradeMarker.back()); progradeMarker.push_back(Vector2f(1, 0));
		progradeMarker.push_back(progradeMarker.back()); progradeMarker.push_back(Vector2f(0, 1));
		progradeMarker.push_back(progradeMarker.back()); progradeMarker.push_back(progradeMarker.front());

		const float invSqr2 = 1.0f / sqrt(2.0f);
		vector<Vector2f> retrogradeMarker;
		retrogradeMarker.push_back(Vector2f(-invSqr2, -invSqr2)); retrogradeMarker.push_back(Vector2f( invSqr2, invSqr2));
		retrogradeMarker.push_back(Vector2f( invSqr2, -invSqr2)); retrogradeMarker.push_back(Vector2f(-invSqr2, invSqr2));

		vector<Vector2f> noseMarker;
		noseMarker.push_back(Vector2f(-1.0f, 0.0f)); noseMarker.push_back(Vector2f(-0.5f, 0.0f));
		noseMarker.push_back(noseMarker.back()); noseMarker.push_back(Vector2f(0.0f, -0.5f));
		noseMarker.push_back(noseMarker.back()); noseMarker.push_back(Vector2f(0.5f, 0.0f));
		noseMarker.push_back(noseMarker.back()); noseMarker.push_back(Vector2f(1.0f, 0.0f));

		auto glDrawMarker = [](const vector<Vector2f>& marker, Vector2f loc, float size)
		{
			for (auto p : marker)
				glVertex2f(p.x * size + loc.x, p.y * size + loc.y);
		};

		auto IconScale = [iconSize](Vector3R offset)
		{
			return float((offset.z * 0.25 + 0.75) * iconSize);
		};

		auto AngularDifference = [PI](Vector3R refDir, Vector3R testDir)
		{
		//	const Vector3R cross = Cross(refDir, testDir);
		//	return Vector3R(std::asin(cross.x), std::asin(cross.y), std::asin(cross.z));
			const Real eps = std::numeric_limits<Real>::epsilon();
			const Real d = Dot(refDir, testDir);
			if (d > 1 - eps)
				return Vector3R{ zero };
			else if (d < -1 + eps)
				return Vector3R(0.0, PI * 2, 0.0);
			else
				return Normalize(Cross(refDir, testDir)) * acos(d);
		};

		TimeCounter timer;

		SetCursorPos(200, 200);
		POINT lastPoint;
		GetCursorPos(&lastPoint);
		Vector3f smoothedCursor{zero};

		enum Mode
		{
			Man,
			KillRot,
			Pro,
			Retro,
		} mode = Man;
		Vector3R targetDir = Vector3R(0, 0, 1);
		PIDControllerReal yawControl;
		PIDControllerReal pitchControl;
		PIDControllerReal rollControl;
		const Real kP = 5;
		const Real kI = 1;
		const Real kD = 1;
		yawControl.SetPID(kP, kI, kD);
		pitchControl.SetPID(kP, kI, kD);
		rollControl.SetPID(kP, kI, kD);

		const Real maxAcc = 0.15;
		const Real maxAngVel = Degrees2Radians(5.0f);

		TunedPIDDesc controlDesc;
		controlDesc.maxForce = maxAcc;
		controlDesc.maxVelocity = maxAngVel;
		controlDesc.inertia = 1.0;
	//	controlDesc.kP = 1.0;
	//	controlDesc.kI = 0.5;
	//	controlDesc.kD = 0.5;
		controlDesc.kP = 2.0;
		controlDesc.kI = 0.5;
		controlDesc.kD = 0.5;
		TunnedPID _yawControl{ controlDesc };
		TunnedPID _pitchControl{ controlDesc };

		while (protoGL.Update())
		{
			Time deltaTime = timer.GetElapsed();

			Vector3R input{ zero };
			{
				if (GetAsyncKeyState('A') && 0x8000)
					input.y += 1.0;
				if (GetAsyncKeyState('D') && 0x8000)
					input.y -= 1.0;
				if (GetAsyncKeyState('W') && 0x8000)
					input.x -= 1.0;
				if (GetAsyncKeyState('S') && 0x8000)
					input.x += 1.0;
				if (GetAsyncKeyState('E') && 0x8000)
					input.z += 1.0;
				if (GetAsyncKeyState('Q') && 0x8000)
					input.z -= 1.0;

				if (GetAsyncKeyState('1') && 0x8000)
					mode = Man;
				if (GetAsyncKeyState('2') && 0x8000)
				{
					if (mode != KillRot)
						targetDir = Rotate(Inverse(orientation), Vector3R(0, 0, 1));
					mode = KillRot;
				}
				if (GetAsyncKeyState('3') && 0x8000)
					mode = Pro;
				if (GetAsyncKeyState('4') && 0x8000)
					mode = Retro;
			}
						
			Vector3R diff = Vector3R{zero};
			{
				switch (mode)
				{
				case Pro:
					targetDir = prograde;
					break;
				case Retro:
					targetDir = -prograde;
					break;
				default: break;
				};
				diff = AngularDifference(Vector3R(0, 0, 1), Rotate(orientation, targetDir));
			}

		//	POINT point;
		//	GetCursorPos(&point);
		//	SetCursorPos(200, 200);
		//	Vector2f cursorDelta = Vector2f(point.x - 200, -(point.y - 200));
		//	smoothedCursor = Lerp(cursorDelta / 24.0f, smoothedCursor, exp(-deltaTime.AsFloat()*4.0f));
		//	smoothedCursor.x = Clamp(smoothedCursor.x, -1.0f, 1.0f);
		//	smoothedCursor.y = Clamp(smoothedCursor.y, -1.0f, 1.0f);
						
			Vector3R velAcc{ zero };
			if (mode != Man)
			{
				if (mode == KillRot && Length(input*Vector3R(1, 1, 0)) != 0.0)
				{
					const QuaternionR targetSpin = QuaternionAxisAngle(-Normalize(input*Vector3R(1, 1, 0)), maxAngVel * Real(deltaTime.AsFloat()));
					targetDir = Rotate(Inverse(orientation), Rotate(targetSpin, Rotate(orientation, targetDir)));
				}

				Vector3R targetVel{ zero };
				targetVel.y = -SignSaturate(diff.y * 10.0) * maxAngVel;
				targetVel.x = -SignSaturate(diff.x * 10.0) * maxAngVel;
				targetVel.z = input.z * maxAngVel;
				targetVel = Clamp(targetVel, maxAngVel);
				
				yawControl.SetTarget(targetVel.y);
				pitchControl.SetTarget(targetVel.x);
				rollControl.SetTarget(targetVel.z);
				// velAcc.y += SignSaturate(yawControl.Compute(localAngVel.y, deltaTime.AsFloat())) * maxAcc;
				// velAcc.x += SignSaturate(pitchControl.Compute(localAngVel.x, deltaTime.AsFloat())) * maxAcc;
				// velAcc.z += SignSaturate(rollControl.Compute(localAngVel.z, deltaTime.AsFloat())) * maxAcc;

				auto Filter = [maxAcc](Real x)
				{
				//	return std::pow(Max(0.0, std::abs(x) / maxAcc - 0.1), 4.0) * maxAcc * Sign(x);
					const Real ax = std::abs(x);
					const Real s = Sign(x);
					return Max(0.0, ax - 1.0 / (1000.0f * ax)) * s;
				};
				_yawControl.SetTargetPosition(-diff.y);
				_pitchControl.SetTargetPosition(-diff.x);
				velAcc.y += Filter(_yawControl.Compute(0.0, localAngVel.y, deltaTime.AsFloat()));
				velAcc.x += Filter(_pitchControl.Compute(0.0, localAngVel.x, deltaTime.AsFloat()));

				targetVel = Clamp(velAcc, maxAcc);
			}
			else
			{
				velAcc = input * maxAcc;
			}

		//	POINT point;
		//	GetCursorPos(&point);
		//	Vector3f cursorDelta = Vector3f(point.x - 200.0f, point.y - 200.0f, 0.0f);
		//	smoothedCursor = cursorDelta / 200.0f;

		//	smoothedCursor = Lerp(rotate, smoothedCursor, exp(-deltaTime.AsFloat()*0.25f));
		//	smoothedCursor.x = Clamp(smoothedCursor.x, -1.0f, 1.0f);
		//	smoothedCursor.y = Clamp(smoothedCursor.y, -1.0f, 1.0f);
		//	smoothedCursor.z = Clamp(smoothedCursor.z, -1.0f, 1.0f);
						
			// angularVelocity = angularVelocity + Rotate(Inverse(orientation), Vector3R(rotate) * angAcc);
			localAngVel = localAngVel + velAcc * deltaTime.AsFloat();
			if (Length(localAngVel) > 1.0 / 1024 / 1024)
			{
				const Vector3R worldAngVel = Rotate(Inverse(orientation), localAngVel);
				orientation = orientation * QuaternionAxisAngle(Normalize(worldAngVel), Length(worldAngVel) * Real(deltaTime.AsFloat()));
			//	orientation = orientation * QuaternionAxisAngle(Normalize(angularVelocity), Length(angularVelocity) * Real(deltaTime.AsFloat()));
			}

			rollSlider.SetValue(Radians2Degrees(localAngVel.z));
			yawSlider.SetValue(Radians2Degrees(-localAngVel.y));
			pitchSlider.SetValue(Radians2Degrees(localAngVel.x));
			accSlider.SetValue(Radians2Degrees(Length(velAcc)));
			yawErrorSlider.SetValue(Radians2Degrees(diff.y));
			pitchErrorSlider.SetValue(Radians2Degrees(-diff.x));
			
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			camera.CommitToGL();

			auto LatLonToVector = [](Real lat, Real lon)
			{
				return Vector3R(
					cos(lon) * cos(lat),
					sin(lat),
					sin(lon) * cos(lat));
			};
			
			const Real latStep = Degrees2Radians(10.0);
			const Real lonStep = Degrees2Radians(90.0 / 4.0);

			Vector3R offset;

			glColor4ub(128, 128, 128, 255);			
			glBegin(GL_LINES);
			for (Real lat = -PI / 2; lat < PI / 2; lat += latStep)
			{
				const Real step = 1.0 / 256.0;
				Real lon = 0.0;
				Vector3R p0 = Rotate(orientation, LatLonToVector(lat, lon));
				for (lon = step; lon < PI * 2.0; lon += 1.0 / 256.0)
				{
					Vector3R p1 = Rotate(orientation, LatLonToVector(lat, lon));
					if (p0.z < 0.0 && p1.z < 0.0)
					{
						p0 = p1;
						continue;
					}
					glVertex2f(p0.x, p0.y);
					glVertex2f(p1.x, p1.y);
					p0 = p1;
				}
			}
			for (Real lon = -PI; lon < PI; lon += lonStep)
			{
				Real lat = 0.0;
				const Real step = 1.0 / 256.0;
				Vector3R p0 = Rotate(orientation, LatLonToVector(lat, lon));
				for (lat = step; lat < PI * 2.0; lat += 1.0 / 256.0)
				{
					Vector3R p1 = Rotate(orientation, LatLonToVector(lat, lon));
					if (p0.z < 0.0 && p1.z < 0.0)
					{
						p0 = p1;
						continue;
					}
					glVertex2f(p0.x, p0.y);
					glVertex2f(p1.x, p1.y);
					p0 = p1;
				}
			}
			glEnd();

			{
				const Vector3R forward = Rotate(orientation, Vector3R(1.0, 0.0, 0.0));
				const Vector3R up = Rotate(orientation, Vector3R(0.0, 1.0, 0.0));
				const Vector3R hdir = Normalize(Vector3R(forward.x, 0.0, forward.z));
				const Real heading = Radians2Degrees(std::atan2(hdir.z, hdir.x));
				const Real pitch = Radians2Degrees(std::acos(Dot(forward, hdir)));
				const Real roll = Radians2Degrees(std::acos(Dot(up, Vector3R(0.0, 1.0, 0.0))));

				const float textSz = 0.05f;
				float off = 1.0f - textSz;
				text.size = textSz;
				glColor4ub(255, 255, 255, 255);
				text.OutputText(Vector2f(-1.0f, off), "Heading:%.2f", heading); off -= textSz;
				text.OutputText(Vector2f(-1.0f, off), "Pitch  :%.2f", pitch); off -= textSz;
				text.OutputText(Vector2f(-1.0f, off), "Roll   :%.2f", roll); off -= textSz;
			}

			{
				for (Real lat = -80.0; lat <= 80.0; lat += 20.0)
				{
					if (lat == 0.0)
						continue;
					for (Real lon = 0.0; lon < 360.0; lon += 90.0 / 2.0)
					{
						Vector3R p = Rotate(orientation, LatLonToVector(Degrees2Radians(lat), Degrees2Radians(lon)));
						if (p.z < 0.0)
							continue;
						text.OutputText(Vector2f(p.x, p.y), "%d", int(lat));
					}
				}
			}

			glBegin(GL_LINES);
			// progade / retrograde
			if (mode != Man)
			{
				glColor4ub(32, 192, 255, 255);
				offset = Rotate(orientation, targetDir);
				glDrawMarker(progradeMarker, Vector2f(offset.x, offset.y), IconScale(offset));
				offset = Rotate(orientation, -targetDir);
				glDrawMarker(retrogradeMarker, Vector2f(offset.x, offset.y), IconScale(offset));
			}

			// nose
			glColor4ub(255, 192, 0, 255);
			glDrawMarker(noseMarker, Vector2f(zero), iconSize * 2.0f);

			glEnd();

			glPointSize(2);
			glBegin(GL_POINTS);

			// progade / retrograde
			if (mode != Man)
			{
				glColor4ub(32, 192, 255, 255);
				offset = Rotate(orientation, targetDir);
				glVertex2f(offset.x, offset.y);
				offset = Rotate(orientation, -targetDir);
				glVertex2f(offset.x, offset.y);
			}

			// nose
			glColor4ub(255, 192, 0, 255);
			glVertex2f(0, 0);

			glEnd();

			rollSlider.glDraw();
			yawSlider.glDraw();
			pitchSlider.glDraw();
			accSlider.glDraw();
			if (mode != Man && mode != KillRot)
			{
				yawErrorSlider.glDraw();
				pitchErrorSlider.glDraw();
			}

			protoGL.Swap();
			Sleep(10);
		}
	}


}


namespace
{
	using namespace std;


	typedef complex<float> complexf;
	const float pi = std::acos(-1.0f);
	const float freq = 44100.0f;
	const float invFreq = 1.0f / freq;



	////////////////////////////////////////

	/* Bandlimited synthesis of sawtooth by * leaky integration of a DSF BLIT * * Emanuel Landeholm, March 2002 * emanuel.landeholm@telia.com * * Provided \"as is\". * Free as in Ef Are Ee Ee. */
	// double pi = 3.1415926535897932384626433832795029L; double twopi = 6.2831853071795864769252867665590058L;
	const double twopi = 2.0 * pi;

	/* Leaky integrator/first order lowpass which
	* shapes the impulse train into a nice
	* -6dB/octave spectrum
	*
	* The cutoff frequency needs to be pretty lowish
	* or the sawtooth will suffer phase distortion
	* at the low end.
	*/
	typedef struct
	{
		double x1, y1;
		double a, b;
	} lowpass_t;

	/* initializes a lowpass, sets cutoff/leakiness */
	void init_lowpass(lowpass_t *lp, double cutoff)
	{
		double Omega;
		lp->x1 = lp->y1 = 0.0;
		Omega = atan(pi * cutoff);
		lp->a = -(1.0 - Omega) / (1.0 + Omega);
		// lp->b = (1.0 - lp->b) / 2.0;
		lp->b = (1.0 - Omega) / 2.0;
	}

	double update_lowpass(lowpass_t *lp, double x)
	{
	//	double y;
	//	y = lp->b * (x + lp->x1) - lp->a * lp->y1;
	//	lp->x1 = x;
	//	lp->y1 = y;
	//	return y;

		lp->x1 += x;
		return lp->x1;
	}

	/* dsf blit datatype * */
	typedef struct
	{
		double phase;    /* phase accumulator */
		double aNQ;      /* attenuation at nyquist */
		double curcps;   /* current frequency, updated once per cycle */
		double curper;   /* current period, updated once per cycle */
		lowpass_t leaky; /* leaky integrator */
		double N;        /* # partials */
		double a;        /* dsf parameter which controls roll-off */
		double aN;       /* former to the N */

		double startPhase;
		double syncRate;
		double syncPhase;
	} blit_t;

	/* initializes a blit structure
	 *
	 * The aNQ parameter is the desired attenuation
	 * at nyquist. A low value yields a duller
	 * sawtooth but gets rid of those annoying CLICKS
	 * when sweeping the frequency up real high. |aNQ|
	 * must be strictly less than 1.0! Find a setting
	 * which works for you.
	 *
	 * The cutoff parameter controls the leakiness of
	 * the integrator.
	 */
	void init_blit(blit_t *b, double aNQ, double cutoff, double _phase, double _sync)
	{
		b->phase = _phase;
		b->aNQ = aNQ;
		b->curcps = 0.0;
		b->curper = 0.0;

		b->startPhase = _phase;
		b->syncRate = pow(4.0f, _sync);
		b->syncPhase = 0.0;

		init_lowpass(&b->leaky, cutoff);
	}

	/* Returns a sawtooth computed from a leaky integration
	* of a DSF bandlimited impulse train.
	*
	* cps (cycles per sample) is the fundamental
	* frequency: 0 -> 0.5 == 0 -> nyquist
	*/
	double update_blit(blit_t *b, double cps)
	{
		double P2, beta, Nbeta, cosbeta, n, d, blit, saw;

		b->syncPhase += cps;
		if (b->syncPhase >= 1.0)
		{
			b->syncPhase -= 1.0;
		//	b->phase -= fmod(b->syncRate, 1.0f);
			b->phase -= b->startPhase;
			b->leaky.x1 = b->leaky.y1 = 0.0;
		}

		/* New cycle, update frequency and everything
		* that depends on it
		*/
		if (b->phase >= 1.0)
			b->phase -= 1.0;
		b->curcps = cps * b->syncRate;
		/* this cycle\'s frequency */
		b->curper = 1.0 / (cps * b->syncRate);
		/* this cycle\'s period */
		P2 = b->curper / 2.0;
		b->N = 1.0 + floor(P2);
		/* # of partials incl. dc */
															/* find the roll-off parameter which gives
															* the desired attenuation at nyquist 
															*/
		b->a = pow(b->aNQ, 1.0 / P2);
		b->aN = pow(b->a, b->N);
		
		beta = twopi * b->phase;
		Nbeta = b->N * beta;
		cosbeta = cos(beta);
		/* The dsf blit is scaled by 1 / period to give approximately the same
		* peak-to-peak over a wide range of frequencies.
		*/
		n = 1.0 - b->aN * cos(Nbeta) - b->a * (cosbeta - b->aN * cos(Nbeta - beta));
		d = b->curper * (1.0 + b->a * (-2.0 * cosbeta + b->a));
		b->phase += b->curcps;
		/* update phase */
		blit = n / d - b->curcps; /* This division can only fail if |a| == 1.0
		* Subtracting the fundamental frq rids of DC
		*/
		saw = update_lowpass(&b->leaky, blit); /* shape blit spectrum into a saw */
		return saw - 0.25f;
	}

	///////////////////////////////////////////////////////


	class Saw
	{
	public:
		Saw(float _phase = 0.0f, float sync = 0.0f)
			: phase(_phase)
			, startPhase(_phase)
			, syncRate(pow(4.0f, sync)) {}

		float Sample(float tune)
		{
			const float f = tune * invFreq;
			const float sf = f * syncRate;

			syncPhase += f;
			if (syncPhase >= 1.0f)
			{
				phase = startPhase;
				syncPhase -= 1.0f;
			}

			phase += sf;
			if (phase >= 0.5f)
				phase -= 1.0f;

			return phase;
		}

	private:
		float startPhase;
		float phase;
		float syncPhase = 0.0f;
		float syncRate;
	};



	class Voice
	{
	public:
		Voice(float _tune, float phase, float _wave, float sync, float _amp)
			: tune(_tune)
			, saw0{ -phase * 0.5f, sync }
			, saw1{ phase * 0.5f + 0.5f, sync }
			, saw2{ -phase * 0.5f, sync }
			, saw3{ phase * 0.5f, sync }
			, wave(_wave)
			, amp(_amp) {}

		float Sample()
		{
			const uint numSamples = 8;
			const float invNumSamples = 1.0f / numSamples;

			float output = 0.0f;
			for (uint i = 0; i < numSamples; ++i)
			{
				const float subTune = tune * invNumSamples;
				const float saw = (saw2.Sample(subTune) + saw3.Sample(subTune)) * 0.5f;
				const float pulse = saw0.Sample(subTune) - saw1.Sample(subTune);
				output += Lerp(saw, pulse, wave);
			}
			return output * amp * invNumSamples;
		}

	private:
		Saw saw0;
		Saw saw1;
		Saw saw2;
		Saw saw3;
		float tune;
		float wave;
		float amp;
	};



	const float pitchOfA4 = 57.0f;
	const float freqOfA4 = 440.0f;


	float NoteToFreq(float note)
	{
		const float factor = 12.0f / log(2.0f);
		return (float)(exp((note - pitchOfA4) / factor) * freqOfA4);
	}



	void AnalogWave()
	{
		ProtoGL protoGL;
		protoGL.Initialize(ProtoGLDesc(1280, 720));
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);

		CameraOrtho2D camera(protoGL.GetCanvas(), CameraOrtho2D::Upward);
		camera.SetCamera(Vector2f(zero), 2.5f);

		float phase = 0.0f;
		float wave = 0.0f;
		float sync = 0.0f;

		std::vector<Voice> voices;

		while (protoGL.Update())
		{
			const float knobSp = 0.01f;
			if (GetAsyncKeyState('A') & 0x8000)
				phase += knobSp;
			if (GetAsyncKeyState('Z') & 0x8000)
				phase -= knobSp;
			if (GetAsyncKeyState('S') & 0x8000)
				wave += knobSp;
			if (GetAsyncKeyState('X') & 0x8000)
				wave -= knobSp;
			if (GetAsyncKeyState('D') & 0x8000)
				sync += knobSp;
			if (GetAsyncKeyState('C') & 0x8000)
				sync -= knobSp;
			phase = Clamp(phase, -0.5f, 0.5f);
			wave = Clamp(wave, 0.0f, 1.0f);
			sync = Clamp(sync, 0.0f, 1.0f);

			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			camera.CommitToGL();
						
			const float length = 10000.0f;
			const float tick = 0.01f;
			const float tune = pitchOfA4;

			glLineWidth(1.0f);
			glBegin(GL_LINES);
			glColor4ub(192, 32, 32, 255);
			glVertex2f(0.0f,  0.0f);
			glVertex2f(length * tick, 0.0f);
			glVertex2f(0.0f, -1.0f);
			glVertex2f(0.0f,  1.0f);
			glColor4ub(255, 192, 128, 255);
			glVertex2f(0.0f, 0.5f);
			glVertex2f(length * tick, 0.5f);
			glVertex2f(0.0f, -0.5f);
			glVertex2f(length * tick, -0.5f);
			glVertex2f(0.0f, 1.0f);
			glVertex2f(length * tick, 1.0f);
			glVertex2f(0.0f, -1.0f);
			glVertex2f(length * tick, -1.0f);
			glEnd();

			voices.clear();
			voices.emplace_back(NoteToFreq(tune), phase, wave, sync, 1.0f);
		//	voices.emplace_back(NoteToFreq(tune + 0.25f), phase, wave, 0.25f);
		//	voices.emplace_back(NoteToFreq(tune - 0.25f), phase, wave, 0.25f);
		//	voices.emplace_back(NoteToFreq(tune + 0.5f), phase, wave, 0.25f);
		//	voices.emplace_back(NoteToFreq(tune - 0.5f), phase, wave, 0.25f);

			blit_t b0, b1, b2, b3;
			init_blit(&b0, 1.0f - (1.0f / 1024.0f / 128.0f), 0.001f, -phase * 0.5f, sync);
			init_blit(&b1, 1.0f - (1.0f / 1024.0f / 128.0f), 0.001f,  phase * 0.5f + 0.5f, sync);
			init_blit(&b2, 1.0f - (1.0f / 1024.0f / 128.0f), 0.001f, -phase * 0.5f, sync);
			init_blit(&b3, 1.0f - (1.0f / 1024.0f / 128.0f), 0.001f,  phase * 0.5f, sync);

			glPointSize(2.0f);
			glColor4ub(0, 0, 192, 255);
			// glBegin(GL_POINTS);
			glBegin(GL_LINE_STRIP);
			for (float x = 0.0f; x <= length; x += 1.0f)
			{
				const float cps = NoteToFreq(tune) * invFreq;
				const float bs0 = 2.0f * update_blit(&b0, cps);
				const float bs1 = 2.0f * update_blit(&b1, cps);
				const float bs2 = 2.0f * update_blit(&b2, cps);
				const float bs3 = 2.0f * update_blit(&b3, cps);
				const float saw = (bs2 + bs3) * 0.5f;
				const float pulse = bs0 - bs1;
				const float sample = Lerp(saw, pulse, wave);
				// const float sample = bs0;

				glVertex2f(x * tick, sample);
			}
			glEnd();

			protoGL.Swap();
		}
	}


}


#include "pathtracer/halton.h"


namespace
{



	void LowDiscrepancy()
	{
		std::vector<float> samplesX;
		std::vector<float> samplesY;

		const uint numSamples = 64 * 64;

		Recti bounds;
		bounds.max = Vector2i(64, 64);
		vipt::HaltonSampler sampler(numSamples, bounds);
				
		for (uint i = 0; i < numSamples; ++i)
		{
			sampler.StartPixel(Vector2i(0, 0));
			sampler.SetSample(i);
			Vector2f smp = sampler.Get2D();
			samplesX.push_back(smp.x);
			samplesY.push_back(smp.y);
		//	sampler.StartNextSample();
		}

		ProtoGL protoGL;
		protoGL.Initialize(ProtoGLDesc(1280, 720));
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);

		CameraOrtho2D camera(protoGL.GetCanvas(), CameraOrtho2D::Upward);
		camera.SetCamera(Vector2f(zero), 2.5f);

		while (protoGL.Update())
		{
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			camera.CommitToGL();

			glLineWidth(1.0f);
			glColor4ub(128, 128, 0, 255);
			glBegin(GL_LINE_LOOP);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(1.0f, 0.0f);
			glVertex2f(1.0f, 1.0f);
			glVertex2f(0.0f, 1.0f);
			glEnd();
			glBegin(GL_LINES);
			glColor4ub(192, 192, 192, 255);
			for (uint i = 1; i < 4; ++i)
			{
				glVertex2f(0.0f, i / 4.0f);
				glVertex2f(1.0f, i / 4.0f);
				glVertex2f(i / 4.0f, 0.0f);
				glVertex2f(i / 4.0f, 1.0f);
			}
			glEnd();

			glPointSize(3.0f);
			glColor4ub(255, 0, 0, 255);
			glBegin(GL_POINTS);
			for (uint i = 0; i < samplesX.size(); ++i)
				glVertex2f(samplesX[i], samplesY[i]);
			glEnd();

			protoGL.Swap();
		}
	}


}



namespace
{


typedef vidf::int8   byte;
typedef vidf::uint8  ubyte;
typedef vidf::int16  word;
typedef vidf::uint16 uword;



// d#   - data register d0 to d7
// <d#> - data register as pointer, <d0> to <d7>
enum Opcode
{
	Load,		// load		d#, <d#>	// load data from memory
	Store,		// store	<d#>, d#	// store data into memory
	Move,		// move		d#, d#		// move data from one register to another

	Inc,		// inc		d#			// increment register value by one
	Dec,		// dec		d#			// decrement register value by one
	Add,		// add		d#, d#		// add 
	Sub,		// sub
	MulS,		// muls
	MulU,		// mulu
	DivS,		// divs
	DivU,		// divu

	Or,			// or
	And,		// and
	Shl,		// shl
	Shr,		// shr
	Rol,		// rol
	Ror,		// ror
};



enum Interrupt
{
	Invalid,
	HSync,
	VSync,
};



class Assembler
{
public:
	Assembler(byte* _blockBegin, byte* _blockEnd);

	void Label(const char* name);
	void Add(Opcode op);

private:
	byte* blockBegin;
	byte* blockCur;
	byte* blockEnd;
};



class System
{
public:
	byte LoadByte(uword adrs);
	void StoreByte(uword adrs, byte value);
	word LoadWord(uword adrs);
	void StoreWord(uword adrs, word value);

private:
};



class Processor
{
public:
	void Simulate(System* system, uint clocks);
	void Interrupt(Interrupt interrupt);

private:
	uword reg[8] = { 0 };
	uword sp = 0;
	uword ip = 0;
	union
	{
		ubyte flags = 0;
		struct
		{
			int c : 1;
			int n : 1;
			int z : 1;
		};
	};

	uint doneClocks = 0;
};



Assembler::Assembler(byte* _blockBegin, byte* _blockEnd)
	:	blockBegin(_blockBegin)
	,	blockCur(_blockBegin)
	,	blockEnd(_blockEnd)
{
}



void Assembler::Add(Opcode op)
{
}



byte System::LoadByte(uword adrs)
{
	return 0;
}



void System::StoreByte(uword adrs, byte value)
{
}



word System::LoadWord(uword adrs)
{
	return 0;
}



void System::StoreWord(uword adrs, word value)
{
}



void Processor::Simulate(System* system, uint clocks)
{
	uint targetClk = doneClocks + clocks;
	while (doneClocks <= targetClk)
	{
	}
}



void VintageConsole()
{
	System system;
}


}



namespace
{


const float tickRate = 60.0f;
const float tickDelta = 1.0f / tickRate;

const uint  cellSize = 8;
const uint  cellShf  = 3;
const uint  cellMask = 0x7;
const float gravity  = 250.0f;


void DrawRect(Rectf rect)
{
	glBegin(GL_LINE_LOOP);
	glVertex2f(rect.min.x, rect.min.y);
	glVertex2f(rect.max.x, rect.min.y);
	glVertex2f(rect.max.x, rect.max.y);
	glVertex2f(rect.min.x, rect.max.y);
	glEnd();
}



bool RectRectIntersect(Rectf r0, Rectf r1)
{
	if (r0.max.x < r1.min.x)
		return false;
	if (r0.max.y < r1.min.y)
		return false;
	if (r0.min.x > r1.max.x)
		return false;
	if (r0.min.y > r1.max.y)
		return false;
	return true;
}



Vector2f RectRectSeperation(Rectf r0, Rectf r1)
{
	Vector2f res = Vector2f(0.0f, 0.0f);
	if (r0.max.x > r1.min.x)
		res.x = r1.min.x - r0.max.x;
	if (r0.max.y > r1.min.y)
		res.y = r1.min.y - r0.max.y;
	if (r0.min.x > r1.max.x)
		res.x = r1.max.x - r0.min.x;
	if (r0.min.y > r1.max.y)
		res.y = r1.max.y - r0.min.y;
	if (abs(res.x) < abs(res.y))
		res.y = 0.0f;
	else
		res.x = 0.0f;
	return res;
}



float Interpolate(float current, float target, float speed)
{
	float diff = target - current;
	if (abs(diff) < speed)
		return target;
	return current + speed * Sign(diff);
}



/////////////////////////////////////////////////////////



class Game;


class LevelCollision
{
public:
	LevelCollision();

	bool RectStaticIntersect(Rectf rect) const;
	Vector2f RectStaticSeperation(Rectf rect) const;

	void Draw(const Game& game);

private:
	std::vector<uint> staticMap;
	uint widthMask;
	uint heightMask;
	uint widthShf;
	uint heightShf;
	Rectf rect0;
	Rectf rect1;
};



class Player
{
public:
	Player();

	void Tick(const LevelCollision& collision, Game* game);
	void Draw();

private:
	void UpdateRect();

private:
	Vector2f position;
	Rectf    rect;
	float    moveSpeed;
	float    fallSpeed;
	uint     jumpingTicks;
	bool     inAir;
	bool     jumping;
};



class Game
{
public:
	Game(CanvasPtr canvas);
	void Tick();
	void Draw();

	const CameraOrtho2D& GetCamera() const { return camera; }
	void SetCameraCenter(Vector2f center) { camera.SetCamera(center, 200.0f); }

private:
	LevelCollision levelCollision;
	Player player;
	CameraOrtho2D camera;
};



/////////////////////////////////////////////////////////


LevelCollision::LevelCollision()
{
	rect0.min = Vector2f(-120.0f, 32.0f);
	rect0.max = Vector2f(120.0f, 64.0f);
	rect1.min = Vector2f(100.0f, -32.0f);
	rect1.max = Vector2f(120.0f, 32.0f);
		
	widthShf = 11;
	heightShf = 9;
	widthMask = (1 << widthShf) - 1;
	heightMask = (1 << heightShf) - 1;
	staticMap.resize((1 << widthShf) * (1 << heightShf));

	for (uint x = 0; x < 20; ++x)
		staticMap[4 * (1 << widthShf) + x] = ~0u;
	for (uint y = 0; y < 4; ++y)
	{
		for (uint x = 17; x < 20; ++x)
			staticMap[y * (1 << widthShf) + x] = ~0u;
	}
}


bool LevelCollision::RectStaticIntersect(Rectf rect) const
{
	if (RectRectIntersect(rect0, rect))
		return true;
	if (RectRectIntersect(rect1, rect))
		return true;
	return false;
}



Vector2f LevelCollision::RectStaticSeperation(Rectf rect) const
{
	Vector2f res{ 0.0f, 0.0f };

	Recti testRect;
	testRect.min.x = Max(int(rect.min.x) >> cellShf, 0);
	testRect.min.y = Max(int(rect.min.y) >> cellShf, 0);
	testRect.max.x = Min(int(rect.max.x) >> cellShf, (1 << widthMask) - 1);
	testRect.max.y = Min(int(rect.max.y) >> cellShf, (1 << heightMask) - 1);

	for (int y = testRect.min.y; y < testRect.max.y; ++y)
	{
		for (int x = testRect.min.x; x < testRect.max.x; ++x)
		{
			int idx = y * (1 << widthShf) + x;
			if (staticMap[idx] == ~0u)
			{
				Rectf staticRect;
				rect.min.x = x * cellSize;
				rect.min.y = y * cellSize;
				rect.max.x = rect.min.x + cellSize;
				rect.max.y = rect.min.y + cellSize;
				res = res + RectRectSeperation(rect, staticRect);
			}
		}
	}

	return res;

//	return
//		RectRectSeperation(rect, rect0) + 
//		RectRectSeperation(rect, rect1);
}



void LevelCollision::Draw(const Game& game)
{
	glLineWidth(1.0f);

	Vector2f camCenter = game.GetCamera().GetCenter();
	float camSizeY = game.GetCamera().GetSize() * 0.5f;
	float camSizeX = camSizeY * game.GetCamera().GetAspectRation();
	Recti camRect;
	camRect.min.x = Max(int(camCenter.x - camSizeX) >> cellShf, 0);
	camRect.min.y = Max(int(camCenter.y - camSizeY) >> cellShf, 0);
	camRect.max.x = Min(int(camCenter.x + camSizeX) >> cellShf, (1 << widthMask) - 1);
	camRect.max.y = Min(int(camCenter.y + camSizeY) >> cellShf, (1 << heightMask) - 1);

	glColor4ub(192, 192, 192, 255);
	Rectf mapRect;
	mapRect.min.x = 0;
	mapRect.min.y = 0;
	mapRect.max.x = (1 << widthShf) * cellSize;
	mapRect.max.y = (1 << heightShf) * cellSize;
	DrawRect(mapRect);

	glColor4ub(192, 16, 16, 255);
	for (int y = camRect.min.y; y < camRect.max.y; ++y)
	{
		for (int x = camRect.min.x; x < camRect.max.x; ++x)
		{
			int idx = y * (1 << widthShf) + x;
			if (staticMap[idx] == ~0u)
			{
				Rectf rect;
				rect.min.x = x * cellSize;
				rect.min.y = y * cellSize;
				rect.max.x = rect.min.x + cellSize;
				rect.max.y = rect.min.y + cellSize;
				DrawRect(rect);
			}
		}
	}
}



/////////////////////////////////////////////////////////


Player::Player()
{
	position = Vector2f(32.0f, 0.0f);
	moveSpeed = 0.0f;
	fallSpeed = 0.0f;
	inAir = true;
	jumpingTicks = 0;
	UpdateRect();
}


void Player::Tick(const LevelCollision& collision, Game* game)
{
	const bool rightBtn = GetAsyncKeyState(VK_RIGHT) & 0x8000;
	const bool leftBtn = GetAsyncKeyState(VK_LEFT) & 0x8000;
	const bool jumpBtn = GetAsyncKeyState('A') & 0x8000;

	float accel = 60.0f;
	float decel = 60.0f;

	if (!inAir)
	{
		jumpingTicks = 0;
		jumping = false;

		accel = 160.0f;
		decel = 80.0f;

		if (jumpBtn)
		{
			jumping = true;
			inAir = true;
		}
	}

	if (rightBtn)
		moveSpeed = Interpolate(moveSpeed, 70.0f, tickDelta * accel);
	else if (leftBtn)
		moveSpeed = Interpolate(moveSpeed, -70.0f, tickDelta * accel);
	else
		moveSpeed = Interpolate(moveSpeed, 0.0f, tickDelta * decel);

	if (jumping && jumpBtn)
	{
		jumpingTicks++;
		if (jumpingTicks > 18)
			jumping = false;
	}
	else
	{
		jumping = false;
	}
	if (jumping)
		fallSpeed = -120.0f;
		
	position.x += tickDelta * moveSpeed;
	position.y += tickDelta * fallSpeed;
	UpdateRect();

	const Vector2f staticSep = collision.RectStaticSeperation(rect);
	if (staticSep.y == 0.0f)
	{
		if (fallSpeed < 0.0f)
			fallSpeed += tickDelta * gravity;
		else
			fallSpeed += tickDelta * gravity * 3.0f;
		inAir = true;
	}
	else
	{
		fallSpeed = 0.0f;
		inAir = false;
	}
	if (staticSep.x != 0.0f)
		moveSpeed = 0.0f;
	position = position + staticSep;
	UpdateRect();

	game->SetCameraCenter((rect.min + rect.max) * 0.5f);
}


void Player::UpdateRect()
{
	rect.min.x = position.x - cellSize * 1.25f;
	rect.max.x = position.x + cellSize * 1.25f;
	rect.min.y = position.y - cellSize * 3.75f;
	rect.max.y = position.y;
}


void Player::Draw()
{
	glLineWidth(1.0f);
	glColor4ub(255, 128, 32, 255);
	DrawRect(rect);
}


/////////////////////////////////////////////////////////


Game::Game(CanvasPtr canvas)
	: camera(canvas, CameraOrtho2D::Downward)
{
	SetCameraCenter(Vector2f(zero));
}


void Game::Tick()
{
	player.Tick(levelCollision, this);
}


void Game::Draw()
{
	camera.CommitToGL();

	Rectf platformRect;
	platformRect.min = Vector2f(-120.0f, 32.0f);
	platformRect.max = Vector2f(120.0f, 64.0f);
		
	levelCollision.Draw(*this);
	player.Draw();
}



/////////////////////////////////////////////////////////



void Platformer()
{
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	Game game{ protoGL.GetCanvas() };

	TimeCounter timer;
	Time tickCount;

	while (protoGL.Update())
	{
		Time delta = timer.GetElapsed();
		tickCount += delta;
		if (tickCount.AsFloat() >= tickDelta)
		{
			tickCount -= Time(tickDelta);
			game.Tick();
		}

		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		game.Draw();

		protoGL.Swap();
	}
}



}



namespace vidf::dx11
{
};


namespace
{


template<typename T>
struct Complex
{
	Complex() = default;

	explicit Complex(Zero)
		: r(T(1)), i(0) {}

	Complex(const Complex<T>& v)
		: r(v.r), i(v.i) {}

	template<typename U>
	explicit Complex(U _r, U _i)
		: r((T)_r), i((T)_i) {}

	template<typename U>
	explicit Complex(const Vector2<U>& v)
		: r((T)v.r), i((T)v.i) {}

	T& operator[] (int idx) { return (&x)[idx]; }
	T operator[] (int idx) const { return (&x)[idx]; }

	T r, i;
};

typedef Complex<float> Complexf;

template<typename T>
inline Complex<T> operator * (const Complex<T>& lhv, const Complex<T>& rhv)
{
	return Complex<T>(
		lhv.r * rhv.r - lhv.i * rhv.i,
		lhv.r * rhv.i + lhv.i * rhv.r);
}
template<typename T>
inline Vector2<T> operator * (const Complex<T>& lhv, const Vector2<T>& rhv)
{
	return Vector2<T>(
		lhv.r * rhv.x - lhv.i * rhv.y,
		lhv.r * rhv.y + lhv.i * rhv.x);
}
template<typename T, typename U>
inline Complex<T> operator * (const Complex<T>& lhv, U rhv)
{
	return Complex<T>(lhv.r*(T)rhv, lhv.i*(T)rhv);
}
template<typename T, typename U>
inline Complex<T> operator * (U rhv, const Complex<T>& lhv)
{
	return lhv * (T)rhv;
}

template<typename T, typename U>
inline Complex<T> operator / (const Complex<T>& lhv, U rhv)
{
	return Complex<T>(lhv.r / (T)rhv, lhv.i / (T)rhv);
}
template<typename T, typename U>
inline Complex<T> operator / (U rhv, const Complex<T>& lhv)
{
	return Complex<T>((T)lhv / rhv.r, (T)lhv / rhv.i);
	return lhv * (T)rhv;
}

template<typename T>
inline float Norm(const Complex<T>& v)
{
	return std::sqrt(v.r*v.r + v.i*v.i);
}

template<typename T>
inline Complex<T> Normalize(const Complex<T>& v)
{
	float m = T(1) / Norm(v);
	return Complex<T>(
		v.r * m,
		v.i * m);
}

template<typename T>
inline Complex<T> Log(const Complex<T>& v)
{
	return Complex<T>(
		std::log(Norm(v)),
		std::atan2(v.i, v.r));
}

template<typename T>
inline Complex<T> ComplexFromAngle(T angle)
{
	return Complex<T>(cos(angle), sin(angle));
}



template<typename T>
inline Quaternion<T> Exp(Vector3<T> vec)
{
	const T n = Length(vec);
	Quaternion<T> q;
	if (std::abs(n) > 1.0f / 1024.0f / 1024.0f)
	{
		const T ins = T(1) / n * std::sin(n);
		q.x = vec.x * ins;
		q.y = vec.y * ins;
		q.z = vec.z * ins;
		q.w = std::cos(n);
	}
	else
		q = Quaternion<T>(zero);
	return q;
}



template<typename T, uint lutsize = 1024>
class Clothoid
{
public:
	Clothoid()
		: B(T(1))
	{
		if (lutPoint.empty())
			lutPoint.push_back(Vector2<T>{zero});
	}

	Clothoid(T _B)
		: B(_B)
	{
		if (lutPoint.empty())
			lutPoint.push_back(Vector2<T>{zero});
	}

	Vector2<T> GetPoint(T t) const
	{
		return NormalClothoidPoint(std::abs(t * B)) / std::abs(B) * Vector2<T>(T(1), Sign(B));
	}

	Complex<T> GetComplex(T t) const
	{
		Complex<T> c = NormalClothoidComplex(t * B);
		if (B < 0.0f)
			c.i = -c.i;
		return c;
	}

private:
	Vector2<T> NormalClothoidPoint(T t) const
	{
		const float dU = 1.0f / lutsize;
		UpdateLut(t);
		const uint idx = uint(t * lutsize);
		const float u = idx * dU;
		Vector2<T> p = lutPoint[idx];
		p.x += cos(t * t) * (t - u);
		p.y += sin(t * t) * (t - u);
		return p;
	}

	Complex<T> NormalClothoidComplex(T t) const
	{
		Complex<T> c;
		c.r = cos(t * t);
		c.i = sin(t * t);
		return c;
	}

	static void UpdateLut(T t)
	{
		const float dU = 1.0f / lutsize;
		if (lutPoint.size() <= t * lutsize)
		{
			Vector2<T> p = lutPoint.back();
			float u = lutPoint.size() * dU;
			for (; u <= t; u += dU)
			{
				p.x += cos(u * u) * dU;
				p.y += sin(u * u) * dU;
				lutPoint.push_back(p);
			}
		}
	}

private:
	static std::vector<Vector2<T>> lutPoint;
	T B;
};

template<typename T, uint lutsize>
std::vector<Vector2<T>> Clothoid<T, lutsize>::lutPoint;



class Track
{
public:
	Track()
	{
		trackPositions.push_back(Vector3f(zero));
		trackOrientations.push_back(Quaternionf(zero));
		spinePositions.push_back(Vector3f(zero));
		spineOrientations.push_back(Quaternionf(zero));
	}

	void AdvanceEuler(const Vector3f curvature, const Vector3f offset, const float camber, const float dT)
	{
		const Vector3f prevPosition = spinePositions.back();
		const Vector3f prevTrackPosition = trackPositions.back();

		const Quaternionf orientation =
			Quaternionf(0.0f, 0.0f, complexes[2].i, complexes[2].r) *
			Quaternionf(0.0f, complexes[1].i, 0.0f, complexes[1].r) *
			Quaternionf(complexes[0].i, 0.0f, 0.0f, complexes[0].r);
		spineOrientations.push_back(orientation);

		const Vector3f position = spinePositions.back() + Rotate(orientation, Vector3f(dT, 0.0f, 0.0f));
		spinePositions.push_back(position);

		Vector3f offsetPosition = position + Rotate(orientation, offset);
		offsetPosition.z += (-std::cos(Log(complexes[0]).i) * 0.5f + 0.5f) * camber;
	//	offsetPosition.z += std::abs(std::sin(Log(complexes[0]).i)) * camber;
		trackPositions.push_back(offsetPosition);

		const Vector3f spineMovedir = Normalize(position - prevPosition);
		const Vector3f trackMovedir = Normalize(offsetPosition - prevTrackPosition);
		const Quaternionf trackOrientation = QuaternionVectorDirection(spineMovedir, trackMovedir) * orientation;
		trackOrientations.push_back(trackOrientation);

		complexes[0] = ComplexFromAngle(curvature.x * dT) * complexes[0];
		complexes[1] = ComplexFromAngle(curvature.y * dT) * complexes[1];
		complexes[2] = ComplexFromAngle(curvature.z * dT) * complexes[2];

		distance += dT;
	}
	
	void AdvanceQuaternion(Vector3f curvature, float dT)
	{
		const Quaternionf orientation = spineOrientations.back() * Exp(0.5f * Rotate(spineOrientations.back(), curvature) * dT);
		spineOrientations.push_back(orientation);

		const Vector3f position = spinePositions.back() + Rotate(orientation, Vector3f(dT, 0.0f, 0.0f));
		spinePositions.push_back(position);

		trackPositions.push_back(spinePositions.back());
		trackOrientations.push_back(spineOrientations.back());

		distance += dT;
	}

	uint        GetNumPoints() const { return trackPositions.size(); }
	Vector3f    GetSpinePosition(uint idx) const { return spinePositions[idx]; }
	Quaternionf GetSpineOrientation(uint idx) const { return spineOrientations[idx]; }
	Vector3f    GetTrackPosition(uint idx) const { return trackPositions[idx]; }
	Quaternionf GetTrackOrientation(uint idx) const { return trackOrientations[idx]; }

private:
	std::vector<Vector3f>    spinePositions;
	std::vector<Quaternionf> spineOrientations;
	std::vector<Vector3f>    trackPositions;
	std::vector<Quaternionf> trackOrientations;
	Complexf complexes[3] = { Complexf(zero), Complexf(zero), Complexf(zero) };
	float distance = 0.0f;
};



struct Segment
{
	Segment() = default;
	Segment(float _length, float _value)
		: length(_length)
		, value(_value) {}
	float length = 1.0f;
	float value  = 0.0f;
};


class Graph
{
public:
	void Append(const Segment& segment)
	{
		segments.push_back(segment);
		segmentStart.push_back(length);
		length += segment.length;
	}

	void Append(float rate, float amount)
	{
		float current = segments.empty() ? 0.0f : segments.back().value;
		float length = (std::sqrt(2.0f) * std::sqrt(amount)) / std::sqrt(std::abs(rate));
		Append(Segment(length, current + rate * length));
	}

	void Append(float amount)
	{
		float current = segments.empty() ? 0.0f : segments.back().value;
		float length = amount / current;
		Append(Segment(length, current));
	}

	float Evaluate(float t) const
	{
		if (segments.empty() || t < 0.0f)
			return 0.0f;
		if (t > length)
			return segments.back().value;

		uint idx = 0;
		for (; idx < segmentStart.size() - 1 && segmentStart[idx + 1] <= t; ++idx);

		const float startValue = idx != 0 ? segments[idx - 1].value : 0.0f;
		const float endValue   = segments[idx].value;

		const float u = (t - segmentStart[idx]) / segments[idx].length;
		const float result = Lerp(startValue, endValue, u);

		return result;
	}

	float GetLength() const { return length; }

private:
	std::vector<Segment> segments;
	std::vector<float> segmentStart;
	float length = 0.0f;
};



class Section
{
public:
	Graph& GetYaw()   { return yaw; }
	Graph& GetPitch() { return pitch; }
	Graph& GetRoll()  { return roll; }

	const Graph& GetYaw() const   { return yaw; }
	const Graph& GetPitch() const { return pitch; }
	const Graph& GetRoll() const  { return roll; }

	void CreateTrack(Track& track, const Vector3f offset, const float camber, const float dT) const
	{
		const float length = Max(yaw.GetLength(), Max(pitch.GetLength(), roll.GetLength()));
		for (float t = 0.0f; t < length; t += dT)
		{
			Vector3f curvature;
			curvature.x = roll.Evaluate(t);
			curvature.y = pitch.Evaluate(t);
			curvature.z = yaw.Evaluate(t);
			track.AdvanceEuler(curvature, offset, camber, dT);
		//	track.AdvanceQuaternion(curvature, dT);
		}
	}

private:
	Graph yaw;
	Graph pitch;
	Graph roll;
};



void DrawGuideLine(Vector3f point, Vector3f dir)
{
	glLineWidth(1.0f);
	glColor4ub(255, 196, 64, 255);
	glBegin(GL_LINES);
	glVertex(point + dir * 1024.0f);
	glVertex(point - dir * 1024.0f);
	glEnd();
}



void DrawTrack(const Track& track)
{
	glLineWidth(2.0f);
	glColor4ub(128, 255, 32, 255);
	glBegin(GL_LINE_STRIP);
	for (uint i = 0; i < track.GetNumPoints(); ++i)
	{
		const Vector3f position = track.GetTrackPosition(i);
		const Quaternionf orientation = track.GetTrackOrientation(i);
		Vector3f point = position + Rotate(orientation, Vector3f(0.0f, 1.0f / 8.0f, 0.0f));
		glVertex3f(point.x, point.y, point.z);
	}
	glEnd();
	glBegin(GL_LINE_STRIP);
	for (uint i = 0; i < track.GetNumPoints(); ++i)
	{
		const Vector3f position = track.GetTrackPosition(i);
		const Quaternionf orientation = track.GetTrackOrientation(i);
		Vector3f point = position - Rotate(orientation, Vector3f(0.0f, 1.0f / 8.0f, 0.0f));
		glVertex3f(point.x, point.y, point.z);
	}
	glEnd();

	glLineWidth(1.0f);
	glBegin(GL_LINE_STRIP);
	glColor4ub(98, 98, 98, 255);
	for (uint i = 0; i < track.GetNumPoints(); ++i)
	{
		const Vector3f position = track.GetSpinePosition(i);
		glVertex3f(position.x, position.y, 0.0f);
	}
	glEnd();

	glLineWidth(2.0f);
	glBegin(GL_LINE_STRIP);
	glColor4ub(128, 98, 0, 255);
	for (uint i = 0; i < track.GetNumPoints(); ++i)
	{
		const Vector3f position = track.GetSpinePosition(i);
		glVertex3f(position.x, position.y, position.z);
	}
	glEnd();

	glLineWidth(1.0f);
	glColor4ub(32, 198, 0, 255);
	glBegin(GL_LINES);
	for (uint i = 0; i < track.GetNumPoints(); i += 64)
	{
		const Vector3f position = track.GetTrackPosition(i);
		const Quaternionf orientation = track.GetTrackOrientation(i);
		Vector3f p0 = position - Rotate(orientation, Vector3f(0.0f, 1.0f / 8.0f, 0.0f));
		Vector3f p1 = position + Rotate(orientation, Vector3f(0.0f, 1.0f / 8.0f, 0.0f));
		glVertex3f(p0.x, p0.y, p0.z);
		glVertex3f(p1.x, p1.y, p1.z);
	}
	glEnd();
	/*
	glColor4ub(98, 98, 98, 255);
	glLineWidth(1.0f);
	glBegin(GL_LINES);
	for (uint i = 0; i < track.GetNumPoints(); i += 64)
	{
		const Vector3f position = track.GetSpinePosition(i);
		glVertex3f(position.x, position.y, 0.0f);
		glVertex3f(position.x, position.y, position.z);
	}
	glEnd();
	*/
}



void ClothoidTest()
{
	const float dT = 1.0f / 1024.0f;
//	const float dT = 1.0f / 16.0f;

	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera2D(protoGL.GetCanvas(), CameraOrtho2D::Upward);
	camera2D.SetCamera(Vector2f(zero), 10.0f);

	OrbitalCamera camera3D(protoGL.GetCanvas());
	camera3D.SetPerspective(1.4f, 0.01f, 1024.0f);

	Section section;
//	section.GetYaw().Append(Segment(2.0f, -1.0f));
//	section.GetYaw().Append(Segment(1.0f, -1.0f));
//	section.GetYaw().Append(Segment(2.0f,  0.0f));
//	section.GetRoll().Append(Segment(1.0f,  pi * 0.25f));
//	section.GetRoll().Append(Segment(1.0f,  0.0f));
//	section.GetRoll().Append(Segment(1.0f,  0.0f));
//	section.GetRoll().Append(Segment(1.0f,  pi * 0.25f));
//	section.GetRoll().Append(Segment(1.0f,  0.0f));
//	section.GetPitch().Append(Segment(1.0f, -0.05f));
//	section.GetPitch().Append(Segment(1.0f,  0.0f));
//	section.GetPitch().Append(Segment(1.0f,  0.0f));
//	section.GetPitch().Append(Segment(1.0f,  0.05f));
//	section.GetPitch().Append(Segment(1.0f,  0.0f));

//	section.GetRoll().Append(Segment(1.0f, pi * 0.25f));
//	section.GetRoll().Append(Segment(2.0f, pi * 0.25f));
//	section.GetRoll().Append(Segment(3.0f, 0.0f));

	///

	section.GetPitch().Append(Segment(4.0f, -pi * 0.25f));
//	section.GetPitch().Append(Segment(4.0f, 0.0f));

//	section.GetYaw().Append(Segment(1.0f, -pi * 0.05f));
//	section.GetYaw().Append(Segment(2.0f, pi * 0.05f));
//	section.GetYaw().Append(Segment(1.0f, 0.0f));
//	section.GetYaw().Append(Segment(1.0f, -pi * 0.05f));
//	section.GetYaw().Append(Segment(2.0f, pi * 0.05f));
//	section.GetYaw().Append(Segment(1.0f, 0.0f));

	section.GetRoll().Append(Segment(1.0f, pi * 0.0075f));
	section.GetRoll().Append(Segment(1.0f, 0.0f));
	section.GetRoll().Append(Segment(1.0f, pi * 0.0075f));
	section.GetRoll().Append(Segment(1.0f, 0.0f));
//	section.GetRoll().Append(Segment(1.0f, -pi * 0.005f));
//	section.GetRoll().Append(Segment(2.0f,  pi * 0.005f));
//	section.GetRoll().Append(Segment(1.0f, 0.0f));

	///

	Track track;
//	section.CreateTrack(track, Vector3f(0.0f, 0.0f, -1.0f / 8.0f), 0.0f, dT);
	section.CreateTrack(track, Vector3f(0.0f, 0.0f, -1.0f / 8.0f), 0.0f, dT);
//	section.CreateTrack(track, Vector3f(zero), 1.0f / 8.0f, dT);

	TimeCounter timer;

	float t = 0.0f;

	while (protoGL.Update())
	{
		Vector3f guidePos0;
		Quaternionf guideOri0;
		Vector3f guidePos1;
		Quaternionf guideOri1;
		Vector3f guidePos2;
		Quaternionf guideOri2;

		Track track;
		{
			Section section;
			
		//	section.GetYaw().Append(Segment(1.0f, pi * 0.05f));
		//	section.GetYaw().Append(Segment(2.0f, pi * 0.05f));
		//	section.GetYaw().Append(Segment(3.0f, 0.0f));

			// section.GetYaw().Append(Segment(0.01f, pi * 0.05f));
			// section.GetYaw().Append(Segment(1.0f / 0.05f * 0.5f, pi * 0.05f));

		//	section.GetYaw().Append(Segment(1.0f / 0.05f * 0.25f, pi * 0.05f));
		//	section.GetYaw().Append(Segment(1.0f / 0.05f * 0.25f, 0.0f));

			const float spin = 0.05f;
			const float invSpin = 1.0f / spin;
			// section.GetYaw().Append(Segment(invSpin * 0.175f, pi * spin));
			// section.GetYaw().Append(Segment((1.0f / 0.05f) * 0.25f, pi * 0.05f));
			// section.GetYaw().Append(Segment(1.0f / 0.05f * 0.25f * 0.5f, 0.0f));

			section.GetYaw().Append(spin, pi * 0.25f);
			section.GetYaw().Append(pi * 0.5f);
			section.GetYaw().Append(-spin, pi * 0.25f);

		//	section.CreateTrack(track, Vector3f(0.0f, 0.0f, -1.0f / 8.0f), 0.0f, dT);
			section.CreateTrack(track, Vector3f(0.0f, 0.0f, 0.0f), 0.0f, dT);
			/*
			guidePos0 = track.GetSpinePosition(1.0f / dT);
			guideOri0 = track.GetSpineOrientation(1.0f / dT);
			guidePos1 = track.GetSpinePosition(3.0f / dT);
			guideOri1 = track.GetSpineOrientation(3.0f / dT);
			*/
			guidePos2 = track.GetSpinePosition(track.GetNumPoints() - 1);
			guideOri2 = track.GetSpineOrientation(track.GetNumPoints() - 1);
			
			/*
			section.GetYaw().Append(Segment(1.0f, pi * 0.05f));
			section.GetYaw().Append(Segment(3.0f, pi * 0.05f));
			section.GetYaw().Append(Segment(1.0f, 0.0f));

			section.CreateTrack(track, Vector3f(0.0f, 0.0f, -1.0f / 8.0f), 0.0f, dT);

			guidePos0 = track.GetSpinePosition(1.0f / dT);
			guideOri0 = track.GetSpineOrientation(1.0f / dT);
			guidePos1 = track.GetSpinePosition(4.0f / dT);
			guideOri1 = track.GetSpineOrientation(4.0f / dT);
			guidePos2 = track.GetSpinePosition(track.GetNumPoints() - 1);
			guideOri2 = track.GetSpineOrientation(track.GetNumPoints() - 1);
			*/
		}

		Time deltaTime = timer.GetElapsed();

		if (GetAsyncKeyState('A') & 0x8000)
			t += deltaTime.AsFloat() * 200.0f;
		if (GetAsyncKeyState('Z') & 0x8000)
			t -= deltaTime.AsFloat() * 200.0f;

		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

	//	camera2D.CommitToGL();

	//	t = Clamp(t, 0.0f, track.GetNumPoints() - 1.0f);
	//	camera3D.SetCamera(
	//		track.GetTrackPosition(t) + Rotate(track.GetTrackOrientation(t), Vector3f(0.0f, 0.0f, 0.05f)),
	//		track.GetTrackOrientation(t),
	//		0.1f);
		camera3D.Update(deltaTime);
		camera3D.CommitToGL();
				
		glLineWidth(1.0f);
		glBegin(GL_LINES);
		glColor4ub(232, 232, 232, 255);
		for (int i = -1024; i < 1024; ++i)
		{
			glVertex2f(-1024.0f, i); glVertex2f(1024.0f, i);
			glVertex2f(i, -1024.0f); glVertex2f(i, 1024.0f);
		}
		glColor4ub(196, 196, 196, 255);
		for (int i = -1000; i < 1000; i += 10)
		{
			glVertex2f(-1024.0f, i); glVertex2f(1024.0f, i);
			glVertex2f(i, -1024.0f); glVertex2f(i, 1024.0f);
		}
		glColor4ub(64, 64, 64, 255);
		glVertex2f(-1024.0f, 0.0f); glVertex2f(1024.0f, 0.0f);
		glVertex2f(0.0f, -1024.0f); glVertex2f(0.0f, 1024.0f);
		glEnd();
		
		DrawGuideLine(Vector3f(zero), Vector3f(0, 1, 0));
		DrawGuideLine(Vector3f(zero), Vector3f(1, 0, 0));
		DrawGuideLine(guidePos0, Rotate(guideOri0, Vector3f(0, 1, 0)));
		DrawGuideLine(guidePos1, Rotate(guideOri1, Vector3f(0, 1, 0)));
		DrawGuideLine(guidePos2, Rotate(guideOri2, Vector3f(0, 1, 0)));
		DrawGuideLine(guidePos2, Rotate(guideOri2, Vector3f(1, 0, 0)));

		DrawTrack(track);
		
		protoGL.Swap();
	}
}


}



using namespace std;



template<typename It>
void glLineStrip2f(It begin, It end)
{
	glBegin(GL_LINE_STRIP);
	for (; begin != end; ++begin)
		glVertex2f(begin->x, begin->y);
	glEnd();
}



template<typename It>
void glPoints2f(It begin, It end)
{
	glBegin(GL_POINTS);
	for (; begin != end; ++begin)
		glVertex2f(begin->x, begin->y);
	glEnd();
}



void glDrawGrid()
{
	glLineWidth(1.0f);
	glBegin(GL_LINES);
	glColor4ub(232, 232, 232, 255);
	for (int i = -1024; i < 1024; ++i)
	{
		glVertex2f(-1024.0f, i); glVertex2f(1024.0f, i);
		glVertex2f(i, -1024.0f); glVertex2f(i, 1024.0f);
	}
	glColor4ub(196, 196, 196, 255);
	for (int i = -1000; i < 1000; i += 10)
	{
		glVertex2f(-1024.0f, i); glVertex2f(1024.0f, i);
		glVertex2f(i, -1024.0f); glVertex2f(i, 1024.0f);
	}
	glColor4ub(64, 64, 64, 255);
	glVertex2f(-1024.0f, 0.0f); glVertex2f(1024.0f, 0.0f);
	glVertex2f(0.0f, -1024.0f); glVertex2f(0.0f, 1024.0f);
	glEnd();
}



void RungeKutta()
{
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera2D(protoGL.GetCanvas(), CameraOrtho2D::Upward);
	camera2D.SetCamera(Vector2f(zero), 10.0f);

	deque<Vector2f> integral;
	deque<Vector2f> eulerIntegral;
	deque<Vector2f> rungeKuttaIntegral;

	/*
	auto Derivative = [](double y, double t)
	{
		return sin(t) * sin(t) * y + 0.2;
	};
	{
		const double dT = 1.0 / 1024.0 / 256.0;
		double y = 0.0;
		uint skip = 0;
		for (double x = 0; x <= 5; x += dT)
		{
			if (skip++ % 32 == 0)
				integral.push_back(Vector2f(x, y));
			y = y + Derivative(y, x) * dT;
		}
	}

	{
		const float dT = 0.5f;
		float y = 0.0f;
		for (float x = 0; x <= 5; x += dT)
		{
			eulerIntegral.push_back(Vector2f(x, y));
			y = y + Derivative(y, x) * dT;
		}
	}

	{
		const float dT = 0.5f;
		float y = 0.0f;
		for (float x = 0; x <= 5; x += dT)
		{
			rungeKuttaIntegral.push_back(Vector2f(x, y));
			const float k1 = dT * Derivative(y, x);
			const float k2 = dT * Derivative(y + k1 * 0.5f, x + dT * 0.5f);
			const float k3 = dT * Derivative(y + k2 * 0.5f, x + dT * 0.5f);
			const float k4 = dT * Derivative(y + k3, x + dT);
			y = y + (k1 + 2 * k2 + 2 * k3 + k4) / 6.0;
		}
	}
	*/

	{
		{
			const double dT = 1.0 / 1024.0 / 256.0;
			Vector2<double> p = Vector2<double>(2, 0);
			Vector2<double> v = Vector2<double>(0.0, 0.5);
			uint skip = 0;
			for (double t = 0; t <= 10; t += dT)
			{
				if (skip++ % 32 == 0)
					integral.push_back(Vector2f(p));
				const double ir2 = 1 / Dot(p, p);
				const Vector2<double> a = -Normalize(p) * ir2;
				v = v + a * dT;
				p = p + v * dT;
			}
		}
		
		{
			const float dT = 0.15f;
			Vector2f p = Vector2f(2, 0);
			Vector2f v = Vector2f(0.0, 0.5);
			for (double t = 0; t <= 10; t += dT)
			{
				eulerIntegral.push_back(p);
				const double ir2 = 1 / Dot(p, p);
				const Vector2f a = -Normalize(p) * ir2;
				v = v + a * dT;
				p = p + v * dT;
			}
		}
		
		{
			const float dT = 0.15f;
			Vector2f p = Vector2f(2, 0);
			Vector2f v = Vector2f(0.0, 0.5);
			for (double t = 0; t <= 10; t += dT)
			{
				rungeKuttaIntegral.push_back(p);
				const double ir2 = 1 / Dot(p, p);
				const Vector2f a = -Normalize(p) * ir2;
				const Vector2f v1 = v;
				const Vector2f v2 = v + a * dT * 0.5;
				const Vector2f v3 = v + a * dT;
				const Vector2f k1 = dT * v1;
				const Vector2f k2 = dT * (k1 * 0.5f + v2);
				const Vector2f k3 = dT * (k2 * 0.5f + v2);
				const Vector2f k4 = dT * (k3 + v3);
				v = v3;
				p = p + (k1 + 2 * k2 + 2 * k3 + k4) / 6.0;
			}
		}
	}

	while (protoGL.Update())
	{
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera2D.CommitToGL();

		glDrawGrid();

		glLineWidth(2.0f);
		glColor4ub(255, 198, 128, 255);
		glLineStrip2f(integral.begin(), integral.end());
		
		glLineWidth(1.0f);
		glPointSize(4.0f);
		glColor4ub(198, 64, 0, 255);
		glLineStrip2f(eulerIntegral.begin(), eulerIntegral.end());
		glPoints2f(eulerIntegral.begin(), eulerIntegral.end());
		
		glLineWidth(1.0f);
		glPointSize(4.0f);
		glColor4ub(0, 64, 255, 255);
		glLineStrip2f(rungeKuttaIntegral.begin(), rungeKuttaIntegral.end());
		glPoints2f(rungeKuttaIntegral.begin(), rungeKuttaIntegral.end());

		protoGL.Swap();
	}
}




void SincTst()
{
	const float length = 128.0f;
	const float step = 4.0f;
	const float invStep = 1.0f / step;
	const float subStep = 64.0f;
	const float subInvStep = 1.0f / subStep;

	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera2D(protoGL.GetCanvas(), CameraOrtho2D::Upward);
	camera2D.SetCamera(Vector2f(zero), 10.0f);

	const float PI = acos(-1.0f);
	auto Sinc = [=](float x) { return x == 0.0f ? 1.0 : sin(x) / x; };

	auto Sigmoid = [=](float x) { return exp(x) / (exp(x) + 1.0f); };

//	auto Sample = [=](float x) { return sin(x * PI + PI * 0.5f); };
//	auto Sample = [=](float x) { return (-fmod(x * (0.5f / 6.475f), 1.0f) * 2.0f + 1.0f) * exp(-x * 0.02f); };
//	auto Sample = [=](float x) { return sin(x * PI * 0.95f) * exp(-x * 0.02f); };
	auto Sample = [=](float x)
	{
		const float f = 1.0f / 6.475f;
		// const float f = 1.0f / 2.475f;
		float v = 0.0f;
		float i = 1.0f;
		for (float y = f; y <= 1.0f; y += f, i += 1.0f)
			v += sin(x * PI * y) / i;
		return Sigmoid(v * exp(-x * 0.02f) * 5.0f) * 2 - 1;
	};

	deque<Vector2f> origPts;
	deque<Vector2f> samplesPts;
	vector<Vector2f> reconPts;
	for (float x = 0.0f; x < length; x += subInvStep)
	{
		const float y = Sample(x);
		origPts.push_back(Vector2f(x * invStep, y));
	}
	for (float x = 0.0f; x < length; x += 1.0f)
	{
		const float y = Sample(x);
		samplesPts.push_back(Vector2f(x * invStep, y));
	}
	reconPts.resize(origPts.size(), Vector2f(zero));
	for (uint i = 0; i < origPts.size(); ++i)
		reconPts[i].x = origPts[i].x;
	for (uint s = 0; s < samplesPts.size(); ++s)
	{
		const float samp = samplesPts[s].y;
		auto it = reconPts.begin();
		for (float x = 0.0f; x < length; x += subInvStep, ++it)
			it->y += Sinc((x - s) * PI) * samp;
	}

	while (protoGL.Update())
	{
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera2D.CommitToGL();

		glLineWidth(2.0f);
		glColor4ub(0, 0, 0, 255);
		glBegin(GL_LINES);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(length * invStep, 0.0f);
		glVertex2f(0.0f, -1.0f);
		glVertex2f(0.0f,  1.0f);
		glEnd();

		glLineWidth(1.0f);
		glColor4ub(128, 128, 128, 255);
		glBegin(GL_LINES);
		glVertex2f(0.0f, -1.0f);
		glVertex2f(length * invStep, -1.0f);
		glVertex2f(0.0f, 1.0f);
		glVertex2f(length * invStep, 1.0f);
		for (float x = 1.0f; x < length; x += 1.0f)
		{
			glVertex2f(x * invStep, -1.0f);
			glVertex2f(x * invStep, 1.0f);
		}
		glEnd();

		glLineWidth(2.0f);
		glColor4ub(255, 192, 64, 255);
		glBegin(GL_LINE_STRIP);
		for (Vector2f v : origPts)
			glVertex(v);
		glEnd();

		glPointSize(7.0f);
		glColor4ub(255, 64, 8, 255);
		glBegin(GL_POINTS);
		for (Vector2f v : samplesPts)
			glVertex(v);
		glEnd();

		glLineWidth(2.0f);
		glColor4ub(16, 128, 255, 255);
		glBegin(GL_LINE_STRIP);
		for (Vector2f v : reconPts)
			glVertex(v);
		glEnd();

		protoGL.Swap();
	}
}



float CoxDeBoorBasis(uint i, uint k, float x)
{
	if (k == 0)
		return x >= i && x < (i + 1) ? 1.0f : 0.0f;
	return
		((x - i) / ((i + k) - i)) * CoxDeBoorBasis(i, k - 1, x) +
		(((i + k + 1) - x) / ((i + k + 1) - (i + 1))) * CoxDeBoorBasis(i + 1, k - 1, x);
}



void BSpline()
{
	const float length = 8.0f;
	const float subStep = 64.0f;
	const float subInvStep = 1.0f / subStep;

	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera2D(protoGL.GetCanvas(), CameraOrtho2D::Upward);
	camera2D.SetCamera(Vector2f(zero), 10.0f);

	vector<Vector2f> points;
	points.emplace_back(0.0f, 0.0f);
	points.emplace_back(1.5f, -0.2f);
	points.emplace_back(2.1f,  1.35f);
	points.emplace_back(-0.35f, 0.95f);
	points.emplace_back(-1.35f, 1.36f);
	points.emplace_back(-0.85f, -0.36f);

	while (protoGL.Update())
	{
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera2D.CommitToGL();

		/*
		glLineWidth(2.0f);
		glColor4ub(0, 0, 0, 255);
		glBegin(GL_LINES);
		glVertex2f(-length, 0.0f);
		glVertex2f( length, 0.0f);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(0.0f, 1.0f);
		glEnd();

		glLineWidth(1.0f);
		glColor4ub(128, 128, 128, 255);
		glBegin(GL_LINES);
		for (float x = -length; x <= length; x += 1.0f)
		{
			glVertex2f(x, 0.0f);
			glVertex2f(x, 1.0f);
		}
		glEnd();

		glLineWidth(3.0f);
		glColor4ub(255, 192, 32, 255);
		glBegin(GL_LINE_STRIP);
		for (float x = -length; x <= length; x += subInvStep)
		{
			const float y = CoxDeBoorBasis(0, 3, x + 2);
			glVertex2f(x, y);
		}
		glEnd();
		*/

		glLineWidth(1.0f);
		glColor4ub(128, 128, 128, 255);
		glBegin(GL_LINES);
		glVertex2f(-10.0f,   0.0f);
		glVertex2f( 10.0f,   0.0f);
		glVertex2f(  0.0f, -10.0f);
		glVertex2f(  0.0f,  10.0f);
		glEnd();

		glLineWidth(1.0f);
		glColor4ub(255, 196, 128, 255);
		glBegin(GL_LINE_STRIP);
		for (Vector2f p : points)
			glVertex(p);
		glEnd();

		glPointSize(7.0f);
		glColor4ub(32, 192, 255, 255);
		glBegin(GL_POINTS);
		for (Vector2f p : points)
			glVertex(p);
		glEnd();

		const int degree = 3;
		glLineWidth(3.0f);
		glColor4ub(196, 128, 32, 255);
		glBegin(GL_LINE_STRIP);
		for (int i = 0; i < points.size() + 1; ++i)
		{
			for (float x = 0.0f; x < 1.0f; x += subInvStep)
			{
				Vector2f p{ zero };
				for (int j = 0; j < degree + degree / 2 - 1; ++j)
				{
					const Vector2f pt = points[Clamp(i + j - (degree - 1), 0, points.size() - 1)];
					const float m = CoxDeBoorBasis(0, degree, x + (degree - j));
					p = p + pt * m;
				}
				glVertex(p);
			}
		}
		glEnd();

		protoGL.Swap();
		glEnd();
	}
}



namespace std
{


	constexpr inline size_t hash_combine(size_t hash1, size_t hash2)
	{
		return hash1 ^ (hash2 * 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
	}


	template<> struct hash<Vector2i>
	{
		typedef vidf::Vector2i argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const& s) const noexcept
		{
			result_type const h1(hash<int>{}(s.x));
			result_type const h2(hash<int>{}(s.y));
			return hash_combine(h1, h2);
		}
	};


}


namespace
{
	



	class Element;
	typedef shared_ptr<Element> ElementPtr;
	typedef weak_ptr<Element>   ElementRef;


	enum class VertexType
	{
		Input,
		Output,
	};



	enum class State
	{
		Zero,
		One,
	};



	void glSetLineState()
	{
		glLineWidth(1.0f);
		glColor4ub(0, 0, 0, 255);
	}


	void glSetEdgeState(State state)
	{
		glLineWidth(1.5f);
		if (state == State::Zero)
			glColor4ub(0, 0, 0, 255);
		else
			glColor4ub(0, 198, 0, 255);
	}


	void glDrawQuadric(Vector2f p0, Vector2f p1, Vector2f p2, uint numVertices)
	{
		glBegin(GL_LINE_STRIP);
		for (float t = 0.0f; t <= 1.0f; t += 1.0f / numVertices)
		{
			const Vector2f p = Lerp(Lerp(p0, p1, t), Lerp(p1, p2, t), t);
			glVertex2f(p.x, p.y);
		}
		glEnd();
	}



	class EdgeMap
	{
	private:
		typedef array<uint, 2> Edge;
		typedef vector<uint>   EdgeIds;

		struct ElementVertex
		{
			Element*   element = nullptr;
			VertexType type;
			uint       elementVertexIdx = -1;
		};
		typedef vector<ElementVertex> ElementVertices;

	public:
		uint AddVertex(Vector2i point);
		void AddVertices(Element* element);
		uint AddEdge(Vector2i start, Vector2i end);
		void Draw() const;

		void Trigger(Element* element, uint elementVertexId, State outState);
		void UpdateStates();

	private:
		void TriggerEdge(uint outEdgeIdx, State outState);
		void TriggerVertex(uint outVertexIdx, State outState);

	private:
		deque<Vector2i>               vertexPositions;
		deque<ElementVertices>        elementVertices;
		deque<EdgeIds>                vertexEdges;
		deque<State>                  vertexState;
		deque<uint>                   activeVertices;
		unordered_map<Vector2i, uint> positionToVertex;

		deque<Edge>  edges;
		deque<State> edgeState;
		deque<uint>  activeEdges;

		deque<uint> pendingVertices;
	};



	class Element
	{
	public:
		virtual uint        GetNumVertices() const = 0;
		virtual Vector2i    GetVertexPoint(uint idx) const = 0;
		virtual const char* GetVertexName(uint idx) const = 0;
		virtual VertexType  GetVertexType(uint idx) const = 0;
		Vector2i            GetPosition() const { return position; }
		void                Offset(Vector2i offset) { position = position + offset; }
		virtual Vector2i    GetSize() const = 0;

		virtual void OnVertexTrigger(EdgeMap* edgeMap, uint inVertexId, State inState) {}

		bool IsPointInside(Vector2f pt) const;
		virtual void MouseDown(EdgeMap* edgeMap) {}
		virtual void MouseUp(EdgeMap* edgeMap) {}

		void Draw() const;

	protected:
		virtual void DoDraw() const = 0;

	private:
		friend class EdgeMap;
		vector<uint> verticesToEdgeMapVertexIdx;
		Vector2i     position{zero};
	};

	void Element::Draw() const
	{
		glPushMatrix();
		glTranslatef(position.x, position.y, 0.0f);
		DoDraw();
		glPopMatrix();
	}

	bool Element::IsPointInside(Vector2f pt) const
	{
		const Vector2i size = GetSize();
		if (pt.x < float(position.x))
			return false;
		if (pt.y < float(position.y))
			return false;
		if (pt.x > float(position.x + size.x))
			return false;
		if (pt.y > float(position.y + size.y))
			return false;
		return true;
	}

	

	

	uint EdgeMap::AddVertex(Vector2i position)
	{
		auto it = positionToVertex.find(position);
		if (it == positionToVertex.end())
		{
			uint idx = vertexPositions.size();
			vertexPositions.push_back(position);
			activeVertices.push_back(idx);
			elementVertices.emplace_back();
			vertexEdges.emplace_back();
			vertexState.emplace_back(State::Zero);
			positionToVertex[position] = idx;
			return idx;
		}
		return it->second;
	}

	void EdgeMap::AddVertices(Element* element)
	{
		const uint numVertices = element->GetNumVertices();
		const Vector2i position = element->GetPosition();
		element->verticesToEdgeMapVertexIdx.resize(numVertices);
		for (uint i = 0; i < numVertices; ++i)
		{
			uint idx = AddVertex(position + element->GetVertexPoint(i));
			ElementVertex elementVertex;
			elementVertex.element = element;
			elementVertex.type = element->GetVertexType(i);
			elementVertex.elementVertexIdx = i;
			elementVertices[idx].push_back(elementVertex);
			element->verticesToEdgeMapVertexIdx[i] = idx;
		//	if (elementVertex.type == VertexType::Input)
		//		element->OnVertexTrigger(this, i, State::Zero);
		}
	}

	uint EdgeMap::AddEdge(Vector2i start, Vector2i end)
	{
		Edge edge;
		edge[0] = AddVertex(start);
		edge[1] = AddVertex(end);

		uint idx = edges.size();
		edges.push_back(edge);
		edgeState.push_back(State::Zero);
		activeEdges.push_back(idx);

		vertexEdges[edge[0]].push_back(idx);
		vertexEdges[edge[1]].push_back(idx);

		return idx;
	}

	void EdgeMap::Trigger(Element* element, uint elementVertexId, State outState)
	{
		TriggerVertex(element->verticesToEdgeMapVertexIdx[elementVertexId], outState);
	}

	void EdgeMap::TriggerEdge(uint outEdgeIdx, State outState)
	{
		TriggerVertex(edges[outEdgeIdx][0], outState);
		TriggerVertex(edges[outEdgeIdx][1], outState);
		edgeState[outEdgeIdx] = outState;
	}

	void EdgeMap::TriggerVertex(uint outVertexIdx, State outState)
	{
		if (vertexState[outVertexIdx] == outState)
			return;
		vertexState[outVertexIdx] = outState;
		for (ElementVertex elementVertex : elementVertices[outVertexIdx])
		{
			if (elementVertex.type == VertexType::Input)
				elementVertex.element->OnVertexTrigger(this, elementVertex.elementVertexIdx, outState);
		}
		pendingVertices.push_back(outVertexIdx);
	}

	void EdgeMap::UpdateStates()
	{
		while (!pendingVertices.empty())
		{
			const uint vertexIdx = pendingVertices.front();
			pendingVertices.pop_front();

			const State state = vertexState[vertexIdx];
			for (uint edgeIdx : vertexEdges[vertexIdx])
				TriggerEdge(edgeIdx, state);
		}
	}

	void EdgeMap::Draw() const
	{
		const float radius = 0.1f;

		glSetLineState();
		for (auto idx : activeVertices)
		{
			if (vertexEdges[idx].size() != 1)
				DrawCircle(Vector2f(vertexPositions[idx]), radius, true);
		}

		for (auto idx : activeEdges)
		{
			glSetEdgeState(edgeState[idx]);
			glBegin(GL_LINES);
			glVertex(Vector2f(vertexPositions[edges[idx][0]]));
			glVertex(Vector2f(vertexPositions[edges[idx][1]]));
			glEnd();
		}
	}



	class Button : public Element
	{
	public:
		virtual uint GetNumVertices() const override
		{
			return 1;
		}
		virtual Vector2i GetVertexPoint(uint idx) const override
		{
			return Vector2i(2, 1);
		}
		virtual const char* GetVertexName(uint idx) const override
		{
			return "out";
		}
		virtual VertexType GetVertexType(uint idx) const override
		{
			return VertexType::Output;
		}
		virtual Vector2i GetSize() const override
		{
			return Vector2i(2, 2);
		}
		virtual void MouseUp(EdgeMap* edgeMap) override
		{
			if (state == State::Zero)
				state = State::One;
			else if (state == State::One)
				state = State::Zero;
			edgeMap->Trigger(this, 0, state);
		}

		static uint OutIdx()
		{
			return 0;
		}

	protected:
		virtual void DoDraw() const override
		{
			const float gap = 0.3f;
			const float gap2 = 0.5f;

			glSetEdgeState(state);
			glBegin(GL_QUADS);
			glVertex2f(gap2, gap2);
			glVertex2f(2.0f - gap2, gap2);
			glVertex2f(2.0f - gap2, 2.0f - gap2);
			glVertex2f(gap2, 2.0f - gap2);
			glEnd();
			glBegin(GL_LINES);
			glVertex2f(2.0f - gap, 1.0f);
			glVertex2f(2.0f, 1.0f);
			glEnd();

			glSetLineState();			
			glBegin(GL_LINE_LOOP);
			glVertex2f(gap, gap);
			glVertex2f(2.0f - gap, gap);
			glVertex2f(2.0f - gap, 2.0f - gap);
			glVertex2f(gap, 2.0f - gap);
			glEnd();
		}

	private:
		State state = State::Zero;
	};



	class Led : public Element
	{
	public:
		virtual uint GetNumVertices() const override
		{
			return 1;
		}
		virtual Vector2i GetVertexPoint(uint idx) const override
		{
			return Vector2i(0, 1);
		}
		virtual const char* GetVertexName(uint idx) const override
		{
			return "in";
		}
		virtual VertexType GetVertexType(uint idx) const override
		{
			return VertexType::Input;
		}
		virtual Vector2i GetSize() const override
		{
			return Vector2i(2, 2);
		}
		virtual void OnVertexTrigger(EdgeMap* edgeMap, uint inVertexId, State inState) override
		{
			state = inState;
		}

		static uint InIdx()
		{
			return 0;
		}

	protected:
		virtual void DoDraw() const override
		{
			const float gap = 0.3f;
			const float gap2 = 0.5f;

			glSetEdgeState(state);
			DrawCircle(Vector2f(1.0f, 1.0f), 1.0f - gap2, true);
			glBegin(GL_LINES);
			glVertex2f(0.0f, 1.0f);
			glVertex2f(gap, 1.0f);
			glEnd();

			glSetLineState();					
			DrawCircle(Vector2f(1.0f, 1.0f), 1.0f - gap, false);
		}

	private:
		State state = State::Zero;
	};



	class Port : public Element
	{
	public:
		Port(VertexType _type, const char* _name)
			: name(_name)
			, type(_type) {}

		virtual uint GetNumVertices() const override
		{
			return 2;
		}
		virtual Vector2i GetVertexPoint(uint idx) const override
		{
			if (idx == 0)
				return Vector2i(0, 1);
			return Vector2i(1, 1);
		}
		virtual const char* GetVertexName(uint idx) const override
		{
			if (idx == 0)
				return "in";
			return "out";
		}
		virtual VertexType GetVertexType(uint idx) const override
		{
			if (idx == 0)
				return VertexType::Input;
			else
				return VertexType::Output;
		}
		virtual Vector2i GetSize() const override
		{
			return Vector2i(1, 2);
		}
		virtual void OnVertexTrigger(EdgeMap* edgeMap, uint inVertexId, State inState) override
		{
			state = inState;
			edgeMap->Trigger(this, 1, state);
		}

		const char* GetName() const
		{
			return name.c_str();
		}

		VertexType GetPortType() const
		{
			return type;
		}

		uint InIdx() const { return 0; }
		uint OutIdx() const { return 1; }

	protected:
		virtual void DoDraw() const override
		{
			glSetEdgeState(state);
			glBegin(GL_LINES);
			glVertex2f(0.0f, 1.0f);
			glVertex2f(1.0f, 1.0f);
			glEnd();
			glBegin(GL_QUADS);
			glVertex2f(0.25f, 1.0f);
			glVertex2f(0.5f, 1.25f);
			glVertex2f(0.75f, 1.0f);
			glVertex2f(0.5f, 0.75f);
			glEnd();
		}

	private:
		string     name;
		VertexType type;
		State      state = State::Zero;
	};

	typedef shared_ptr<Port> PortPtr;



	class NotGate : public Element
	{
	public:
		virtual uint GetNumVertices() const override
		{
			return 2;
		}
		virtual Vector2i GetVertexPoint(uint idx) const override
		{
			if (idx == 0)
				return Vector2i(0, 1);
			return Vector2i(3, 1);
		}
		virtual const char* GetVertexName(uint idx) const override
		{
			if (idx == 0)
				return "in";
			return "out";
		}
		virtual VertexType GetVertexType(uint idx) const override
		{
			if (idx == 0)
				return VertexType::Input;
			return VertexType::Output;
		}
		virtual Vector2i GetSize() const override
		{
			return Vector2i(3, 2);
		}
		virtual void OnVertexTrigger(EdgeMap* edgeMap, uint inVertexId, State _inState) override
		{
			inState = _inState;
			if (inState == State::Zero)
				outState = State::One;
			else if (inState == State::One)
				outState = State::Zero;
			edgeMap->Trigger(this, 1, outState);
		}

		static uint InIdx() { return 0; }
		static uint OutIdx() { return 1; }

	protected:
		virtual void DoDraw() const override
		{
			const float offset = 0.5f;
			const float gap = 0.25f;
			const float radius = 0.15f;

			glSetLineState();
			glBegin(GL_LINE_LOOP);
			glVertex2f(offset, gap);
			glVertex2f(offset + 2.0f - gap, 1.0f);
			glVertex2f(offset, 2.0f - gap);
			glEnd();
			DrawCircle(Vector2f(offset + 2.0f - gap + radius, 1.0f), radius, false);

			glSetEdgeState(inState);
			glBegin(GL_LINES);
			glVertex2f(0.0f, 1.0f);
			glVertex2f(offset, 1.0f);
			glEnd();

			glSetEdgeState(outState);
			glBegin(GL_LINES);
			glVertex2f(offset + 2.0f - gap + radius * 2.0f, 1.0f);
			glVertex2f(3.0f, 1.0f);
			glEnd();
		}

	private:
		State inState = State::Zero;
		State outState = State::One;
	};



	enum class GateType
	{
		And,
		Or,
		Nand,
		Nor,
		Xor,
		Nxor,
	};


	
	class Gate : public Element
	{
	public:
		Gate(GateType _type, uint numInputs)
			: type(_type)
		{
			inStates.resize(numInputs);
		}

		virtual uint GetNumVertices() const override
		{
			return inStates.size() + 1;
		}
		virtual Vector2i GetVertexPoint(uint idx) const override
		{
			if (idx == inStates.size())
				return Vector2i(GetSize().x, GetSize().y / 2);
			Vector2i pt = Vector2i(0, int(1 + idx));
			if (idx >= inStates.size() / 2 && HasGap())
				pt.y += 1;
			return pt;
		}
		virtual const char* GetVertexName(uint idx) const override
		{
			if (idx == inStates.size())
				return "out";
			return "in";
		}
		virtual VertexType GetVertexType(uint idx) const override
		{
			if (idx == inStates.size())
				return VertexType::Output;
			return VertexType::Input;
		}
		virtual Vector2i GetSize() const override
		{
			Vector2i sz = Vector2i(5, int((inStates.size() - 1) + 2));
			if (HasGap())
				sz.y += 1;
			return sz;
		}
		virtual void OnVertexTrigger(EdgeMap* edgeMap, uint inVertexId, State _inState) override
		{
			inStates[inVertexId] = _inState;

			switch (type)
			{
			case GateType::And: [[fallthrough]];
			case GateType::Nand:
				outState = State::One;
				for (uint i = 0; i < inStates.size(); ++i)
				{
					if (inStates[i] == State::One)
						continue;
					outState = State::Zero;
					break;
				}
				break;

			case GateType::Or: [[fallthrough]];
			case GateType::Nor:
				outState = State::Zero;
				for (uint i = 0; i < inStates.size(); ++i)
				{
					if (inStates[i] == State::Zero)
						continue;
					outState = State::One;
					break;
				}
				break;

			case GateType::Xor: [[fallthrough]];
			case GateType::Nxor:
			{
				uint count = 0;
				for (uint i = 0; i < inStates.size(); ++i)
					count += inStates[i] == State::One;
				outState = (count == inStates.size() || count == 0) ? State::Zero : State::One;
			}
			break;
			}

			switch (type)
			{
			case GateType::Nand: [[fallthrough]];
			case GateType::Nor: [[fallthrough]];
			case GateType::Nxor:
				if (outState == State::Zero)
					outState = State::One;
				else
					outState = State::Zero;
			}

			edgeMap->Trigger(this, inStates.size(), outState);
		}

	protected:
		virtual void DoDraw() const override
		{
			const float offset = 0.75f;
			const float gap = 0.5f;
			const float radius = 0.15f;
			const Vector2i size = GetSize();

			glSetLineState();
			switch (type)
			{
			case GateType::And: [[fallthrough]];
			case GateType::Nand:
				glBegin(GL_LINE_STRIP);
				glVertex2f(offset + 2.0f, gap);
				glVertex2f(offset, gap);
				glVertex2f(offset, size.y - gap);
				glVertex2f(offset + 2.0f, size.y - gap);
				glEnd();
				glBegin(GL_LINE_STRIP);
				for (float t = 0.0f; t <= 1.0f; t += 1.0f / 12.0f)
				{
					const float th = Lerp(-PI * 0.5f, PI * 0.5f, t);
					const float x = cos(th);
					const float y = sin(th) * 0.5f + 0.5f;
					glVertex2f(offset + 2.0f + x * 1.5f, gap + y * (size.y - gap * 2.0f));
				}
				glEnd();
				break;

			case GateType::Xor: [[fallthrough]];
			case GateType::Nxor:
				glDrawQuadric(
					Vector2f(offset - gap, gap),
					Vector2f(offset - gap + 1.0f, size.y * 0.5f),
					Vector2f(offset - gap, size.y - gap), 6);
				[[fallthrough]];

			case GateType::Or: [[fallthrough]];
			case GateType::Nor:
				glBegin(GL_LINES);
				glVertex2f(offset + 1.0f, gap);
				glVertex2f(offset, gap);
				glVertex2f(offset, size.y - gap);
				glVertex2f(offset + 1.0f, size.y - gap);
				glEnd();
				glDrawQuadric(
					Vector2f(offset, gap),
					Vector2f(offset + 1.0f, size.y * 0.5f),
					Vector2f(offset, size.y - gap), 6);
				glDrawQuadric(
					Vector2f(offset + 1.0f, gap),
					Vector2f(offset + 2.5f, gap),
					Vector2f(offset + 3.5f, size.y * 0.5f), 6);
				glDrawQuadric(
					Vector2f(offset + 1.0f, size.y - gap),
					Vector2f(offset + 2.5f, size.y - gap),
					Vector2f(offset + 3.5f, size.y * 0.5f), 6);
				break;
			}
						
			switch (type)
			{
			case GateType::Nand: [[fallthrough]];
			case GateType::Nor:
			case GateType::Nxor:
				DrawCircle(Vector2f(offset + 3.5f + radius, size.y * 0.5f), radius, false);
				glSetEdgeState(outState);
				glBegin(GL_LINES);
				glVertex2f(offset + 3.5f + radius * 2.0f, size.y * 0.5f);
				glVertex2f(5.0f, size.y * 0.5f);
				glEnd();
				break;

			case GateType::And: [[fallthrough]];
			case GateType::Or:
			case GateType::Xor:
				glSetEdgeState(outState);
				glBegin(GL_LINES);
				glVertex2f(offset + 3.5f, size.y * 0.5f);
				glVertex2f(5.0f, size.y * 0.5f);
				glEnd();
				break;
			}

			for (uint i = 0; i < inStates.size(); ++i)
			{
				Vector2i pt = GetVertexPoint(i);
				glSetEdgeState(inStates[i]);
				glBegin(GL_LINES);
				glVertex2f(0.0f, float(pt.y));
				glVertex2f(offset, float(pt.y));
				glEnd();
			}
		}

	private:
		bool HasGap() const
		{
			return inStates.size() % 2 == 0;
		}

	private:
		vector<State> inStates;
		GateType type;
		State    outState = State::Zero;
	};



	class LogicBlock;
	typedef shared_ptr<LogicBlock> LogicBlockPtr;


	class LogicBlock
	{
	private:
		struct Edge
		{
			Vector2i vertices[2];
		};

	public:
		LogicBlock(const char* _name)
			: name(_name)
		{
		}

		template<typename GateType, typename ...Args>
		shared_ptr<GateType> AddElement(Args... args)
		{
			shared_ptr<GateType> element = make_shared<GateType>(args...);
			elements.emplace_back(element);
			return element;
		}

		LogicBlockPtr AddLogicBlock(const char* name)
		{
			LogicBlockPtr block = make_shared<LogicBlock>(name);
			blocks.emplace_back(block);
			return block;
		}

		PortPtr AddPort(VertexType _type, const char* name)
		{
			PortPtr port = make_shared<Port>(_type, name);
			ports[name] = port;
			return port;
		}

		void AddEdge(Vector2i v0, Vector2i v1)
		{
			Edge edge;
			edge.vertices[0] = v0;
			edge.vertices[1] = v1;
			edges.push_back(edge);
		}

		void AddEdge(Vector2i v0, Vector2i v1, Vector2i v2)
		{
			Edge edge;
			edge.vertices[0] = v0;
			edge.vertices[1] = v1;
			edges.push_back(edge);
			edge.vertices[0] = v1;
			edge.vertices[1] = v2;
			edges.push_back(edge);
		}

		void AddEdge(Vector2i v0, Vector2i v1, Vector2i v2, Vector2i v3)
		{
			Edge edge;
			edge.vertices[0] = v0;
			edge.vertices[1] = v1;
			edges.push_back(edge);
			edge.vertices[0] = v1;
			edge.vertices[1] = v2;
			edges.push_back(edge);
			edge.vertices[0] = v2;
			edge.vertices[1] = v3;
			edges.push_back(edge);
		}

		void Offset(Vector2i offset)
		{
			for (auto subBlock : blocks)
				subBlock->Offset(offset);
			for (ElementPtr element : elements)
				element->Offset(offset);
			for (auto port : ports)
				port.second->Offset(offset);
			for (Edge& edge : edges)
			{
				edge.vertices[0] = edge.vertices[0] + offset;
				edge.vertices[1] = edge.vertices[1] + offset;
			}
			blockRect.min = blockRect.min + offset;
			blockRect.max = blockRect.max + offset;
		}

		void Recenter()
		{
			CalcBlockRect();
			Offset(-blockRect.min);
		}

		void AddToEdgeMap(EdgeMap* edgeMap)
		{
			for (auto subBlock : blocks)
				subBlock->AddToEdgeMap(edgeMap);
			for (ElementPtr element : elements)
				edgeMap->AddVertices(element.get());
			for (auto port : ports)
				edgeMap->AddVertices(port.second.get());
			for (Edge edge : edges)
				edgeMap->AddEdge(edge.vertices[0], edge.vertices[1]);
		};

		void Draw() const
		{
			glLineWidth(1.0f);
			glColor4ub(128, 0, 0, 128);
			glBegin(GL_LINE_LOOP);
			glVertex2f(blockRect.min.x + 0.5f, blockRect.min.y + 0.5f);
			glVertex2f(blockRect.max.x - 0.5f, blockRect.min.y + 0.5f);
			glVertex2f(blockRect.max.x - 0.5f, blockRect.max.y - 0.5f);
			glVertex2f(blockRect.min.x + 0.5f, blockRect.max.y - 0.5f);
			glEnd();

			for (auto subBlock : blocks)
				subBlock->Draw();
			for (ElementPtr element : elements)
				element->Draw();
			for (auto port : ports)
				port.second->Draw();
		}

		void MouseDown(EdgeMap* edgeMap, Vector2f mousePos)
		{
			for (ElementPtr element : elements)
			{
				if (!element->IsPointInside(mousePos))
					continue;
				element->MouseDown(edgeMap);
				break;
			}
		}

		void MouseUp(EdgeMap* edgeMap, Vector2f mousePos)
		{
			for (ElementPtr element : elements)
			{
				if (!element->IsPointInside(mousePos))
					continue;
				element->MouseUp(edgeMap);
				break;
			}
		}

		Recti    GetBounds() const { return blockRect; }
		Vector2i GetPosition() const { return blockRect.min; }
		Vector2i GetSize() const { return blockRect.max - blockRect.min; }

		Vector2i PortIn(const char* name) const
		{
			auto it = ports.find(name);
			if (it == ports.end())
				return Vector2i(0, 0);
			return it->second->GetPosition() + it->second->GetVertexPoint(0);
		}

		Vector2i PortOut(const char* name) const
		{
			auto it = ports.find(name);
			if (it == ports.end())
				return Vector2i(0, 0);
			return it->second->GetPosition() + it->second->GetVertexPoint(1);
		}

	private:
		void CalcBlockRect()
		{
			const int maxVal = std::numeric_limits<int>::max();
			const int minVal = std::numeric_limits<int>::min();
			blockRect.min = Vector2i(maxVal, maxVal);
			blockRect.max = Vector2i(minVal, minVal);

			for (uint i = 0; i < elements.size(); ++i)
			{
				const Vector2i position = elements[i]->GetPosition();
				const Vector2i size = elements[i]->GetSize();
				Recti elementRect;
				elementRect.min = position - Vector2i(1, 1);
				elementRect.max = position + size + Vector2i(1, 1);
				blockRect = Union(blockRect, elementRect);
			}
			for (auto edge : edges)
			{
				Recti bounds{ edge.vertices[0] , edge.vertices[0] };
				bounds = Union(bounds, edge.vertices[1]);
				bounds.min = bounds.min - Vector2i(1, 1);
				bounds.max = bounds.max + Vector2i(1, 1);
				blockRect = Union(blockRect, bounds);
			}
			for (auto port : ports)
			{
				const Vector2i position = port.second->GetPosition();
				const Vector2i size = port.second->GetSize();
				Recti elementRect;
				elementRect.min = position;
				elementRect.max = position + size;
				blockRect = Union(blockRect, elementRect);
			}
			for (auto block : blocks)
			{
				Recti bounds = block->blockRect;
				bounds.min = bounds.min - Vector2i(1, 1);
				bounds.max = bounds.max + Vector2i(1, 1);
				blockRect = Union(blockRect, bounds);
			}
		}

	private:
		string               name;
		deque<ElementPtr>    elements;
		deque<LogicBlockPtr> blocks;
		unordered_map<string, PortPtr> ports;
		deque<Edge>          edges;
		Recti                blockRect;

		vector<State> portStates;
	};



	enum Direction
	{
		Right,
		Left,
		Up,
		Down,
	};



	ElementPtr Align(ElementPtr element, uint fromVertexIdx, Vector2i toLocation, Vector2i offset = Vector2i{zero})
	{
		const Vector2i fromLocation = element->GetPosition() + element->GetVertexPoint(fromVertexIdx);
		element->Offset(toLocation - fromLocation + offset);
		return element;
	}



	template<typename Elem0, typename Elem1>
	void Align(shared_ptr<Elem0> element, shared_ptr<Elem1> reference, Direction dir, Vector2i offset = Vector2i{ zero })
	{
		const Vector2i pos0 = element->GetPosition();
		const Vector2i size0 = element->GetSize();
		const Vector2i pos1 = reference->GetPosition();
		const Vector2i size1 = reference->GetSize();
		switch (dir)
		{
		case Right:
			element->Offset(Vector2i(pos1.x + size1.x, pos1.y) + offset);
			break;
		case Left:
			element->Offset(Vector2i(pos1.x - size0.x, pos1.y) + offset);
			break;
		case Up:
			element->Offset(Vector2i(pos1.x, pos1.y + size1.y) + offset);
			break;
		case Down:
			element->Offset(Vector2i(pos1.x, pos1.y - size0.y) + offset);
			break;
		}
	}



	Vector2i Vertex(ElementPtr elem, uint vertexIdx)
	{
		return elem->GetPosition() + elem->GetVertexPoint(vertexIdx);
	}



	LogicBlockPtr MakeHalfAdder(LogicBlockPtr parent, bool test = false)
	{
		LogicBlockPtr block = parent->AddLogicBlock("HalfAdder");

		auto xor = block->AddElement<Gate>(GateType::Xor, 2);
		auto and = block->AddElement<Gate>(GateType::And, 2);
		Align(and, xor, Direction::Down);
		auto v0 = Vertex(xor, 1) - Vector2i(1, 0);
		auto v1 = Vertex(and, 1) - Vector2i(1, 0);
		block->AddEdge(Vertex(xor, 0), Vertex(and, 0));
		block->AddEdge(v0, Vertex(xor, 1));
		block->AddEdge(v1, Vertex(and, 1));
		block->AddEdge(v0, v1);

		auto A = block->AddPort(VertexType::Input, "A");
		auto B = block->AddPort(VertexType::Input, "B");
		Align(A, A->OutIdx(), v0);
		Align(B, A, Direction::Down);
		block->AddEdge(Vertex(B, B->OutIdx()), Vertex(xor, 0));
		
		auto S = block->AddPort(VertexType::Output, "S");
		Align(S, S->InIdx(), Vertex(xor, 2), Vector2i(0, 1));
		auto C = block->AddPort(VertexType::Output, "C");
		Align(C, S, Direction::Down);
		block->AddEdge(Vertex(xor, 2), Vertex(S, 0));
		block->AddEdge(Vertex(and, 2), Vertex(C, 0));
		
		block->Recenter();

		if (test)
		{
			Align(parent->AddElement<Button>(), Button::OutIdx(), block->PortIn("A"));
			Align(parent->AddElement<Button>(), Button::OutIdx(), block->PortIn("B"));
			Align(parent->AddElement<Led>(), Led::InIdx(), block->PortOut("S"));
			Align(parent->AddElement<Led>(), Led::InIdx(), block->PortOut("C"));
		}

		return block;
	}



	LogicBlockPtr MakeFullAdder(LogicBlockPtr parent, bool test = false)
	{
		LogicBlockPtr block = parent->AddLogicBlock("FullAdder");

		auto halfAdder0 = MakeHalfAdder(block);
		auto halfAdder1 = MakeHalfAdder(block);
		Align(halfAdder1, halfAdder0, Direction::Right, Vector2i(1, 0));
				
		auto A = block->AddPort(VertexType::Input, "A");
		auto B = block->AddPort(VertexType::Input, "B");
		auto Cin = block->AddPort(VertexType::Input, "Cin");
		Align(A, Cin->OutIdx(), halfAdder0->PortIn("A"));
		Align(B, Cin->OutIdx(), halfAdder0->PortIn("B"));
		Align(Cin, B, Direction::Down);

		auto v0 = halfAdder0->GetPosition();
		auto v1 = halfAdder1->GetPosition();
		block->AddEdge(Vertex(Cin, 1), v0, v1, halfAdder1->PortIn("B"));
		block->AddEdge(halfAdder0->PortOut("S"), halfAdder1->PortIn("A"));

		auto or = block->AddElement<Gate>(GateType::Or, 2);
		Align(or , 1, halfAdder1->PortOut("C"), Vector2i(1, 0));
		auto v2 = Vector2i(halfAdder0->PortOut("C").x, halfAdder0->GetPosition().y - 1);
		auto v3 = Vector2i(Vertex(or, 0).x, v2.y);
		block->AddEdge(halfAdder0->PortOut("C"), v2);
		block->AddEdge(halfAdder1->PortOut("C"), Vertex(or, 1));
		block->AddEdge(v2, v3, Vertex(or , 0));

		auto S = block->AddPort(VertexType::Output, "S");
		auto Cout = block->AddPort(VertexType::Output, "Cout");
		Align(Cout, 0, Vertex(or, 2), Vector2i(0, 1));
		Align(S, Cout, Direction::Up);
		block->AddEdge(Vertex(or, 2), Vertex(Cout, 0));
		block->AddEdge(halfAdder1->PortOut("S"), Vertex(S, 0));

		block->Recenter();

		if (test)
		{
			Align(parent->AddElement<Button>(), Button::OutIdx(), block->PortIn("A"));
			Align(parent->AddElement<Button>(), Button::OutIdx(), block->PortIn("B"));
			Align(parent->AddElement<Button>(), Button::OutIdx(), block->PortIn("Cin"));
			Align(parent->AddElement<Led>(), Led::InIdx(), block->PortOut("S"));
			Align(parent->AddElement<Led>(), Led::InIdx(), block->PortOut("Cout"));
		}

		return block;
	}



	LogicBlockPtr MakeIntegerAdder(LogicBlockPtr parent, uint bits, bool test = false)
	{
		LogicBlockPtr block = parent->AddLogicBlock("IntegerAdder");

		vector<LogicBlockPtr> adders;
		vector<PortPtr> aports;
		vector<PortPtr> bports;
		adders.reserve(bits);
		aports.reserve(bits);
		bports.reserve(bits);

		// create adders
		for (uint b = 0; b < bits; ++b)
		{
			auto adder = MakeFullAdder(block);
			if (b != 0)
				Align(adder, adders.back(), Direction::Down, Vector2i(0, 0));

			adders.push_back(adder);
		}

		// create Cin and connect
		PortPtr Cin = block->AddPort(VertexType::Input, "Cin");
		Align(Cin, Cin->OutIdx(), adders[0]->PortIn("A"), Vector2i(-bits*2, -1));
		block->AddEdge(Vertex(Cin, 1), Vector2i(adders[0]->PortIn("Cin").x, Vertex(Cin, 1).y));
		block->AddEdge(Vector2i(adders[0]->PortIn("Cin").x, Vertex(Cin, 1).y), adders[0]->PortIn("Cin"));
		for (uint b = 0; b < bits - 1; ++b)
		{
			const auto Cout = adders[b]->PortOut("Cout");
			const auto Cin = adders[b + 1]->PortIn("Cin");
			const auto bminy = adders[b]->GetBounds().min.y;
			block->AddEdge(
				Cout,
				Vector2i(Cout.x, bminy),
				Vector2i(Cin.x, bminy),
				Cin);
		}
		
		// create A and B ports and connect
		for (uint b = 0; b < bits; ++b)
		{
			PortPtr ap = block->AddPort(VertexType::Input, (string("A") + char(b + '0')).c_str());
			PortPtr bp = block->AddPort(VertexType::Input, (string("B") + char(b + '0')).c_str());

			Align(ap, Cin, Direction::Down, Vector2i(0, -2 - b * 2));
			Align(bp, Cin, Direction::Down, Vector2i(0, -2 - bits * 2 - 2 - b * 2));

			const auto av = Vertex(ap, 1);
			const auto portInA = adders[b]->PortIn("A");
			block->AddEdge(
				av,
				Vector2i(portInA.x - b * 2 - 1, av.y),
				Vector2i(portInA.x - b * 2 - 1, portInA.y),
				portInA);

			const auto bv = Vertex(bp, 1);
			const auto portInB = adders[b]->PortIn("B");
			block->AddEdge(
				bv,
				Vector2i(portInB.x - b * 2 - 2, bv.y),
				Vector2i(portInB.x - b * 2 - 2, portInB.y),
				portInB);
		}

		// create Cout and connect
		PortPtr Cout = block->AddPort(VertexType::Input, "Cout");
		Align(Cout, Cout->OutIdx(), adders[0]->PortOut("Cout"), Vector2i(bits + 2, Cout->GetPosition().y));
		const auto coutp = adders[bits - 1]->PortOut("Cout");
		const auto portOutC = Vertex(Cout, 0);
		block->AddEdge(
			coutp,
			coutp + Vector2i(1, 0),
			Vector2i(coutp.x + 1, portOutC.y),
			portOutC);

		// create S ports and connect
		for (uint b = 0; b < bits; ++b)
		{
			PortPtr sp = block->AddPort(VertexType::Input, (string("S") + char(b + '0')).c_str());
			Align(sp, Cout, Direction::Down, Vector2i(0, -2 - b * 2));

			auto v0 = adders[b]->PortOut("S");
			auto v1 = Vertex(sp, 0);
			block->AddEdge(
				v0,
				Vector2i(v0.x + b + 2, v0.y),
				Vector2i(v0.x + b + 2, v1.y),
				v1);
		}

		block->Recenter();

		if (test)
		{
			Align(parent->AddElement<Button>(), Button::OutIdx(), block->PortIn("Cin"));
			Align(parent->AddElement<Led>(), Led::InIdx(), block->PortOut("Cout"));
			for (uint b = 0; b < bits; ++b)
			{
				Align(parent->AddElement<Button>(), Button::OutIdx(), block->PortIn((string("A") + char(b + '0')).c_str()));
				Align(parent->AddElement<Button>(), Button::OutIdx(), block->PortIn((string("B") + char(b + '0')).c_str()));
				Align(parent->AddElement<Led>(), Led::InIdx(), block->PortOut((string("S") + char(b + '0')).c_str()));
			}
		}

		return block;
	}



	LogicBlockPtr MakeSRLatch(LogicBlockPtr parent, bool test = false)
	{
		LogicBlockPtr block = parent->AddLogicBlock("SRLatch");

		// make SR latch
		auto nand0 = block->AddElement<Gate>(GateType::Nand, 2);
		auto nand1 = block->AddElement<Gate>(GateType::Nand, 2);
		Align(nand1, nand0, Direction::Down, Vector2i(0, -1));
		auto v0 = Vertex(nand0, 2);
		auto v1 = Vertex(nand1, 1);
		block->AddEdge(
			v0,
			Vector2i(v0.x, v0.y - 2),
			Vector2i(v1.x, v1.y + 1),
			v1);
		v0 = Vertex(nand1, 2);
		v1 = Vertex(nand0, 0);
		block->AddEdge(
			v0,
			Vector2i(v0.x, v0.y + 2),
			Vector2i(v1.x, v1.y - 1),
			v1);

		// Make DE circuit
		auto nand2 = block->AddElement<Gate>(GateType::Nand, 2);
		auto nand3 = block->AddElement<Gate>(GateType::Nand, 2);
		Align(nand2, 2, Vertex(nand0, 1), Vector2i(-1, 0));
		Align(nand3, 2, Vertex(nand1, 0), Vector2i(-1, 0));
		block->AddEdge(Vertex(nand2, 2), Vertex(nand0, 1));
		block->AddEdge(Vertex(nand3, 2), Vertex(nand1, 0));
		v0 = Vertex(nand2, 2);
		v1 = Vertex(nand3, 1);
		block->AddEdge(
			v0,
			Vector2i(v0.x, v0.y - 2),
			Vector2i(v1.x, v1.y + 2),
			v1);
		v0 = Vertex(nand2, 0);
		v1 = Vertex(nand3, 0);
		block->AddEdge(
			v0,
			Vector2i(v0.x - 1, v0.y),
			Vector2i(v1.x - 1, v1.y),
			v1);

		// add ports
		PortPtr D = block->AddPort(VertexType::Input, "D");
		PortPtr E = block->AddPort(VertexType::Input, "E");
		PortPtr Q = block->AddPort(VertexType::Output, "Q");
		Align(D, 1, Vertex(nand2, 1), Vector2i(-1, 0));
		Align(E, 1, Vertex(nand2, 0), Vector2i(-1, 0));
		Align(Q, 0, Vertex(nand0, 2));
		block->AddEdge(Vertex(D, 1), Vertex(nand2, 1));

		block->Recenter();

		if (test)
		{
			Align(parent->AddElement<Button>(), Button::OutIdx(), block->PortIn("D"));
			Align(parent->AddElement<Button>(), Button::OutIdx(), block->PortIn("E"));
			Align(parent->AddElement<Led>(), Led::InIdx(), block->PortOut("Q"));
		}

		return block;
	}



	inline int Log2Int(uint32_t v)
	{
		unsigned long lz = 0;
		if (_BitScanReverse(&lz, v)) return lz;
		return 0;
	}



	LogicBlockPtr MakeDecoder(LogicBlockPtr parent, uint count, uint spacing, bool test = false)
	{
		LogicBlockPtr block = parent->AddLogicBlock("Decoder");
		uint addrBits = Log2Int(count - 1) + 1;

		// create stack of ands
		vector<ElementPtr> ands;
		ands.reserve(count);
		for (uint i = 0; i < count; ++i)
		{
			auto and = block->AddElement<Gate>(GateType::And, addrBits);
			if (i != 0)
				Align(and, ands.back(), Direction::Down, Vector2i(0, 0));
			ands.push_back(and);
		}

		// create decoder lines
		const auto v0 = Vertex(ands.front(), addrBits - 1);
		const auto v1 = Vertex(ands.back(), 0);
		for (uint b = 0; b < addrBits * 2; ++b)
		{
			block->AddEdge(
				v0 + Vector2i(-1 - b, 0),
				v1 + Vector2i(-1 - b, 0));
		}

		// connect decoder lines to ands
		for (uint b = 0; b < addrBits; ++b)
		{
			for (uint c = 0; c < count; ++c)
			{
				const auto v0 = Vertex(ands[c], b);
				const auto v1 = v0 + Vector2i(-1 - b*2, 0);
				block->AddEdge(v0, v1);
			}
		}

		block->Recenter();

		return block;
	}



	class _Module;
	class _Element;
	typedef shared_ptr<_Module> _ModulePtr;
	typedef shared_ptr<_Element> _ElementPtr;


	struct Stats
	{
		uint numGates = 0;
		uint numFlipFlops = 0;
		uint numTbuffs = 0;
		uint numMosfets = 0;
		uint numLinks = 0;
		uint numPorts = 0;
	};



	struct ElementDesc
	{
		string name;
		uint   numInputs;
		uint   numOutputs;
		float  width;
		vector<Vector2f> portPositions;
	};



	State Not(State s)
	{
		return (s == State::Zero) ? State::One : State::Zero;
	}



	class DLatch
	{
	public:
		void Step(State d, State e)
		{
			st = (e == State::One) ? d : st;
		}

		State S() const { return st; }

	private:
		State st = State::Zero;
	};



	class DLatchSR
	{
	public:
		void Step(State d, State e, State s, State r)
		{
			if (s == State::One)
				st = State::One;
			else if (r == State::One)
				st = State::Zero;
			else
				st = (e == State::One) ? d : st;
		}

		State S() const { return st; }

	private:
		State st = State::Zero;
	};



	class _Element
	{
	public:
		virtual void Proccess(_Module& module) = 0;
		virtual void Reset(_Module& module) { Proccess(module); }
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const = 0;
		virtual void AddStats(Stats& stats) const = 0;

		virtual const ElementDesc& GetDesc() const = 0;
		virtual uint GetInput(uint idx) const = 0;
		virtual uint GetOutput(uint idx) const = 0;
	};

	typedef shared_ptr<_Element> _ElementPtr;



	class _NotElement : public _Element
	{
	public:
		_NotElement(_Module& module, const json11::Json& cell);
		_NotElement(_Module& module, const _NotElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 2;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"not",
				1, 1,
				0.5f,
				{
					Vector2f(0.1f, 0.5f),
					Vector2f(0.4f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return inIdx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return outIdx;
		}

	private:
		uint inIdx;
		uint outIdx;
	};



	class _BufElement : public _Element
	{
	public:
		_BufElement(_Module& module, const json11::Json& cell);
		_BufElement(_Module& module, const _BufElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 4;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"buf",
				1, 1,
				1.0f,
				{
					Vector2f(0.1f, 0.5f),
					Vector2f(0.9f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return inIdx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return outIdx;
		}

	private:
		uint inIdx;
		uint outIdx;
	};


	
	class _TBufElement : public _Element
	{
	public:
		_TBufElement(_Module& module, const json11::Json& cell);
		_TBufElement(_Module& module, const _TBufElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 8;
			stats.numTbuffs++;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"tbuf",
				2, 1,
				1.0f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.75f),
					Vector2f(0.9f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return idx == 0 ? inIdx : enIdx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return outIdx;
		}

	private:
		uint inIdx;
		uint enIdx;
		uint outIdx;
	};



	class _NorElement : public _Element
	{
	public:
		_NorElement(_Module& module, const json11::Json& cell);
		_NorElement(_Module& module, const _NorElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 4;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"nor",
				2, 1,
				1.0f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.75f),
					Vector2f(0.9f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return idx == 0 ? in0Idx : in1Idx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return outIdx;
		}

	private:
		uint in0Idx;
		uint in1Idx;
		uint outIdx;
	};



	class _OrElement : public _Element
	{
	public:
		_OrElement(_Module& module, const json11::Json& cell);
		_OrElement(_Module& module, const _OrElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 6;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"or",
				2, 1,
				1.0f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.75f),
					Vector2f(0.9f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return idx == 0 ? in0Idx : in1Idx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return outIdx;
		}

	private:
		uint in0Idx;
		uint in1Idx;
		uint outIdx;
	};



	class _AndElement : public _Element
	{
	public:
		_AndElement(_Module& module, const json11::Json& cell);
		_AndElement(_Module& module, const _AndElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 6;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"and",
				2, 1,
				1.0f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.75f),
					Vector2f(0.9f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return idx == 0 ? in0Idx : in1Idx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return outIdx;
		}

	private:
		uint in0Idx;
		uint in1Idx;
		uint outIdx;
	};




	class _NandElement : public _Element
	{
	public:
		_NandElement(_Module& module, const json11::Json& cell);
		_NandElement(_Module& module, const _NandElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 4;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"nand",
				2, 1,
				1.0f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.75f),
					Vector2f(0.9f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return idx == 0 ? in0Idx : in1Idx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return outIdx;
		}

	private:
		uint in0Idx;
		uint in1Idx;
		uint outIdx;
	};



	class _XorElement : public _Element
	{
	public:
		_XorElement(_Module& module, const json11::Json& cell);
		_XorElement(_Module& module, const _XorElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 4;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"xor",
				2, 1,
				1.5f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.75f),
					Vector2f(1.4f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return idx == 0 ? in0Idx : in1Idx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return outIdx;
		}

	private:
		uint in0Idx;
		uint in1Idx;
		uint outIdx;
	};



	class _XnorElement : public _Element
	{
	public:
		_XnorElement(_Module& module, const json11::Json& cell);
		_XnorElement(_Module& module, const _XnorElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 4;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"xnor",
				2, 1,
				1.5f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.75f),
					Vector2f(1.4f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return idx == 0 ? in0Idx : in1Idx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return outIdx;
		}

	private:
		uint in0Idx;
		uint in1Idx;
		uint outIdx;
	};



	class _Mux2Element : public _Element
	{
	public:
		_Mux2Element(_Module& module, const json11::Json& cell);
		_Mux2Element(_Module& module, const _Mux2Element& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 4;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"mux2",
				3, 1,
				1.5f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.5f),
					Vector2f(0.1f, 0.75f),
					Vector2f(1.4f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return idx == 0 ? in0Idx : idx == 1 ? in1Idx : insIdx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return outIdx;
		}

	private:
		uint in0Idx;
		uint in1Idx;
		uint insIdx;
		uint outIdx;
	};


	class _DLatchElement : public _Element
	{
	public:
		_DLatchElement(_Module& module, const json11::Json& cell);
		_DLatchElement(_Module& module, const _DLatchElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual void Reset(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 10;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"dlat",
				2, 1,
				2.0f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.5f),
					Vector2f(1.9f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return idx == 0 ? dIdx : eIdx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return qIdx;
		}

	private:
		State state = State::Zero;;
		uint dIdx;
		uint eIdx;
		uint qIdx;
	};



	class _DFFElement : public _Element
	{
	public:
		_DFFElement(_Module& module, const json11::Json& cell);
		_DFFElement(_Module& module, const _DFFElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual void Reset(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 10;
			stats.numFlipFlops++;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"dff",
				2, 1,
				4.0f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.5f),
					Vector2f(3.8f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			return idx == 0 ? dIdx : cIdx;
		}
		virtual uint GetOutput(uint idx) const
		{
			return qIdx;
		}

	private:
	//	State master = State::Zero;;
	//	State slave  = State::Zero;;
		DLatch master;
		DLatch slave;
		uint cIdx;
		uint dIdx;
		uint qIdx;
	};


	class _ADFFElement : public _Element
	{
	public:
		_ADFFElement(_Module& module, const json11::Json& cell, State _rstLvl, State _rstVal);
		_ADFFElement(_Module& module, const _ADFFElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual void Reset(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 10;
			stats.numFlipFlops++;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"adff",
				3, 1,
				5.0f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.5f),
					Vector2f(3.5f, 0.75f),
					Vector2f(1.0f, 0.75f),
					Vector2f(4.8f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			switch (idx)
			{
			case 0: return cIdx;
			case 1: return dIdx;
			case 2: return rIdx;
			default: __debugbreak();
			}
		}
		virtual uint GetOutput(uint idx) const
		{
			return qIdx;
		}

	private:
		//	State master = State::Zero;;
		//	State slave = State::Zero;;
		DLatch master;
		DLatch slave;
		uint cIdx;
		uint dIdx;
		uint qIdx;
		uint rIdx;
		State rstLvl;
		State rstVal;
	};


	class _DFFSRElement : public _Element
	{
	public:
		_DFFSRElement(_Module& module, const json11::Json& cell);
		_DFFSRElement(_Module& module, const _DFFSRElement& other, uint offset);

		virtual void Proccess(_Module& module) override;
		virtual void Reset(_Module& module) override;
		virtual _ElementPtr Clone(_Module& module, uint offsetIdx) const override;

		virtual void AddStats(Stats& stats) const override
		{
			stats.numMosfets += 10;
			stats.numFlipFlops++;
		}
		virtual const ElementDesc& GetDesc() const
		{
			static ElementDesc desc
			{
				"dffsr",
				4, 1,
				5.0f,
				{
					Vector2f(0.1f, 0.35f),
					Vector2f(0.1f, 0.5f),
					Vector2f(3.5f, 0.75f),
					Vector2f(1.0f, 0.75f),
					Vector2f(4.8f, 0.5f),
				},
			};
			return desc;
		}
		virtual uint GetInput(uint idx) const
		{
			switch (idx)
			{
			case 0: return cIdx;
			case 1: return dIdx;
			case 2: return sIdx;
			case 3: return rIdx;
			default: __debugbreak();
			}
		}
		virtual uint GetOutput(uint idx) const
		{
			return qIdx;
		}

	private:
	//	State master = State::Zero;;
	//	State slave = State::Zero;;
		DLatch master;
		DLatchSR slave;
		uint cIdx;
		uint dIdx;
		uint qIdx;
		uint sIdx;
		uint rIdx;
	};



	class Circuit
	{
	public:
		struct Line
		{
			Vector2f v[2];
		};

		deque<Rectf> cells;

		deque<Vector2f> pads;
		deque<uint>     padToState;

		deque<Line>  wires;
		deque<uint>  wireToState;

		vidf::Rand48 rand48;
		vidf::UniformReal<float> dist{ -0.15f, 0.15f };

		void AddCell(Vector2f p, float width)
		{
			Rectf cell;
			cell.min = p;
			cell.max = cell.min + Vector2f(width, 0.5f);
			cells.push_back(cell);
		}

		void AddPad(Vector2f point, uint stateIdx)
		{
			pads.push_back(point);
			padToState.push_back(stateIdx);
		}

		void Connect(Vector2f begin, Vector2f end, uint stateIdx, uint destIdx)
		{
#if 0
		//	const float rng = ((stateIdx * 8609) % 3851) / float(3851) * 2 - 1.0f;
			const float rng0 = ((stateIdx % 32) / float(32)) * 2.0f - 1.0f;
			const float rng1 = (((stateIdx + destIdx) % 32) / float(32)) * 2.0f - 1.0f;

			Vector2f beginr, endr;
		//	beginr.x = begin.x + dist(rand48);
			beginr.x = begin.x + rng0 * 0.5f;
			beginr.y = begin.y;
			endr.x = end.x;
		//	endr.y = end.y + dist(rand48);
			endr.y = end.y + rng1 * 0.5f;

			Circuit::Line line;
			line.v[0] = begin;
			line.v[1] = beginr;
			wires.push_back(line);
			wireToState.push_back(stateIdx);

			line.v[0] = beginr;
			line.v[1] = Vector2f(beginr.x, endr.y);
			wires.push_back(line);
			wireToState.push_back(stateIdx);

			line.v[0] = line.v[1];
			line.v[1] = endr;
			wires.push_back(line);
			wireToState.push_back(stateIdx);

			line.v[0] = endr;
			line.v[1] = end;
			wires.push_back(line);
			wireToState.push_back(stateIdx);
			
			/*
			Circuit::Line line;
			line.v[0] = begin;
			line.v[1] = end;
			wires.push_back(line);
			wireToState.push_back(stateIdx);
			*/
#endif

			const float rng0 = (((stateIdx % 32) + 1) / float(34)) * 0.5f + 0.25f;

			Circuit::Line line;
			line.v[0] = begin;
			line.v[1] = begin + Vector2f(0.0f, rng0);
			wires.push_back(line);
			wireToState.push_back(stateIdx);

			line.v[0] = line.v[1];
			line.v[1] = Vector2f(end.x, line.v[1].y);
			wires.push_back(line);
			wireToState.push_back(stateIdx);

			line.v[0] = line.v[1];
			line.v[1] = end;
			wires.push_back(line);
			wireToState.push_back(stateIdx);
		}

		void glDraw(const _Module& module) const;
	};



	class _Module
	{
	public:
		typedef map<string, uint> Ports;

		struct SourceElements
		{
			SourceElements*  parent;
			deque<shared_ptr<SourceElements>> children;
			string           name;
			deque<_Element*> elements;
		};

	public:
		_Module() = default;
		_Module(const string& _name, const json11::Json& module)
			: name(_name)
		{
			topSourceElements = make_shared<SourceElements>();
			srcElements["$ROOT"] = topSourceElements;

			const json11::Json& ports = module["ports"];
			const json11::Json& cells = module["cells"];
			const json11::Json& netnames = module["netnames"];

			for (const auto& port : ports.object_items())
			{
				const string& portName = port.first;
				const string& direction = port.second["direction"].string_value();
				assert(port.second["bits"].array_items().size() == 1);	// use "splitnets -ports -format :"
				const uint idx = port.second["bits"].array_items()[0].int_value();
				if (direction == "input" || direction == "inout")
					inputs[portName] = idx;
				else if (direction == "output")
					outputs[portName] = idx;
				else
					__debugbreak();
			}
			for (const auto& cell : cells.object_items())
			{
				const string& type = cell.second["type"].string_value();
				const auto& connections = cell.second["connections"];
				if (type == "$_NOT_" || type == "NOT")
				{
					AddElement<_NotElement>(cell.second);
				}
				else if (type == "BUF")
				{
					AddElement<_BufElement>(cell.second);
				}
				else if (type == "$_TBUF_")
				{
					AddElement<_TBufElement>(cell.second);
				}
				else if (type == "$_NAND_" || type == "NAND")
				{
					AddElement<_NandElement>(cell.second);
				}
				else if (type == "$_NOR_")
				{
					AddElement<_NorElement>(cell.second);
				}
				else if (type == "$_AND_")
				{
					AddElement<_AndElement>(cell.second);
				}
				else if (type == "$_OR_")
				{
					AddElement<_OrElement>(cell.second);
				}
				else if (type == "$_XOR_" || type == "XOR")
				{
					AddElement<_XorElement>(cell.second);
				}
				else if (type == "$_XNOR_" || type == "XNOR")
				{
					AddElement<_XnorElement>(cell.second);
				}
				else if (type == "$_MUX_")
				{
					AddElement<_Mux2Element>(cell.second);
				}
				else if (type == "$_DLATCH_P_")
				{
					AddElement<_DLatchElement>(cell.second);
				}
				else if (type == "$_DFF_P_")
				{
					AddElement<_DFFElement>(cell.second);
				}
				else if (type == "$_DFF_PN0_")
				{
					AddElement<_ADFFElement>(cell.second, State::Zero, State::Zero);
				}
				else if (type == "$_DFF_PN1_")
				{
					AddElement<_ADFFElement>(cell.second, State::Zero, State::One);
				}
				else if (type == "$_DFF_PP0_")
				{
					AddElement<_ADFFElement>(cell.second, State::One, State::Zero);
				}
				else if (type == "$_DFF_PP1_")
				{
					AddElement<_ADFFElement>(cell.second, State::One, State::One);
				}
				else if (type == "$_DFFSR_PPP_")
				{
					AddElement<_DFFSRElement>(cell.second);
				}
				else
				{
					__debugbreak();
				}
			}
			for (const auto& net : netnames.object_items())
			{
				const string& netName = net.first;
				const uint idx = net.second["bits"].array_items()[0].int_value();
				states.resize(max(uint(states.size()), idx + 1));
				const uint hideName = net.second["hide_name"].int_value();
				if (hideName)
					continue;
				if (inputs.find(netName) != inputs.end())
					continue;
				if (outputs.find(netName) != outputs.end())
					continue;
				internals[netName] = idx;
			}
		}
		
		template<typename GateType, typename... Args>
		void AddElement(const json11::Json& cell, Args... args)
		{
			_ElementPtr element = make_shared<GateType>(*this, cell, args...);
			elements.emplace_back(element);

			const auto& attributes = cell["attributes"];
			const string& source = attributes["src"].string_value();
			if (!source.empty())
			{
				shared_ptr<SourceElements> srcElement;
				auto it = srcElements.find(source);
				if (it == srcElements.end())
				{
					srcElement = make_shared<SourceElements>();
					srcElement->name = source;
					srcElements.insert_or_assign(source, srcElement);
				}
				else
					srcElement = it->second;
				srcElement->elements.push_back(element.get());
			}
			else
			{
				topSourceElements->elements.push_back(element.get());
			}
		}

		void AddInput(uint idx, _Element* element)
		{
			assert(element != nullptr);
			states.resize(max(uint(states.size()), idx + 1));
			elementTriggers.resize(states.size());
			elementTriggers[idx].push_back(element);
		}

		void AddOutput(uint idx, _Element* element)
		{
			states.resize(max(uint(states.size()), idx + 1));
		}

		void SetState(uint idx, State state)
		{
			if (states[idx] == state)
				return;
			states[idx] = state;
			triggers.push_back(idx);
		}

		State GetState(uint idx) const
		{
			return states[idx];
		}

		void Step()
		{
			deque<uint> pending;
			while (!triggers.empty())
			{
				pending = triggers;
				triggers.clear();
				for (uint idx : pending)
				{
					for (_Element* element : elementTriggers[idx])
						element->Proccess(*this);
				}
				assert(states[0] == State::Zero);	// these are special pins synthezised by yosys. represent gnd and vcc
				assert(states[1] == State::One);
			}
		}

		void StepAll()
		{
			for (_ElementPtr element : elements)
				element->Proccess(*this);
			Step();
		}

		const Ports& Inputs() const { return inputs; }

		const Ports& Outputs() const { return outputs; }

		const Ports& Internals() const { return internals; }

		Stats CalcStats() const
		{
			Stats stats;
			stats.numGates += elements.size();
			for (_ElementPtr element : elements)
				element->AddStats(stats);
			stats.numLinks = states.size();
			stats.numPorts += inputs.size();
			stats.numPorts += outputs.size();
			return stats;
		}

		void Reset()
		{
			for (State& state : states)
				state = State::One;
			for (auto input : inputs)
				states[input.second] = State::Zero;
			states[0] = State::Zero;
			StepAll();
		}

		void BuildCircuit(Circuit* output)
		{
			Circuit& circuit = *output;

			float offX = 0;
			float offY = 0;
			for (uint elemIdx = 0; elemIdx < elements.size(); ++elemIdx)
			{
				_ElementPtr element = elements[elemIdx];
				const ElementDesc& elemDesc = element->GetDesc();

				const float cellWidth = elemDesc.width;

				if (offX + cellWidth > 40.0f)
				{
					offX = 0;
					offY += 1.0f;
				}

				const Vector2f point = Vector2f(offX, offY);
				circuit.AddCell(Vector2f(offX, offY), cellWidth);
				offX += cellWidth;

				for (uint i = 0; i < elemDesc.numInputs; ++i)
					circuit.AddPad(point + elemDesc.portPositions[i] * Vector2f(1.0f, 0.5f), element->GetInput(i));
				for (uint i = 0; i < elemDesc.numOutputs; ++i)
					circuit.AddPad(point + elemDesc.portPositions[elemDesc.numInputs + i] * Vector2f(1.0f, 0.5f), element->GetOutput(i));
			}

			struct Vertex
			{
				uint elemIdx;
				uint pinIdx;
			};
			deque<deque<Vertex>> sources;
			sources.resize(states.size());
			deque<deque<Vertex>> destinations;
			destinations.resize(states.size());
			for (uint elemIdx = 0; elemIdx < elements.size(); ++elemIdx)
			{
				_ElementPtr element = elements[elemIdx];
				const ElementDesc& elemDesc = element->GetDesc();
				for (uint i = 0; i < elemDesc.numInputs; ++i)
					destinations[element->GetInput(i)].emplace_back(Vertex{ elemIdx, i });
				for (uint i = 0; i < elemDesc.numOutputs; ++i)
					sources[element->GetOutput(i)].emplace_back(Vertex{ elemIdx, elemDesc.numInputs + i });
			}
			for (uint i = 0; i < states.size(); ++i)
			{
				if (sources[i].empty())
					continue;

				const uint srcIdx = sources[i][0].elemIdx;
				const ElementDesc& srcDesc = elements[srcIdx]->GetDesc();
								
				for (uint j = 0; j < destinations[i].size(); ++j)
				{
					const uint destIdx = destinations[i][j].elemIdx;
					const ElementDesc& destDesc = elements[destIdx]->GetDesc();

					const uint srcPinSrc = sources[i][0].pinIdx;
					const uint destPinSrc = destinations[i][j].pinIdx;

					const Vector2f srcCellPos = circuit.cells[srcIdx].min + srcDesc.portPositions[srcPinSrc] * Vector2f(1.0f, 0.5f);
					const Vector2f dstCellPos = circuit.cells[destIdx].min + destDesc.portPositions[destPinSrc] * Vector2f(1.0f, 0.5f);
					
					circuit.Connect(srcCellPos, dstCellPos, i, j);
				}
			}
		}

	private:
		string name;
		deque<_ElementPtr> elements;
		deque<deque<_Element*>> elementTriggers;
		deque<State> states;
		deque<uint> triggers;
		Ports inputs;
		Ports outputs;
		Ports internals;

		unordered_map<string, shared_ptr<SourceElements>> srcElements;
		shared_ptr<SourceElements> topSourceElements;
	};



	void Circuit::glDraw(const _Module& module) const
	{
		glColor4ub(0, 92, 128, 255);
		glBegin(GL_QUADS);
		for (const Rectf& cell : cells)
		{
			glVertex2f(cell.min.x, cell.min.y);
			glVertex2f(cell.max.x, cell.min.y);
			glVertex2f(cell.max.x, cell.min.y);
			glVertex2f(cell.max.x, cell.max.y);
			glVertex2f(cell.max.x, cell.max.y);
			glVertex2f(cell.min.x, cell.max.y);
			glVertex2f(cell.min.x, cell.max.y);
			glVertex2f(cell.min.x, cell.min.y);
		}
		glEnd();

		glLineWidth(2.0f);
		glColor4ub(16, 192, 255, 255);
		glBegin(GL_LINES);
		for (const Rectf& cell : cells)
		{
			glVertex2f(cell.min.x, cell.min.y);
			glVertex2f(cell.max.x, cell.min.y);
			glVertex2f(cell.max.x, cell.min.y);
			glVertex2f(cell.max.x, cell.max.y);
			glVertex2f(cell.max.x, cell.max.y);
			glVertex2f(cell.min.x, cell.max.y);
			glVertex2f(cell.min.x, cell.max.y);
			glVertex2f(cell.min.x, cell.min.y);
		}
		glEnd();

		glLineWidth(1.0f);
		glBegin(GL_LINES);
		for (uint i = 0; i < wires.size(); ++i)
		{
			const State state = module.GetState(wireToState[i]);
		//	if (state == State::Zero)
		//		glColor4ub(255, 128, 128, 255);
		//	else
		//		glColor4ub(128, 255, 128, 255);
			if (state == State::Zero)
				glColor4ub(96, 96, 96, 255);
			else
				glColor4ub(32, 192, 32, 255);
			glVertex(wires[i].v[0]);
			glVertex(wires[i].v[1]);
		}
		glEnd();

		glPointSize(5.0);
		glBegin(GL_POINTS);
		for (uint i = 0; i < pads.size(); ++i)
		{
			const State state = module.GetState(padToState[i]);
		//	if (state == State::Zero)
		//		glColor4ub(255, 0, 0, 255);
		//	else
		//		glColor4ub(0, 255, 0, 255);
			if (state == State::Zero)
				glColor4ub(128, 128, 128, 255);
			else
				glColor4ub(64, 255, 64, 255);
			glVertex(pads[i]);
		}
		glEnd();
	}


		
	_NotElement::_NotElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		inIdx = connections["A"].array_items()[0].int_value();
		outIdx = connections["Y"].array_items()[0].int_value();
		module.AddInput(inIdx, this);
		module.AddOutput(outIdx, this);
	}

	_NotElement::_NotElement(_Module& module, const _NotElement& other, uint offset)
	{
		inIdx = other.inIdx + offset;
		outIdx = other.outIdx + offset;
		module.AddInput(inIdx, this);
		module.AddOutput(outIdx, this);
	}

	void _NotElement::Proccess(_Module& module)
	{
		if (module.GetState(inIdx) == State::Zero)
			module.SetState(outIdx, State::One);
		else
			module.SetState(outIdx, State::Zero);
	}

	_ElementPtr _NotElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_NotElement>(module, *this, offsetIdx);
	}



	_BufElement::_BufElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		inIdx = connections["A"].array_items()[0].int_value();
		outIdx = connections["Y"].array_items()[0].int_value();
		module.AddInput(inIdx, this);
		module.AddOutput(outIdx, this);
	}

	_BufElement::_BufElement(_Module& module, const _BufElement& other, uint offset)
	{
		inIdx = other.inIdx + offset;
		outIdx = other.outIdx + offset;
		module.AddInput(inIdx, this);
		module.AddOutput(outIdx, this);
	}

	void _BufElement::Proccess(_Module& module)
	{
		module.SetState(outIdx, module.GetState(inIdx));
	}

	_ElementPtr _BufElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_BufElement>(module, *this, offsetIdx);
	}



	_TBufElement::_TBufElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		inIdx = connections["A"].array_items()[0].int_value();
		enIdx = connections["E"].array_items()[0].int_value();
		outIdx = connections["Y"].array_items()[0].int_value();
		module.AddInput(inIdx, this);
		module.AddInput(enIdx, this);
		module.AddOutput(outIdx, this);
	}

	_TBufElement::_TBufElement(_Module& module, const _TBufElement& other, uint offset)
	{
	}

	void _TBufElement::Proccess(_Module& module)
	{
		if (module.GetState(enIdx) == State::One)
			module.SetState(outIdx, module.GetState(inIdx));
	}

	_ElementPtr _TBufElement::Clone(_Module& module, uint offsetIdx) const
	{
		return _ElementPtr();
	}



	_NorElement::_NorElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		in0Idx = connections["A"].array_items()[0].int_value();
		in1Idx = connections["B"].array_items()[0].int_value();
		outIdx = connections["Y"].array_items()[0].int_value();
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	_NorElement::_NorElement(_Module& module, const _NorElement& other, uint offset)
	{
		in0Idx = other.in0Idx + offset;
		in1Idx = other.in1Idx + offset;
		outIdx = other.outIdx + offset;
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	void _NorElement::Proccess(_Module& module)
	{
		const State s0 = module.GetState(in0Idx);
		const State s1 = module.GetState(in1Idx);
		if (s0 == State::One || s1 == State::One)
			module.SetState(outIdx, State::Zero);
		else
			module.SetState(outIdx, State::One);
	}

	_ElementPtr _NorElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_NorElement>(module, *this, offsetIdx);
	}



	_OrElement::_OrElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		in0Idx = connections["A"].array_items()[0].int_value();
		in1Idx = connections["B"].array_items()[0].int_value();
		outIdx = connections["Y"].array_items()[0].int_value();
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	_OrElement::_OrElement(_Module& module, const _OrElement& other, uint offset)
	{
		in0Idx = other.in0Idx + offset;
		in1Idx = other.in1Idx + offset;
		outIdx = other.outIdx + offset;
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	void _OrElement::Proccess(_Module& module)
	{
		const State s0 = module.GetState(in0Idx);
		const State s1 = module.GetState(in1Idx);
		if (s0 == State::One || s1 == State::One)
			module.SetState(outIdx, State::One);
		else
			module.SetState(outIdx, State::Zero);
	}

	_ElementPtr _OrElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_OrElement>(module, *this, offsetIdx);
	}



	_AndElement::_AndElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		in0Idx = connections["A"].array_items()[0].int_value();
		in1Idx = connections["B"].array_items()[0].int_value();
		outIdx = connections["Y"].array_items()[0].int_value();
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	_AndElement::_AndElement(_Module& module, const _AndElement& other, uint offset)
	{
		in0Idx = other.in0Idx + offset;
		in1Idx = other.in1Idx + offset;
		outIdx = other.outIdx + offset;
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	void _AndElement::Proccess(_Module& module)
	{
		const State s0 = module.GetState(in0Idx);
		const State s1 = module.GetState(in1Idx);
		if (s0 == State::One && s1 == State::One)
			module.SetState(outIdx, State::One);
		else
			module.SetState(outIdx, State::Zero);
	}

	_ElementPtr _AndElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_AndElement>(module, *this, offsetIdx);
	}



	_NandElement::_NandElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		in0Idx = connections["A"].array_items()[0].int_value();
		in1Idx = connections["B"].array_items()[0].int_value();
		outIdx = connections["Y"].array_items()[0].int_value();
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	_NandElement::_NandElement(_Module& module, const _NandElement& other, uint offset)
	{
		in0Idx = other.in0Idx + offset;
		in1Idx = other.in1Idx + offset;
		outIdx = other.outIdx + offset;
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	void _NandElement::Proccess(_Module& module)
	{
		const State s0 = module.GetState(in0Idx);
		const State s1 = module.GetState(in1Idx);
		if (s0 == State::One && s1 == State::One)
			module.SetState(outIdx, State::Zero);
		else
			module.SetState(outIdx, State::One);
	}

	_ElementPtr _NandElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_NandElement>(module, *this, offsetIdx);
	}



	_XorElement::_XorElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		in0Idx = connections["A"].array_items()[0].int_value();
		in1Idx = connections["B"].array_items()[0].int_value();
		outIdx = connections["Y"].array_items()[0].int_value();
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	_XorElement::_XorElement(_Module& module, const _XorElement& other, uint offset)
	{
		in0Idx = other.in0Idx + offset;
		in1Idx = other.in1Idx + offset;
		outIdx = other.outIdx + offset;
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	void _XorElement::Proccess(_Module& module)
	{
		const State s0 = module.GetState(in0Idx);
		const State s1 = module.GetState(in1Idx);
		if (s0 != s1)
			module.SetState(outIdx, State::One);
		else
			module.SetState(outIdx, State::Zero);
	}

	_ElementPtr _XorElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_XorElement>(module, *this, offsetIdx);
	}



	_XnorElement::_XnorElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		in0Idx = connections["A"].array_items()[0].int_value();
		in1Idx = connections["B"].array_items()[0].int_value();
		outIdx = connections["Y"].array_items()[0].int_value();
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	_XnorElement::_XnorElement(_Module& module, const _XnorElement& other, uint offset)
	{
		in0Idx = other.in0Idx + offset;
		in1Idx = other.in1Idx + offset;
		outIdx = other.outIdx + offset;
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddOutput(outIdx, this);
	}

	void _XnorElement::Proccess(_Module& module)
	{
		const State s0 = module.GetState(in0Idx);
		const State s1 = module.GetState(in1Idx);
		if (s0 != s1)
			module.SetState(outIdx, State::Zero);
		else
			module.SetState(outIdx, State::One);
	}

	_ElementPtr _XnorElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_XnorElement>(module, *this, offsetIdx);
	}



	_Mux2Element::_Mux2Element(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		in0Idx = connections["A"].array_items()[0].int_value();
		in1Idx = connections["B"].array_items()[0].int_value();
		insIdx = connections["S"].array_items()[0].int_value();
		outIdx = connections["Y"].array_items()[0].int_value();
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddInput(insIdx, this);
		module.AddOutput(outIdx, this);
	}

	_Mux2Element::_Mux2Element(_Module& module, const _Mux2Element& other, uint offset)
	{
		in0Idx = other.in0Idx + offset;
		in1Idx = other.in1Idx + offset;
		insIdx = other.insIdx + offset;
		outIdx = other.outIdx + offset;
		module.AddInput(in0Idx, this);
		module.AddInput(in1Idx, this);
		module.AddInput(insIdx, this);
		module.AddOutput(outIdx, this);
	}

	void _Mux2Element::Proccess(_Module& module)
	{
		const State s0 = module.GetState(in0Idx);
		const State s1 = module.GetState(in1Idx);
		const State s = module.GetState(insIdx);
		if (s == State::Zero)
			module.SetState(outIdx, s0);
		else
			module.SetState(outIdx, s1);
	}

	_ElementPtr _Mux2Element::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_Mux2Element>(module, *this, offsetIdx);
	}



	_DLatchElement::_DLatchElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		dIdx = connections["D"].array_items()[0].int_value();
		eIdx = connections["E"].array_items()[0].int_value();
		qIdx = connections["Q"].array_items()[0].int_value();
		module.AddInput(dIdx, this);
		module.AddInput(eIdx, this);
		module.AddOutput(qIdx, this);
	}

	_DLatchElement::_DLatchElement(_Module& module, const _DLatchElement& other, uint offset)
	{
		dIdx = other.dIdx + offset;
		eIdx = other.eIdx + offset;
		qIdx = other.qIdx + offset;
		module.AddInput(dIdx, this);
		module.AddInput(eIdx, this);
		module.AddOutput(qIdx, this);
	}

	void _DLatchElement::Proccess(_Module& module)
	{
		const State d = module.GetState(dIdx);
		const State e = module.GetState(eIdx);
		if (e == State::One)
			state = d;
		module.SetState(qIdx, state);
	}

	void _DLatchElement::Reset(_Module& module)
	{
		state = State::Zero;
		module.SetState(qIdx, state);
	}

	_ElementPtr _DLatchElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_DLatchElement>(module, *this, offsetIdx);
	}


		
	_DFFElement::_DFFElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		cIdx = connections["C"].array_items()[0].int_value();
		dIdx = connections["D"].array_items()[0].int_value();
		qIdx = connections["Q"].array_items()[0].int_value();
		module.AddInput(cIdx, this);
		module.AddInput(dIdx, this);
		module.AddOutput(qIdx, this);
	}

	_DFFElement::_DFFElement(_Module& module, const _DFFElement& other, uint offset)
	{
		cIdx = other.cIdx + offset;
		dIdx = other.dIdx + offset;
		qIdx = other.qIdx + offset;
		module.AddInput(cIdx, this);
		module.AddInput(dIdx, this);
		module.AddOutput(qIdx, this);
	}

	void _DFFElement::Proccess(_Module& module)
	{
		const State clk = module.GetState(cIdx);
		const State d = module.GetState(dIdx);
		master.Step(d, clk);
		slave.Step(master.S(), Not(clk));
		module.SetState(qIdx, slave.S());
	}

	void _DFFElement::Reset(_Module& module)
	{
		master = DLatch();
		slave = DLatch();
		module.SetState(qIdx, slave.S());
	}

	_ElementPtr _DFFElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_DFFElement>(module, *this, offsetIdx);
	}



	_ADFFElement::_ADFFElement(_Module& module, const json11::Json& cell, State _rstLvl, State _rstVal)
		: rstLvl(_rstLvl)
		, rstVal(_rstVal)
	{
		const auto& connections = cell["connections"];
		cIdx = connections["C"].array_items()[0].int_value();
		dIdx = connections["D"].array_items()[0].int_value();
		rIdx = connections["R"].array_items()[0].int_value();
		qIdx = connections["Q"].array_items()[0].int_value();
		module.AddInput(cIdx, this);
		module.AddInput(dIdx, this);
		module.AddInput(rIdx, this);
		module.AddOutput(qIdx, this);
	}

	_ADFFElement::_ADFFElement(_Module& module, const _ADFFElement& other, uint offset)
	{
		cIdx = other.cIdx + offset;
		dIdx = other.dIdx + offset;
		rIdx = other.rIdx + offset;
		qIdx = other.qIdx + offset;
		rstLvl = other.rstLvl;
		rstVal = other.rstVal;
		module.AddInput(cIdx, this);
		module.AddInput(dIdx, this);
		module.AddInput(rIdx, this);
		module.AddOutput(qIdx, this);
	}

	void _ADFFElement::Proccess(_Module& module)
	{
		const State clk = module.GetState(cIdx);
		const State d = module.GetState(dIdx);
		const State r = module.GetState(rIdx);
		master.Step(d, clk);
		if (r == rstLvl)
		{
			slave.Step(rstVal, Not(clk));
			module.SetState(qIdx, rstVal);
		}
		else
		{
			slave.Step(master.S(), Not(clk));
			module.SetState(qIdx, slave.S());
		}
	}

	void _ADFFElement::Reset(_Module& module)
	{
		master = DLatch();
		slave = DLatch();
		module.SetState(qIdx, slave.S());
	}

	_ElementPtr _ADFFElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_ADFFElement>(module, *this, offsetIdx);
	}



	_DFFSRElement::_DFFSRElement(_Module& module, const json11::Json& cell)
	{
		const auto& connections = cell["connections"];
		cIdx = connections["C"].array_items()[0].int_value();
		dIdx = connections["D"].array_items()[0].int_value();
		sIdx = connections["S"].array_items()[0].int_value();
		rIdx = connections["R"].array_items()[0].int_value();
		qIdx = connections["Q"].array_items()[0].int_value();
		module.AddInput(cIdx, this);
		module.AddInput(dIdx, this);
		module.AddInput(sIdx, this);
		module.AddInput(rIdx, this);
		module.AddOutput(qIdx, this);
	//	assert(sIdx == 0 || rIdx == 0);
	}
	
	_DFFSRElement::_DFFSRElement(_Module& module, const _DFFSRElement& other, uint offset)
	{
		cIdx = other.cIdx + offset;
		dIdx = other.dIdx + offset;
		sIdx = other.sIdx + offset;
		rIdx = other.rIdx + offset;
		qIdx = other.qIdx + offset;
		module.AddInput(cIdx, this);
		module.AddInput(dIdx, this);		
		module.AddInput(sIdx, this);
		module.AddInput(rIdx, this);
		module.AddOutput(qIdx, this);
	//	assert(sIdx == 0 || rIdx == 0);
	}

	void _DFFSRElement::Proccess(_Module& module)
	{
		const State clk = module.GetState(cIdx);
		const State d = module.GetState(dIdx);
		const State s = module.GetState(sIdx);
		const State r = module.GetState(rIdx);
		master.Step(d, clk);
		slave.Step(master.S(), Not(clk), s, r);
		module.SetState(qIdx, slave.S());
	}

	void _DFFSRElement::Reset(_Module& module)
	{
		master = DLatch();
		slave = DLatchSR();
		module.SetState(qIdx, slave.S());
	}

	_ElementPtr _DFFSRElement::Clone(_Module& module, uint offsetIdx) const
	{
		return make_shared<_DFFSRElement>(module, *this, offsetIdx);
	}



	const uint maxNumSamples = 128;
	const float widthMult    = 0.35f;
	const float heightMult   = 1.0f;



	bool IsPointInside(Rectf bounds, Vector2f pt)
	{
		if (pt.x < bounds.min.x)
			return false;
		if (pt.y < bounds.min.y)
			return false;
		if (pt.x > bounds.max.x)
			return false;
		if (pt.y > bounds.max.y)
			return false;
		return true;
	}



	class StateGraph
	{
	public:
		StateGraph()
			: port(-1)
		{
		}
		StateGraph(uint _port)
			: port(_port)
		{
		}

		void Update(const _Module& module)
		{
			graph.push_back(module.GetState(port));
			while (graph.size() > maxNumSamples)
				graph.pop_front();
		}

		void Draw(float offset, float weigth) const
		{
			glLineWidth(2.0f);
			glColor4f(weigth * 0.8f, weigth, weigth * 0.8f, 1.0f);
			glBegin(GL_LINE_STRIP);
			for (uint j = 0; j < graph.size(); ++j)
			{
				const float signalOffset = graph[j] == State::One ? 0.15f : 0.85f;
				glVertex2f(j * widthMult, (offset + signalOffset) * heightMult);
				glVertex2f((j + 1) * widthMult, (offset + signalOffset) * heightMult);
			}
			glEnd();
		}

	private:
		deque<State> graph;
		uint port;
	};



	class MultiStateGraph
	{
	private:
		struct Portdata
		{
			Rectf bounds;
			uint  portIdx;
		};

	public:
		MultiStateGraph(const string& _name, float _offset, bool _isInside)
			: name(_name)
			, offset(_offset)
			, isInside(_isInside) {}

		void AddIndexPort(uint pinIdx, uint portIdx)
		{
			const float right = widthMult * maxNumSamples;
			graphs.resize(max(uint(graphs.size()), pinIdx + 1));
			ports.resize(graphs.size());
			graphs[pinIdx] = StateGraph(portIdx);
			ports[pinIdx].bounds.min = Vector2f(right + pinIdx, offset);
			ports[pinIdx].bounds.max = ports[pinIdx].bounds.min + Vector2f(1.0f, 1.0f);
			ports[pinIdx].portIdx = portIdx;
		}

		void FlipPins()
		{
			const uint pinCount = graphs.size();
			for (uint i = 0; i < pinCount / 2; ++i)
				swap(ports[i].bounds, ports[pinCount - i - 1].bounds);
		}

		void Update(const _Module& module)
		{
			for (auto& graph : graphs)
				graph.Update(module);
		}

		void Draw(const _Module& module, Text& textRd) const
		{
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glLineWidth(1.0f);
			const float right = widthMult * maxNumSamples;
			glBegin(GL_QUADS);
			glColor4ub(0, 0, 64, 255);   glVertex2f(0.0f, offset + 0.15f);
			glColor4ub(0, 32, 128, 255); glVertex2f(0.0f, offset + 0.85f);
			glColor4ub(0, 32, 128, 255); glVertex2f(right, offset + 0.85f);
			glColor4ub(0, 0, 64, 255);   glVertex2f(right, offset + 0.15f);
			glEnd();

			for (const auto& port : ports)
				DrawLed(module, port);

			glColor4ub(255, 255, 255, 255);
			textRd.OutputText(Vector2f(right + ports.size(), offset), name.c_str());

			glBlendFunc(GL_SRC_ALPHA, GL_ONE);
			for (auto& graph : graphs)
				graph.Draw(offset, 1.0f / graphs.size());
		}

		void MouseDown(_Module& module, Vector2f mouseCursor)
		{
			if (!isInside)
				return;
			for (uint i = 0; i < ports.size(); ++i)
			{
				if (IsPointInside(ports[i].bounds, mouseCursor))
					module.SetState(ports[i].portIdx, Not(module.GetState(ports[i].portIdx)));
			}
		}

	private:
		void DrawLed(const _Module& module, const Portdata& port) const
		{
			const Rectf bounds = port.bounds;
			const Vector2f center = (port.bounds.min + port.bounds.max) * 0.5f;
			const float radius = 0.45f;
			const float gap = 0.05f;

			if (module.GetState(port.portIdx) == State::Zero)
				glColor4ub(255, 32, 0, 255);
			else
				glColor4ub(32, 255, 32, 255);

			if (!isInside)
			{
				DrawCircle(center, radius, true);
			}
			else
			{
				glBegin(GL_QUADS);
				glVertex2f(bounds.min.x + gap, bounds.min.y + gap);
				glVertex2f(bounds.min.x + gap, bounds.max.y - gap);
				glVertex2f(bounds.max.x - gap, bounds.max.y - gap);
				glVertex2f(bounds.max.x - gap, bounds.min.y + gap);
				glEnd();
			}

			glLineWidth(3.0f);
			glColor4ub(192, 192, 192, 255);
			if (!isInside)
			{
				DrawCircle(center, radius, false);
			}
			else
			{
				glBegin(GL_LINE_LOOP);
				glVertex2f(bounds.min.x + gap, bounds.min.y + gap);
				glVertex2f(bounds.min.x + gap, bounds.max.y - gap);
				glVertex2f(bounds.max.x - gap, bounds.max.y - gap);
				glVertex2f(bounds.max.x - gap, bounds.min.y + gap);
				glEnd();
			}
		}

	private:
		deque<StateGraph> graphs;
		deque<Portdata>   ports;
		string name;
		float  offset;
		bool   isInside;
	};



	class LogicInteraction
	{
	public:
		LogicInteraction(const _Module& module)
		{
			AddPorts(module.Inputs(), true);
			AddPorts(module.Outputs(), false);
			AddPorts(module.Internals(), false);
			for (auto& port : ports)
				port.FlipPins();
		}

		void Update(const _Module& module)
		{
			for (auto& port : ports)
				port.Update(module);
		}

		void Draw(const _Module& module, Text& textRd)
		{
			for (auto& port : ports)
				port.Draw(module, textRd);
		}

		void MouseDown(_Module& module, Vector2f mouseCursor)
		{
			for (auto& port : ports)
				port.MouseDown(module, mouseCursor);
		}

	private:
		void AddPorts(const _Module::Ports& modulePorts, bool isInput)
		{
			unordered_map<string, uint> nameToGraph;
			for (const auto& port : modulePorts)
			{
				string portName = port.first;
				const uint portIdx = port.second;
				auto sepIdx = portName.find(':');
				if (sepIdx == -1)
				{
					ports.emplace_back(portName, float(ports.size()), isInput);
					ports.back().AddIndexPort(0, portIdx);
				}
				else
				{
					const uint pinIdx = atoi(portName.substr(sepIdx + 1, portName.size() - sepIdx - 1).c_str());
					portName = portName.substr(0, sepIdx);
					auto it = nameToGraph.find(portName);
					uint graphIdx;
					if (it == nameToGraph.end())
					{
						graphIdx = ports.size();
						nameToGraph[portName] = graphIdx;
						ports.emplace_back(portName, float(ports.size()), isInput);
					}
					else
						graphIdx = it->second;
					ports[graphIdx].AddIndexPort(pinIdx, portIdx);
				}
			}
		}

	private:
		deque<MultiStateGraph> ports;
	};



	class TestBench
	{
	private:
		struct Port
		{
			string        name;
			string        wireName;
			vector<uint>  bitPorts;
			vector<State> bitStates;
			bool          changed = false;
		};
		struct Scope
		{
			string         name;
			vector<string> portNames;
			vector<string> portWireNames;
			vector<uint>   ports;
			vector<Scope>  children;
		};
		struct Event
		{
			uint  tick;
			uint  portIdx;
			State state;
		};

	public:
		TestBench(_ModulePtr _module, const char* vcdName)
			: module(_module)
			, ofs(vcdName)
		{
			root.name = "test";
			ports.clear();
			CreatePorts(_module->Inputs());
			CreatePorts(_module->Outputs());
			CreatePorts(_module->Internals());
			for (Port& port : ports)
				port.bitStates.resize(port.bitPorts.size());

			module->Reset();

			VcdHeader(ofs);

			for (Port& port : ports)
				port.changed = true;
		}

		~TestBench()
		{
			VcdTick(ofs, numSteps);
		}

		void Simulate(int ticks)
		{
			for (uint i = 0; i < ticks; ++i, ++numSteps)
			{
				if (clockIdx != -1)
					module->SetState(clockIdx, clockState);

				Step();

				VcdTick(ofs, numSteps);
				for (Port& port : ports)
					port.changed = false;

				if (clockIdx != -1)
					clockState = clockState == State::Zero ? State::One : State::Zero;
			}
		}

		void Step()
		{
			module->Step();

			for (Port& port : ports)
			{
				for (uint i = 0; i < port.bitPorts.size(); ++i)
				{
					State curState = module->GetState(port.bitPorts[i]);
					if (curState != port.bitStates[i])
						port.changed = true;
					port.bitStates[i] = curState;
				}
			}
		}

		uint FindPort(const char* name)
		{
			auto it = nameToPort.find(name);
			assert(it != nameToPort.end());
			return it->second;
		}

		void SetState(uint portIdx, uint64 value)
		{
			const Port& port = ports[portIdx];
			for (uint i = 0; i < port.bitPorts.size(); ++i)
			{
				module->SetState(
					port.bitPorts[i],
					(value & (1 << i)) ? State::One : State::Zero);
			}
		}

		uint64 GetState(uint portIdx)
		{
			uint64 result = 0;
			const Port& port = ports[portIdx];
			for (uint i = 0; i < port.bitPorts.size(); ++i)
			{
				State state = module->GetState(port.bitPorts[i]);
				if (state == State::One)
					result |= 1 << i;
			}
			return result;
		}

		void SetState(uint tick, const char* name, uint value)
		{
			AddEvent(events, tick, name, value);
		}

		void Assert(uint tick, const char* name, uint value)
		{
			AddEvent(asserts, tick, name, value);
		}

		void SetClock(const char* name, State initState)
		{
			auto it = nameToPort.find(name);
			assert(it != nameToPort.end());
			assert(ports[it->second].bitPorts.size() == 1);
			clockIdx = ports[it->second].bitPorts[0];
			clockState = initState;
		}

	private:
		void CreatePorts(const _Module::Ports& modulePorts)
		{
			char buff[16];
			for (const auto& port : modulePorts)
			{
				string portName = port.first;
				const uint portIdx = port.second;
				auto sepIdx = portName.find(':');
				if (sepIdx == -1)
				{
					Port port;
					port.name = portName;
					sprintf_s(buff, "n%d", ports.size() + 1);
					port.wireName = buff;
					port.bitPorts.push_back(portIdx);
					nameToPort[portName] = ports.size();
					FindScope(portName, port.wireName).ports.push_back(ports.size());
					ports.push_back(port);
				}
				else
				{
					const uint pinIdx = atoi(portName.substr(sepIdx + 1, portName.size() - sepIdx - 1).c_str());
					portName = portName.substr(0, sepIdx);
					auto it = nameToPort.find(portName);
					if (it == nameToPort.end())
					{
						Port port;
						port.name = portName;
						sprintf_s(buff, "n%d", ports.size() + 1);
						port.wireName = buff;						
						nameToPort[portName] = ports.size();
						FindScope(portName, port.wireName).ports.push_back(ports.size());
						it = nameToPort.find(portName);
						ports.push_back(port);
					}
					ports[it->second].bitPorts.resize(max(pinIdx + 1, uint(ports[it->second].bitPorts.size())));
					ports[it->second].bitPorts[pinIdx] = portIdx;
				}
			}
		}

		void UpdatePorts(uint tick)
		{
			while (!events.empty())
			{
				const Event& event = events.back();
				if (event.tick > tick)
					break;
				module->SetState(event.portIdx, event.state);
				events.pop_back();
			}
		}

		void CheckAsserts(uint tick)
		{
			while (!asserts.empty())
			{
				const Event& event = asserts.back();
				if (event.tick > tick)
					break;
				const State current = module->GetState(event.portIdx);
				const State expected = event.state;
				if (current != expected)
				{
					__debugbreak();
				}
				asserts.pop_back();
			}
		}

		void VcdHeader(ostream& os) const
		{
			VcdScope(os, root);
			os << "$enddefinitions $end" << endl;
		}

		void VcdScope(ostream& os, const Scope& scope) const
		{
			os << "$scope module " << scope.name << " $end" << endl;
			for (uint i = 0; i < scope.portNames.size(); ++i)
			{
				const Port& port = ports[scope.ports[i]];
				os << "$var wire ";
				os << port.bitPorts.size() << " ";
				os << scope.portWireNames[i] << " ";
				os << scope.portNames[i] << " ";
				os << "$end";
				os << endl;
			}
			for (const Scope& child : scope.children)
				VcdScope(os, child);
			os << "$upscope $end" << endl;
		}

		void VcdTick(ostream& os, uint tick) const
		{
			os << "#" << tick * 5 << endl;
			for (const Port& port : ports)
			{
				if (!port.changed)
					continue;
				os << "b";
				for (int i = port.bitPorts.size() - 1; i >= 0; --i)
					os << (module->GetState(port.bitPorts[i]) == State::One ? '1' : '0');
				os << " ";
				os << port.wireName;
				os << endl;
			}
		}

		void AddEvent(vector<Event>& _events, uint tick, const char* name, uint value)
		{
			auto it = nameToPort.find(name);
			assert(it != nameToPort.end());
			const Port& port = ports[it->second];
			Event event;
			event.tick = tick;
			for (uint i = 0; i < port.bitPorts.size(); ++i)
			{
				event.portIdx = port.bitPorts[i];
				event.state = (value & (1 << i)) ? State::One : State::Zero;
				_events.push_back(event);
			}
		}

		Scope& FindScope(const string& name, const string& wireName)
		{
			if (name.find('.') == -1)
			{
				root.portNames.push_back(name);
				root.portWireNames.push_back(wireName);
				return root;
			}
			else
				return FindScope(root, name, wireName);
		}

		Scope& FindScope(Scope& node, const string& name, const string& wireName)
		{
			auto idx = name.find('.');
			string nodeName = name.substr(0, idx);
			string rest = name.substr(idx + 1, name.size() - idx - 1);

			auto it = find_if(node.children.begin(), node.children.end(),
			[&nodeName](const Scope& scope)
			{
				return scope.name == nodeName;
			});
			if (it == node.children.end())
			{
				Scope scope;
				scope.name = nodeName;
				node.children.push_back(scope);
				it = node.children.end() - 1;
			}

			if (rest.find('.') == -1)
			{				
				it->portNames.push_back(rest);
				it->portWireNames.push_back(wireName);
				return *it;
			}
			else
				return FindScope(*it, rest, wireName);
		}

	private:
		ofstream     ofs;
		uint         numSteps = 0;
		_ModulePtr   module;
		unordered_map<string, uint> nameToPort;
		Scope        root;
		vector<Port> ports;
		vector<Event> events;
		vector<Event> asserts;
		uint  clockIdx = -1;
		State clockState = State::Zero;
	};


	class KC8
	{
	public:
		KC8(_ModulePtr module, const char* vcdFile)
			: testBench(module, vcdFile)
		{
			testBench.SetClock("clk", State::Zero);

			memory.resize(1 << 16);

			iodb = testBench.FindPort("iodb");
			abl = testBench.FindPort("abl");
			abh = testBench.FindPort("abh");
			rw = testBench.FindPort("rw");
			rst = testBench.FindPort("rst");

			labels["_main"] = 0x10;
		}

		void Reset()
		{
			steps = 0;
		}

		void Step(bool reset)
		{
			testBench.SetState(rst, steps < 2 || reset);
			testBench.Simulate(1);
			uint addr = (testBench.GetState(abh) << 8) | testBench.GetState(abl);
			if (testBench.GetState(rw))
				memory[addr] = testBench.GetState(iodb);
			else
				testBench.SetState(iodb, memory[addr]);
			steps++;
		}

		void SetLabel(const char* label)
		{
			auto it = labels.find(label);
			assert(it != labels.end());
			curAddr = it->second;
		}

		void MemSet(uint addr, uint8 value)
		{
			memory[addr] = value;
		}

		void MemPush(uint8 value)
		{
			memory[curAddr++] = value;
		}

	// private:
		vector<uint8> memory;
		TestBench     testBench;
		uint          steps = 0;

		unordered_map<const char*, uint> labels;
		uint curAddr = 0;

		uint iodb;
		uint abl;
		uint abh;
		uint rw;
		uint rst;
	};



	void Logic()
	{
		using namespace json11;
	//	ifstream ifs{ "data/test.json" };
		ifstream ifs{ "data/logic/kc8/kc8.json" };
	//	ifstream ifs{ "data/logic/test/test.json" };
		string text;
		while (!ifs.eof())
		{
			char buff[1024]{};
			ifs.read(buff, 1023);
			text += buff;
		}
		string err;
		Json json = Json::parse(text.c_str(), err);
		map<string, _ModulePtr> modules;
		for (const auto& module : json["modules"].object_items())
		{
			const string& moduleName = module.first;
			_ModulePtr newModule = make_shared<_Module>(moduleName, module.second);
			modules[moduleName] = newModule;
		}

		// _ModulePtr module = modules["counter"];
		// _ModulePtr module = modules["doublecounter"];
		// _ModulePtr module = modules["adder"];
		// _ModulePtr module = modules["mult"];
		// _ModulePtr module = modules["ALU"];
		// _ModulePtr module = modules["CPU"];
		// _ModulePtr module = modules["test1"];
		// _ModulePtr module = modules["test2"];
		// _ModulePtr module = modules["Register"];
		// _ModulePtr module = modules["ProgramCounter"];
		// _ModulePtr module = modules["RegisterBank"];
		// _ModulePtr module = modules["Control"];
		_ModulePtr module = modules["CPU"];
		// _ModulePtr module = modules["Register"];
		// _ModulePtr module = modules["test"];
		Stats stats = module->CalcStats();
		module->Reset();
		auto inputs = module->Inputs();
		auto outputs = module->Outputs();

		Circuit circuit;
		module->BuildCircuit(&circuit);

		for (const auto& mod : modules)
		{
			Stats stats = mod.second->CalcStats();
			cout << mod.first << ":" << endl;
			cout << "  Gates     : " << stats.numGates << endl;
			cout << "  FlipFlops : " << stats.numFlipFlops << endl;
			cout << "  Tbuffs    : " << stats.numTbuffs << endl;
			cout << "  Mosfets   : " << stats.numMosfets << endl;
			cout << "  Links     : " << stats.numLinks << endl;
			cout << "  Ports     : " << stats.numPorts << endl;
			cout << "  FlipFlops : " << stats.numFlipFlops << endl;
			cout << endl;
		}
		
		const uint maxNumSamples = 128;
		typedef deque<State> StateGraph;
		typedef unordered_map<uint, StateGraph> StateGraphs;
		typedef unordered_map<uint, uint> InputIdx;
		StateGraphs stateGraphs;

		InputIdx inputIds;

		{
			uint i = 0;
			for (auto input : inputs)
			{
				const uint idx = input.second;
				stateGraphs[idx] = StateGraph{};
				inputIds[i++] = idx;
			}
		}
		for (auto output : outputs)
		{
			const uint idx = output.second;
			stateGraphs[idx] = StateGraph{};
		}
		
		bool hasClock = false;
		auto clockIt = inputs.find("clk");
		auto rstIt = inputs.find("rst");
		uint clockIdx;
		if (clockIt != inputs.end())
		{
			clockIdx = clockIt->second;
			hasClock = true;
		}

#if 1

#		define LG_ASSERT_EQUAL(a, b) \
			if ((a) != (b)) { \
				cout << dec << __FILE__ << "(" << __LINE__ << hex << ") : Assert Failed : found 'h" << a << "', expected 'h" << b << "'" << endl; }

		{
			TestBench testBench{ modules["RegisterBank"], "data/logic/kc8/RegisterBank.vcd" };
			testBench.SetClock("clk", State::Zero);
						
			const uint dbe = testBench.FindPort("dbe");
			const uint sbe = testBench.FindPort("sbe");
			const uint dba = testBench.FindPort("dba");
			const uint sba = testBench.FindPort("sba");
			const uint dbld = testBench.FindPort("dbld");
			const uint db = testBench.FindPort("db");
			const uint sb = testBench.FindPort("sb");
			const uint dbi = testBench.FindPort("dbi");

			// load data from data bus to r1 and check db
			const uint value0 = 0xfe;
			testBench.SetState(dbe, 1);
			testBench.SetState(dbi, value0);
			testBench.SetState(dba, 0b01);
			testBench.SetState(dbld, 1);
			testBench.Simulate(2);
			testBench.SetState(dbld, 0);
			testBench.Simulate(2);			// clock again so db gets updated
			LG_ASSERT_EQUAL(testBench.GetState(db), value0);

			// load data from data bus to r0 and check sb
			const uint value1 = 0xcc;
			testBench.SetState(sbe, 1);
			testBench.SetState(dbi, value1);
			testBench.SetState(dba, 0b00);
			testBench.SetState(sba, 0b00);
			testBench.SetState(dbld, 1);
			testBench.Simulate(2);
			testBench.SetState(dbld, 0);
			testBench.Simulate(2);			// clock again so sb gets updated
			LG_ASSERT_EQUAL(testBench.GetState(sb), value1);

			// set r0 to db and r1 to sb
			testBench.SetState(dba, 0b00);
			testBench.SetState(sba, 0b01);
			testBench.Simulate(2);
			LG_ASSERT_EQUAL(testBench.GetState(db), value1);
			LG_ASSERT_EQUAL(testBench.GetState(sb), value0);
		}

		{
			TestBench testBench{ modules["ProgramCounter"], "data/logic/kc8/ProgramCounter.vcd" };
			testBench.SetClock("clk", State::Zero);

			const uint rst = testBench.FindPort("rst");
			const uint pcl_abl = testBench.FindPort("pcl_abl");
			const uint pch_abh = testBench.FindPort("pch_abh");
			const uint abl_pcl = testBench.FindPort("abl_pcl");
			const uint abh_pch = testBench.FindPort("abh_pch");
			const uint db = testBench.FindPort("db");
			const uint abl = testBench.FindPort("abl");
			const uint abli = testBench.FindPort("abli");
			const uint abh = testBench.FindPort("abh");
			const uint abhi = testBench.FindPort("abhi");
			const uint pci = testBench.FindPort("pci");
			
			// by default, PC should be set to 0x0010 when reset
			// set both registers to drive address bus
			testBench.SetState(rst, 1);
			testBench.SetState(pcl_abl, 1);
			testBench.SetState(pch_abh, 1);
			testBench.Simulate(2);
			testBench.SetState(rst, 0);
			LG_ASSERT_EQUAL(testBench.GetState(abl), 0x10);
			LG_ASSERT_EQUAL(testBench.GetState(abh), 0x00);

			// increment PC
			testBench.SetState(pci, 1);
			testBench.Simulate(2);
			testBench.SetState(pci, 0);
		//	testBench.Simulate(2);
			LG_ASSERT_EQUAL(testBench.GetState(abl), 0x11);
			LG_ASSERT_EQUAL(testBench.GetState(abh), 0x00);

			// load PCL from ABL
			testBench.SetState(pcl_abl, 0);
			testBench.SetState(abl_pcl, 1);
			testBench.SetState(abli, 0xff);
			testBench.Simulate(2);
			testBench.SetState(pcl_abl, 1);
			testBench.SetState(abl_pcl, 0);
			testBench.Simulate(2);
			LG_ASSERT_EQUAL(testBench.GetState(abl), 0xff);

			// load PCH from ABH
			testBench.SetState(pch_abh, 0);
			testBench.SetState(abh_pch, 1);
			testBench.SetState(abhi, 0x20);
			testBench.Simulate(2);
			testBench.SetState(pch_abh, 1);
			testBench.SetState(abh_pch, 0);
			testBench.Simulate(2);
			LG_ASSERT_EQUAL(testBench.GetState(abh), 0x20);

			// test boundary condition (pcl is 0xff atm)
			testBench.SetState(pci, 1);
			testBench.Simulate(2);
			testBench.SetState(pci, 0);
			testBench.Simulate(2);
			LG_ASSERT_EQUAL(testBench.GetState(abl), 0x00);
			LG_ASSERT_EQUAL(testBench.GetState(abh), 0x21);

			testBench.SetState(pci, 1);
		}

#if 0
		{
			TestBench testBench{ modules["Control"], "data/logic/kc8/Control.vcd" };
			testBench.SetClock("clk", State::Zero);

			const uint rst = testBench.FindPort("rst");
			const uint db = testBench.FindPort("db");
			const uint pci = testBench.FindPort("pci");
			const uint dl_db = testBench.FindPort("dl_db");
			const uint pcl_abl = testBench.FindPort("pcl_abl");
			const uint pch_abh = testBench.FindPort("pch_abh");
			const uint op = testBench.FindPort("op");
			const uint alue = testBench.FindPort("alue");
			const uint dba = testBench.FindPort("dba");
			const uint sba = testBench.FindPort("sba");
			const uint dbe = testBench.FindPort("dbe");
			const uint sbe = testBench.FindPort("sbe");
			const uint dbld = testBench.FindPort("dbld");
			const uint rw = testBench.FindPort("rw");
			const uint ral_adl = testBench.FindPort("ral_adl");
			const uint rah_adh = testBench.FindPort("rah_adh");

			// test: sub r2, r1 (1001 01 10)
			{
				// reset and T0
				testBench.SetState(rst, 1);
				testBench.Simulate(2);
				testBench.SetState(rst, 0);
				LG_ASSERT_EQUAL(testBench.GetState(pci), 1);

				// T1
				testBench.SetState(db, 0b10010110);
				testBench.Simulate(2);
				LG_ASSERT_EQUAL(testBench.GetState(pcl_abl), 1);
				LG_ASSERT_EQUAL(testBench.GetState(pch_abh), 1);
				LG_ASSERT_EQUAL(testBench.GetState(dl_db), 1);
				LG_ASSERT_EQUAL(testBench.GetState(rw), 0);

				// T2
				testBench.Simulate(2);
				LG_ASSERT_EQUAL(testBench.GetState(op), 0b001);
				LG_ASSERT_EQUAL(testBench.GetState(dba), 0b010);
				LG_ASSERT_EQUAL(testBench.GetState(sba), 0b001);
				LG_ASSERT_EQUAL(testBench.GetState(dbe), 1);
				LG_ASSERT_EQUAL(testBench.GetState(sbe), 1);

				// T3
				testBench.Simulate(2);
				LG_ASSERT_EQUAL(testBench.GetState(alue), 1);
				LG_ASSERT_EQUAL(testBench.GetState(dba), 0b010);
				LG_ASSERT_EQUAL(testBench.GetState(dbld), 1);
			}

			// test: st r2 (0100 00 10)
			{
				// T0
				testBench.Simulate(2);
				LG_ASSERT_EQUAL(testBench.GetState(pci), 1);

				// T1
				testBench.SetState(db, 0b01000010);
				testBench.Simulate(2);
				LG_ASSERT_EQUAL(testBench.GetState(pcl_abl), 1);
				LG_ASSERT_EQUAL(testBench.GetState(pch_abh), 1);
				LG_ASSERT_EQUAL(testBench.GetState(dl_db), 1);
				LG_ASSERT_EQUAL(testBench.GetState(rw), 0);

				// T2
				testBench.Simulate(2);
				LG_ASSERT_EQUAL(testBench.GetState(dba), 0b10);
				LG_ASSERT_EQUAL(testBench.GetState(dbe), 1);
				LG_ASSERT_EQUAL(testBench.GetState(dbld), 0);
				LG_ASSERT_EQUAL(testBench.GetState(dl_db), 0);
				LG_ASSERT_EQUAL(testBench.GetState(ral_adl), 1);
				LG_ASSERT_EQUAL(testBench.GetState(rah_adh), 1);
				LG_ASSERT_EQUAL(testBench.GetState(rw), 1);
			}

			// test: ld r1 (0010 00 01)
			{
				// T0
				testBench.Simulate(2);
				LG_ASSERT_EQUAL(testBench.GetState(pci), 1);

				// T1
				testBench.SetState(db, 0b00100001);
				testBench.Simulate(2);
				LG_ASSERT_EQUAL(testBench.GetState(pcl_abl), 1);
				LG_ASSERT_EQUAL(testBench.GetState(pch_abh), 1);
				LG_ASSERT_EQUAL(testBench.GetState(dl_db), 1);
				LG_ASSERT_EQUAL(testBench.GetState(rw), 0);

				// T2
				testBench.Simulate(2);
				LG_ASSERT_EQUAL(testBench.GetState(dba), 0b01);
				LG_ASSERT_EQUAL(testBench.GetState(dbe), 0);
				LG_ASSERT_EQUAL(testBench.GetState(dbld), 1);
				LG_ASSERT_EQUAL(testBench.GetState(dl_db), 1);
				LG_ASSERT_EQUAL(testBench.GetState(ral_adl), 1);
				LG_ASSERT_EQUAL(testBench.GetState(rah_adh), 1);
				LG_ASSERT_EQUAL(testBench.GetState(rw), 0);
			}

			testBench.Simulate(10);
		}
#endif

#endif

		enum Reg
		{
			R0,
			R1,
			R2,
			R3,
		};

		enum ALU
		{
			Add,
			Sub,
			And,
			Or,
			Neg,
			Shl,
			Shr,
		};

		auto alu = [](ALU alu, Reg a, Reg b)
		{
			return 0;
		};
				
		KC8 kc8{ module, "data/logic/kc8/cpu.vcd" };
		{
			kc8.SetLabel("_main");
			kc8.MemPush(alu(Add, R0, R1));
			kc8.MemPush(0b10010110);
			kc8.MemPush(0b01000010);

		//	for (uint i = 0; i < 64; ++i)
		//		kc8.Step();
		}

		LogicInteraction logicInteraction{ *module };

		ProtoGL protoGL;
		protoGL.Initialize(ProtoGLDesc(1280, 720));
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);

		CameraOrtho2D camera2D(protoGL.GetCanvas(), CameraOrtho2D::Downward);
		camera2D.SetCamera(Vector2f(zero), 30.0f);

		Text textRd;
		textRd.Init(0.2f);
		textRd.size = 0.7f;
		textRd.flip = true;

		TimeCounter timer;
		Time nextClock{ 0.0f };
	//	Time period{ 0.2f };
	//	Time period{ 1.0f / 120.0f };
		Time period{ 1.0f / 30.0f };
		Time time{ 0.0f };
		bool stepSim = false;

		bool mouseState = false;

		auto IsPointInside = [](Rectf bounds, Vector2f pt)
		{
			if (pt.x < bounds.min.x)
				return false;
			if (pt.y < bounds.min.y)
				return false;
			if (pt.x > bounds.max.x)
				return false;
			if (pt.y > bounds.max.y)
				return false;
			return true;
		};

		while (protoGL.Update())
		{
			/*
			const float widthMult = 0.35f;
			const float heightMult = 1.0f;

			bool mousePushed = false;
			bool mouseReleased = false;
			Vector2f mousePos = camera2D.CursorPosition();
			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
			{
				mousePushed = !mouseState;
				mouseState = true;
			}
			else
			{
				mouseReleased = mouseState;
				mouseState = false;
			}

			if (mousePushed)
			{
				logicInteraction.MouseDown(*module, mousePos);
				stepSim = true;
			}
			*/
			
			if (hasClock)
			{
				Time deltaTime = timer.GetElapsed();
				nextClock += deltaTime;
				time += Time(deltaTime.AsFloat() * period.AsFloat());
				while (nextClock.GetTicks() > period.GetTicks())
				{
					nextClock -= period;
					module->SetState(clockIdx, Not(module->GetState(clockIdx)));
					kc8.Step(GetAsyncKeyState('R') && 0x8000);
				//	logicInteraction.Update(*module);
				}
			}
			else if (stepSim)
			{
				module->StepAll();
				logicInteraction.Update(*module);
			}
			stepSim = false;

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			camera2D.CommitToGL();

		//	logicInteraction.Draw(*module, textRd);
			glPushMatrix();
		//	glTranslatef(60.0f, 0.0f, 0.0f);
			circuit.glDraw(*module);
			glPopMatrix();

			protoGL.Swap();
		}
	}



	void __Logic()
	{
		LogicBlockPtr logicBlock = make_shared<LogicBlock>("Schematic");

		/*
		LogicBlockPtr logicBlock = make_shared<LogicBlock>("Basic Test");

		logicBlock->AddElement<Button>(Vector2i(0, 0));
		logicBlock->AddElement<NotGate>(Vector2i(3, 0));
		logicBlock->AddElement<Led>(Vector2i(6, 0));

		logicBlock->AddEdge(Vector2i(2, 1), Vector2i(3, 1));
		logicBlock->AddEdge(Vector2i(5, 1), Vector2i(6, 1));
		*/

		///////////////////////////////////////////////
		/*
		LogicBlockPtr halfAdder = make_shared<LogicBlock>("HalfAdder");

		halfAdder->AddElement<Button>(false, Vector2i(-6, 6));
		halfAdder->AddElement<Button>(false, Vector2i(-6, 2));
		halfAdder->AddPort(Vector2i(-3, 6), VertexType::Input, "A");
		halfAdder->AddPort(Vector2i(-3, 2), VertexType::Input, "B");
		halfAdder->AddEdge(false, Vector2i(-4, 7), Vector2i(-3, 7));
		halfAdder->AddEdge(false, Vector2i(-4, 3), Vector2i(-3, 3));

		halfAdder->AddElement<Gate>(true, Vector2i(0, 4), GateType::Xor, 2);
		halfAdder->AddElement<Gate>(true, Vector2i(0, 0), GateType::And, 2);
		halfAdder->AddEdge(true, Vector2i(-2, 3), Vector2i(-2, 1));
		halfAdder->AddEdge(true, Vector2i(-2, 1), Vector2i(0, 1));
		halfAdder->AddEdge(true, Vector2i(-2, 3), Vector2i(-2, 1));
		halfAdder->AddEdge(true, Vector2i(-2, 3), Vector2i(-2, 5));
		halfAdder->AddEdge(true, Vector2i(-2, 5), Vector2i(0, 5));
		halfAdder->AddEdge(true, Vector2i(-2, 7), Vector2i(-1, 7));
		halfAdder->AddEdge(true, Vector2i(-1, 7), Vector2i(0, 7));
		halfAdder->AddEdge(true, Vector2i(-1, 7), Vector2i(-1, 3));
		halfAdder->AddEdge(true, Vector2i(-1, 3), Vector2i(0, 3));

		halfAdder->AddPort(Vector2i(5, 6), VertexType::Output, "S");
		halfAdder->AddPort(Vector2i(5, 2), VertexType::Output, "C");
		halfAdder->AddEdge(true, Vector2i(5, 6), Vector2i(5, 7));
		halfAdder->AddEdge(true, Vector2i(5, 2), Vector2i(5, 3));

		halfAdder->AddElement<Led>(false, Vector2i(7, 6));
		halfAdder->AddElement<Led>(false, Vector2i(7, 2));
		halfAdder->AddEdge(false, Vector2i(6, 7), Vector2i(7, 7));
		halfAdder->AddEdge(false, Vector2i(6, 3), Vector2i(7, 3));
		*/

	//	MakeHalfAdder(logicBlock, true);
	//	MakeFullAdder(logicBlock, true);
	//	MakeIntegerAdder(logicBlock, 8, true);
	//	MakeSRLatch(logicBlock, true);
	//	MakeDecoder(logicBlock, 4, 4, true);
		MakeDecoder(logicBlock, 1 << 3, 4, true);

		///////////////////////////////////////////////
		
		/*
		LogicBlockPtr logicBlock = make_shared<LogicBlock>("Test Gates");

		logicBlock->AddElement<Button>(Vector2i(0, 0));
		logicBlock->AddElement<Button>(Vector2i(0, 2));
		logicBlock->AddElement<Gate>(Vector2i(4, 0), GateType::And, 2);
		logicBlock->AddElement<Gate>(Vector2i(4, 4), GateType::Nand, 2);
		logicBlock->AddElement<Gate>(Vector2i(4, 8), GateType::Or, 2);
		logicBlock->AddElement<Gate>(Vector2i(4, 12), GateType::Nor, 2);
		logicBlock->AddElement<Gate>(Vector2i(4, 16), GateType::Xor, 2);
		logicBlock->AddElement<Gate>(Vector2i(4, 20), GateType::Nxor, 2);
		logicBlock->AddElement<Led>(Vector2i(10, 1));
		logicBlock->AddElement<Led>(Vector2i(10, 5));
		logicBlock->AddElement<Led>(Vector2i(10, 9));
		logicBlock->AddElement<Led>(Vector2i(10, 13));
		logicBlock->AddElement<Led>(Vector2i(10, 17));
		logicBlock->AddElement<Led>(Vector2i(10, 21));

		logicBlock->AddEdge(Vector2i(2, 1), Vector2i(3, 1));
		logicBlock->AddEdge(Vector2i(3, 1), Vector2i(4, 1));
		logicBlock->AddEdge(Vector2i(2, 3), Vector2i(4, 3));
		logicBlock->AddEdge(Vector2i(3, 1), Vector2i(3, 5));
		logicBlock->AddEdge(Vector2i(3, 5), Vector2i(3, 9));
		logicBlock->AddEdge(Vector2i(3, 9), Vector2i(3, 13));
		logicBlock->AddEdge(Vector2i(3, 13), Vector2i(3, 17));
		logicBlock->AddEdge(Vector2i(3, 17), Vector2i(3, 21));
		logicBlock->AddEdge(Vector2i(3, 5), Vector2i(4, 5));
		logicBlock->AddEdge(Vector2i(3, 9), Vector2i(4, 9));
		logicBlock->AddEdge(Vector2i(3, 13), Vector2i(4, 13));
		logicBlock->AddEdge(Vector2i(3, 17), Vector2i(4, 17));
		logicBlock->AddEdge(Vector2i(3, 21), Vector2i(4, 21));
		logicBlock->AddEdge(Vector2i(2, 3), Vector2i(2, 7));
		logicBlock->AddEdge(Vector2i(2, 7), Vector2i(2, 11));
		logicBlock->AddEdge(Vector2i(2, 11), Vector2i(2, 15));
		logicBlock->AddEdge(Vector2i(2, 15), Vector2i(2, 19));
		logicBlock->AddEdge(Vector2i(2, 19), Vector2i(2, 23));
		logicBlock->AddEdge(Vector2i(2, 7), Vector2i(4, 7));
		logicBlock->AddEdge(Vector2i(2, 11), Vector2i(4, 11));
		logicBlock->AddEdge(Vector2i(2, 15), Vector2i(4, 15));
		logicBlock->AddEdge(Vector2i(2, 19), Vector2i(4, 19));
		logicBlock->AddEdge(Vector2i(2, 23), Vector2i(4, 23));
		logicBlock->AddEdge(Vector2i(9, 2), Vector2i(10, 2));
		logicBlock->AddEdge(Vector2i(9, 6), Vector2i(10, 6));
		logicBlock->AddEdge(Vector2i(9, 10), Vector2i(10, 10));
		logicBlock->AddEdge(Vector2i(9, 14), Vector2i(10, 14));
		logicBlock->AddEdge(Vector2i(9, 18), Vector2i(10, 18));
		logicBlock->AddEdge(Vector2i(9, 22), Vector2i(10, 22));
		*/

		///////////////////////////////////////////////
		/*
		LogicBlockPtr fullAdder = make_shared<LogicBlock>("FullAdder");

		fullAdder->AddPort(Vector2i(0, 6), VertexType::Input, "A");
		fullAdder->AddPort(Vector2i(0, 4), VertexType::Input, "B");
		fullAdder->AddPort(Vector2i(0, 2), VertexType::Input, "Cin");
		fullAdder->AddBlockLogic(true, Vector2i(3, 0), halfAdder);
		*/
		///////////////////////////////////////////////

		EdgeMap edgeMap;
		logicBlock->AddToEdgeMap(&edgeMap);

		///////////////////////////////////////////////

		ProtoGL protoGL;
		protoGL.Initialize(ProtoGLDesc(1280, 720));
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);

		CameraOrtho2D camera2D(protoGL.GetCanvas(), CameraOrtho2D::Upward);
		camera2D.SetCamera(Vector2f(zero), 30.0f);

		bool mouseState = false;

		while (protoGL.Update())
		{
			bool mousePushed = false;
			bool mouseReleased = false;
			Vector2f mousePos = camera2D.CursorPosition();
			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
			{
				mousePushed = !mouseState;
				mouseState = true;
			}
			else
			{
				mouseReleased = mouseState;
				mouseState = false;
			}

			if (mousePushed)
				logicBlock->MouseDown(&edgeMap, mousePos);
			if (mouseReleased)
				logicBlock->MouseUp(&edgeMap, mousePos);

			edgeMap.UpdateStates();

			glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			camera2D.CommitToGL();

			{
				glLineWidth(1.0f);
				glBegin(GL_LINES);
				glColor4ub(235, 235, 235, 255);
				for (int i = -1024; i < 1024; ++i)
				{
					glVertex2f(-1024.0f, i); glVertex2f(1024.0f, i);
					glVertex2f(i, -1024.0f); glVertex2f(i, 1024.0f);
				}
				glColor4ub(196, 196, 196, 255);
				for (int i = -1000; i < 1000; i += 10)
				{
					glVertex2f(-1024.0f, i); glVertex2f(1024.0f, i);
					glVertex2f(i, -1024.0f); glVertex2f(i, 1024.0f);
				}
				glEnd();
			}

			edgeMap.Draw();
			logicBlock->Draw();

			protoGL.Swap();
			glEnd();
		}
	}



}


namespace track
{


	class Segment;
	typedef shared_ptr<Segment> SegmentPtr;



	struct Transform
	{
		Vector3f position;
		Complexf rotation;

		Vector3f Trans(Vector3f v) const
		{
			return Vector3f(
				rotation.r * v.x - rotation.i * v.y,
				rotation.r * v.y + rotation.i * v.x,
				v.z) + position;
		}
		Vector3f Front() const
		{
			return Vector3f(rotation.r, rotation.i, 0.0f);
		}
		Vector3f Right() const
		{
			return Vector3f(rotation.i, -rotation.r, 0.0f);
		}
	};



	class Track
	{
	public:
		void AddSegment(Vector3f left, Vector3f right)
		{
			leftBorder.push_back(left);
			rightBorder.push_back(right);
		}

		void Draw()
		{
			glLineWidth(3.0f);

			glBegin(GL_QUADS);
			for (uint i = 0; i < leftBorder.size() - 1; ++i)
			{
				if ((i % 8) < 4)
					glColor4ub(192, 192, 192, 255);
				else
					glColor4ub(128, 128, 128, 255);
				glVertex(leftBorder[i]);
				glVertex(rightBorder[i]);
				glVertex(rightBorder[i+1]);
				glVertex(leftBorder[i+1]);
			}
			glEnd();

			glColor4ub(0, 0, 0, 255);
			glBegin(GL_LINE_STRIP);
			for (uint i = 0; i < leftBorder.size(); ++i)
				glVertex(leftBorder[i]);
			glEnd();
			glBegin(GL_LINE_STRIP);
			for (uint i = 0; i < leftBorder.size(); ++i)
				glVertex(rightBorder[i]);
			glEnd();
		}

	private:
		deque<Vector3f> leftBorder;
		deque<Vector3f> rightBorder;
	};



	class Segment
	{
	public:
		virtual Transform GenTrack(Track* track, const Transform& entry, float step) = 0;

	private:
	};



	class Straigh : public Segment
	{
	public:
		Straigh(float _length)
			: length(_length) {}

		virtual Transform GenTrack(Track* track, const Transform& entry, float step)
		{
			const Vector3f front = entry.Front();
			const Vector3f right = entry.Right();

			for (float l = 0.0f; l < length; l += step)
			{
				Vector3f p;
				p.x = entry.position.x + front.x * l;
				p.y = entry.position.y + front.y * l;
				p.z = 0.0f;
				track->AddSegment(p - right * 7.0f, p + right * 7.0f);
			}

			Transform out = entry;
			out.position = entry.position + front * length;
			return out;
		}

	private:
		float length;
	};



	class Turn : public Segment
	{
	public:
		Turn(float _radius, float _angle)
			: radius(_radius)
			, angle(_angle) {}

		virtual Transform GenTrack(Track* track, const Transform& entry, float step)
		{
			const Vector3f front = entry.Front();
			const Vector3f right = entry.Right();
			const Vector3f center = entry.position + right * radius;
			const float length = angle * PI * abs(radius);
			const float invLength = 1.0f / length;

			for (float l = 0.0f; l <= length; l += step)
			{
				const float th = l * invLength * angle;
				const float c = cos(th);
				const float s = sin(th);

				const float leftOffset = 7.0f;
				const float rightOffset = 7.0f;

				Vector3f p0;
				p0.x = s * abs(radius);
				p0.y = c * radius - radius;
				p0.z = 0.0f;
				p0.x += (s * rightOffset) * Sign(radius);
				p0.y += c * rightOffset;

				Vector3f p1;
				p1.x = s * abs(radius);
				p1.y = c * radius - radius;
				p1.z = 0.0f;
				p1.x -= (s * rightOffset) * Sign(radius);
				p1.y -= c * rightOffset;

				track->AddSegment(entry.Trans(p0), entry.Trans(p1));
			}

			Transform out = entry;
			out.position = Pivot(entry, angle, 0.0f);
			if (radius < 0.0f)
				out.rotation = Complexf(cos(angle), sin(angle)) * entry.rotation;
			else
				out.rotation = Complexf(cos(-angle), sin(-angle)) * entry.rotation;
			return out;
		}

	private:
		Vector3f Pivot(const Transform& entry, float angle, float offset) const
		{
			const float c = cos(angle);
			const float s = sin(angle);
			Vector3f p;
			p.x = s * abs(radius) + s * offset;
			p.y = c * radius - radius + c * offset;
			p.z = 0.0f;
			return entry.Trans(p);
		}

	private:
		float radius;
		float angle;
	};



	const Vector2f carSize = Vector2f(3.0f, 1.8f);
	const Vector2f tireSize = Vector2f(0.55f, 0.24f);
	const Vector2f tireOffset = Vector2f(1.2f, carSize.y * 0.5f - tireSize.y * 0.5f);
	const float carMass = 800.0f;
	const float maxSpeed = 280.0f / 3.6f;
	const float maxAngle = 0.35f;


	b2Vec2 ToB2(Vector2f v)
	{
		return b2Vec2(v.x, v.y);
	}



	class Tire
	{
	public:
		b2Body* m_body;
		float m_maxForwardSpeed;
		float m_maxBackwardSpeed;
		float m_maxDriveForce;
		float m_maxLateralImpulse;
		float m_currentTraction;

		Tire(b2World* world)
		{
			b2BodyDef bodyDef;
			bodyDef.type = b2_dynamicBody;
			m_body = world->CreateBody(&bodyDef);

			b2PolygonShape polygonShape;
			polygonShape.SetAsBox(tireSize.x * 0.5f, tireSize.y * 0.5f);
			b2Fixture* fixture = m_body->CreateFixture(&polygonShape, 1);

			m_body->SetUserData(this);

			m_currentTraction = 1;
		}

		~Tire()
		{
			m_body->GetWorld()->DestroyBody(m_body);
		}

		void SetCharacteristics(float maxForwardSpeed, float maxBackwardSpeed, float maxDriveForce, float maxLateralImpulse)
		{
			m_maxForwardSpeed = maxForwardSpeed;
			m_maxBackwardSpeed = maxBackwardSpeed;
			m_maxDriveForce = maxDriveForce;
			m_maxLateralImpulse = maxLateralImpulse;
		}

		b2Vec2 GetLateralVelocity()
		{
			b2Vec2 currentRightNormal = m_body->GetWorldVector(b2Vec2(0, 1));
			return b2Dot(currentRightNormal, m_body->GetLinearVelocity()) * currentRightNormal;
		}

		b2Vec2 GetForwardVelocity()
		{
			b2Vec2 currentForwardNormal = m_body->GetWorldVector(b2Vec2(1, 0));
			return b2Dot(currentForwardNormal, m_body->GetLinearVelocity()) * currentForwardNormal;
		}

		void UpdateFriction()
		{
			//lateral linear velocity
			b2Vec2 impulse = 20.0f * -GetLateralVelocity();
			m_body->ApplyLinearImpulse(m_currentTraction * impulse, m_body->GetWorldCenter(), true);

			//angular velocity
			m_body->ApplyAngularImpulse(m_currentTraction * 0.1f * m_body->GetInertia() * -m_body->GetAngularVelocity(), true);
		}

		void UpdateDrive(float desiredSpeed, float driveForce)
		{
			//find current speed in forward direction
			b2Vec2 currentForwardNormal = m_body->GetWorldVector(b2Vec2(1, 0));
			float currentSpeed = b2Dot(GetForwardVelocity(), currentForwardNormal);

			//apply necessary force
			float force = 0;
			if (desiredSpeed > currentSpeed)
				force = driveForce;
			else if (desiredSpeed < currentSpeed)
				force = -driveForce;
			else
				return;
			m_body->ApplyForce(m_currentTraction * force * currentForwardNormal, m_body->GetWorldCenter(), true);
		}

		void UpdateTurn(float desiredAngle, float torque)
		{
			//find current speed in forward direction
			float current = m_body->GetAngle();

			//apply necessary force
			m_body->ApplyTorque((desiredAngle - current) * torque, true);
		}
	};



	class Car
	{
	public:
		Car(b2World* physics)
		{
			b2BodyDef bd;
			bd.type = b2_dynamicBody;
			bd.gravityScale = 0.0f;

			bd.position.Set(0.0f, 0.0f);
			object = physics->CreateBody(&bd);

			b2PolygonShape box;
			b2FixtureDef fixtureDef;
			fixtureDef.shape = &box;
			fixtureDef.density = carMass / (carSize.x * carSize.y);
			fixtureDef.friction = 0.0f;
			fixtureDef.restitution = 0.0f;

			box.SetAsBox(carSize.x * 0.5f, (carSize.y - tireSize.y * 2.0f) * 0.5f);
			b2Fixture* fixture = object->CreateFixture(&fixtureDef);
			
			const float maxForwardSpeed = 250.0f;
			const float maxBackwardSpeed = -40.0f;
			const float backTireMaxDriveForce = 3000.0f;
			const float frontTireMaxDriveForce = 500.0f;
			const float backTireMaxLateralImpulse = 8.5f;
			const float frontTireMaxLateralImpulse = 7.5f;

			b2RevoluteJointDef jointDef;
			jointDef.bodyA = object;
			jointDef.enableLimit = true;
			jointDef.localAnchorB.SetZero();
			
			tires.reserve(4);

			// back left tire
			jointDef.lowerAngle = 0;
			jointDef.upperAngle = 0;
			{
				Tire& tire = tires.emplace_back(physics);
				tire.SetCharacteristics(
					maxForwardSpeed, maxBackwardSpeed,
					backTireMaxDriveForce, backTireMaxLateralImpulse);
				jointDef.bodyB = tire.m_body;
				jointDef.localAnchorA = ToB2(tireOffset * Vector2f(-1, 1));
				physics->CreateJoint(&jointDef);
			}
			// back right tire
			{
				Tire& tire = tires.emplace_back(physics);
				tire.SetCharacteristics(
					maxForwardSpeed, maxBackwardSpeed,
					backTireMaxDriveForce, backTireMaxLateralImpulse);
				jointDef.bodyB = tire.m_body;
				jointDef.localAnchorA = ToB2(tireOffset * Vector2f(-1, -1));
				physics->CreateJoint(&jointDef);
			}

			// front left tire
			jointDef.lowerAngle = -maxAngle;
			jointDef.upperAngle = maxAngle;
			{
				Tire& tire = tires.emplace_back(physics);
				tire.SetCharacteristics(
					maxForwardSpeed, maxBackwardSpeed,
					frontTireMaxDriveForce, frontTireMaxLateralImpulse);
				jointDef.bodyB = tire.m_body;
				jointDef.localAnchorA = ToB2(tireOffset * Vector2f(1, 1));
				physics->CreateJoint(&jointDef);
			}
			// front right tire
			{
				Tire& tire = tires.emplace_back(physics);
				tire.SetCharacteristics(
					maxForwardSpeed, maxBackwardSpeed,
					frontTireMaxDriveForce, frontTireMaxLateralImpulse);
				jointDef.bodyB = tire.m_body;
				jointDef.localAnchorA = ToB2(tireOffset * Vector2f(1, -1));
				physics->CreateJoint(&jointDef);
			}
		}

		void Step(float deltaTime)
		{
			const float curSpeed = object->GetLinearVelocity().Length();

			for (uint i = 0; i < tires.size(); ++i)
				tires[i].UpdateFriction();

			bool coast = true;
			if (GetAsyncKeyState('G') & 0x8000)
			{
				tires[0].UpdateDrive(maxSpeed, 3000.0f);
				tires[1].UpdateDrive(maxSpeed, 3000.0f);
				coast = false;
			}
			if (GetAsyncKeyState('F') & 0x8000)
			{
				tires[2].UpdateDrive(0.0f, 10000.0f);
				tires[3].UpdateDrive(0.0f, 10000.0f);
				coast = false;
			}
			if (coast)
			{
				tires[0].UpdateDrive(0.0f, 1500.0f);
				tires[1].UpdateDrive(0.0f, 1500.0f);
				tires[2].UpdateDrive(0.0f, 1500.0f);
				tires[3].UpdateDrive(0.0f, 1500.0f);
			}

			float targetAngle = 0.0f;
			float targetTorque = 1.0f;
			if (GetAsyncKeyState('A') & 0x8000)
			{
				targetAngle = maxAngle;
				targetTorque = 0.35f;
			}
			else if (GetAsyncKeyState('S') & 0x8000)
			{
				targetAngle = -maxAngle;
				targetTorque = 0.35f;
			}
			targetAngle += object->GetAngle();
			const float torque = Saturate(1 - curSpeed / maxSpeed) * targetTorque;
			tires[2].UpdateTurn(targetAngle, torque);
			tires[3].UpdateTurn(targetAngle, torque);
		}

		void Draw() const
		{
		}

		b2Body* object = nullptr;
		vector<Tire> tires;
	};



	class DebugDraw : public b2Draw
	{
	public:
		virtual void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
		{
			glLineWidth(2.0f);
			glBegin(GL_LINE_LOOP);
			glColor4f(color.r, color.g, color.b, color.a);
			for (uint i = 0; i < vertexCount; ++i)
				glVertex2f(vertices[i].x, vertices[i].y);
			glEnd();
		}
		virtual void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override
		{
			glBegin(GL_TRIANGLES);
			glColor4f(color.r, color.g, color.b, color.a * 0.25f);
			for (uint i = 1; i < vertexCount - 1; ++i)
			{
				glVertex2f(vertices[0].x, vertices[0].y);
				glVertex2f(vertices[i].x, vertices[i].y);
				glVertex2f(vertices[i+1].x, vertices[i+1].y);
			}
			glEnd();
			DrawPolygon(vertices, vertexCount, color);
		}
		virtual void DrawCircle(const b2Vec2& center, float32 radius, const b2Color& color) override
		{
			__debugbreak();
		}
		virtual void DrawSolidCircle(const b2Vec2& center, float32 radius, const b2Vec2& axis, const b2Color& color) override
		{
			__debugbreak();
		}
		virtual void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override
		{
			glLineWidth(2.0f);
			glBegin(GL_LINES);
			glColor4f(color.r, color.g, color.b, color.a);
			glVertex2f(p1.x, p1.y);
			glVertex2f(p2.x, p2.y);
			glEnd();
		}
		virtual void DrawTransform(const b2Transform& xf) override
		{
			__debugbreak();
		}
		virtual void DrawPoint(const b2Vec2& p, float32 size, const b2Color& color) override
		{
			__debugbreak();
		}
	};



	void RacingTrack()
	{
		deque<SegmentPtr> segments;
		segments.emplace_back(make_shared<Straigh>(450.0f));
		segments.emplace_back(make_shared<Turn>(20.0f, Degrees2Radians(45.0f)));
		segments.emplace_back(make_shared<Turn>(40.0f, Degrees2Radians(45.0f)));
		segments.emplace_back(make_shared<Turn>(20.0f, Degrees2Radians(45.0f)));
		segments.emplace_back(make_shared<Turn>(-40.0f, Degrees2Radians(45.0f)));
		segments.emplace_back(make_shared<Turn>(-180.0f, Degrees2Radians(25.0f)));
		segments.emplace_back(make_shared<Straigh>(15.0f));
		segments.emplace_back(make_shared<Turn>(-9.0f, Degrees2Radians(65.0f)));
		segments.emplace_back(make_shared<Straigh>(20.0f));
		segments.emplace_back(make_shared<Turn>(60.0f, Degrees2Radians(180.0f-45.0f)));
		segments.emplace_back(make_shared<Turn>(40.0f, Degrees2Radians(45.0f)));
		segments.emplace_back(make_shared<Turn>(220.0f, Degrees2Radians(45.0f)));
		
		const float step = 1.0f;
		Track track;
		Transform transform;
		transform.position = Vector3f(zero);
		transform.rotation = Complexf(1.0f, 0.0f);
		for (const auto& segment : segments)
			transform = segment->GenTrack(&track, transform, step);

		///////////////////////////////////////////////

		DebugDraw physicsDebug;
		b2World physics{ b2Vec2(0.0f, 0.0f) };
		physicsDebug.SetFlags(b2Draw::e_shapeBit);
		physics.SetDebugDraw(&physicsDebug);

		Car player{ &physics };
			
		///////////////////////////////////////////////

		ProtoGL protoGL;
		protoGL.Initialize(ProtoGLDesc(1280, 720));
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_CULL_FACE);
		glEnable(GL_LINE_SMOOTH);
		glEnable(GL_BLEND);

	//	CameraOrtho2D camera2D(protoGL.GetCanvas(), CameraOrtho2D::Upward);
	//	camera2D.SetCamera(Vector2f(zero), 20.0f);

		const Vector3f camTarget{ 0.742179930f, 0.139933944f, 3.67527342f };
		const Quaternionf camQuat{ -0.000500082970f, 0.0499544740f, 0.0100392150f, 0.998699248f };
		OrbitalCamera camera{ protoGL.GetCanvas() };
		camera.SetPerspective(0.75f, 0.1f, 1000.0f);
		camera.SetCamera(camTarget, camQuat, 25.0f);

		Time steptime = Time(1.0f / 60.0f);
		TimeCounter timer;
		Time accum;

		while (protoGL.Update())
		{
			Time deltaTime = timer.GetElapsed();
			accum += deltaTime;

			while (accum > steptime)
			{
				physics.Step(steptime.AsFloat(), 4, 4);
				player.Step(deltaTime.AsFloat());
				accum -= steptime;
			}

			//

			const b2Vec2 carPos = player.object->GetPosition();
			const float carAngle = player.object->GetAngle();
			const Quaternionf carQuat = QuaternionAxisAngle(Vector3f(0, 0, 1), carAngle);
		//	camera2D.SetCamera(Vector2f(carPos.x, carPos.y), 180.0f, -carAngle + PI*0.5f);

			camera.SetCamera(
				camTarget + Vector3f(carPos.x, carPos.y, 0.0f),
				carQuat * camQuat,
				25.0f);

		//	camera.Update(deltaTime);

			glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			camera.CommitToGL();

			glLineWidth(1.0f);
			glColor4ub(128, 128, 128, 255);
			glBegin(GL_LINES);
			glVertex2f(-10.0f,   0.0f);
			glVertex2f( 10.0f,   0.0f);
			glVertex2f(  0.0f, -10.0f);
			glVertex2f(  0.0f,  10.0f);
			glEnd();

			track.Draw();
			player.Draw();
			physics.DrawDebugData();

			protoGL.Swap();
		}
	}


}


namespace orbit
{


typedef double Scalar;

const Scalar π     = acos(-1.0);
const Scalar two_π = π * 2;
const Scalar G     = 6.674e-11;
const Scalar inf   = numeric_limits<Scalar>::infinity();



Scalar Deg2Rad(Scalar x)
{
	return x * π / 180.0;
}

Scalar Sqr(Scalar x)
{
	return x * x;
}



struct CelestialBody;



struct GeoCoord
{
	Scalar latitude;
	Scalar longitude;
	Scalar altitude;
};



struct Vector
{
	Scalar x;
	Scalar y;
	Scalar z;
};



struct Quaternion
{
	Scalar x;
	Scalar y;
	Scalar z;
	Scalar w;
};



Vector MakeVector(float x, float y, float z)
{
	Vector v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}



Vector VectorAdd(Vector lhv, Vector rhv)
{
	return MakeVector(lhv.x + rhv.x, lhv.y + rhv.y, lhv.z + rhv.z);
}


Vector VectorSub(Vector lhv, Vector rhv)
{
	return MakeVector(lhv.x - rhv.x, lhv.y - rhv.y, lhv.z - rhv.z);
}


Vector VectorScale(Vector lhv, Scalar rhv)
{
	return MakeVector(lhv.x * rhv, lhv.y * rhv, lhv.z * rhv);
}


Scalar VectorLengthSquared(Vector vec)
{
	return Sqr(vec.x) + Sqr(vec.y) + Sqr(vec.z);
}


Scalar VectorLength(Vector vec)
{
	return sqrt(VectorLengthSquared(vec));
}


Vector VectorNormalize(Vector vec)
{
	return VectorScale(vec, 1 / VectorLength(vec));
}



Scalar VectorDot(Vector lhv, Vector rhv)
{
	return lhv.x * rhv.x + lhv.y * rhv.y + lhv.z * rhv.z;
}



Vector VectorCross(Vector lhv, Vector rhv)
{
	return MakeVector(
		lhv.y*rhv.z - lhv.z*rhv.y,
		lhv.z*rhv.x - lhv.x*rhv.z,
		lhv.x*rhv.y - lhv.y*rhv.x);
}



Quaternion QuaternionAngleAxis(Scalar angle, Vector axis)
{
	Scalar d = angle * 0.5;
	Scalar s = sin(d);
	Quaternion q;
	q.x = axis.x * s;
	q.y = axis.y * s;
	q.z = axis.z * s;
	q.w = cos(d);
	return q;
}



Quaternion QuaternionMul(Quaternion lhv, Quaternion rhv)
{
	Scalar e = (lhv.x + lhv.z)*(rhv.x + rhv.y);
	Scalar f = (lhv.z - lhv.x)*(rhv.x - rhv.y);
	Scalar g = (lhv.w + lhv.y)*(rhv.w - rhv.z);
	Scalar h = (lhv.w - lhv.y)*(rhv.w + rhv.z);
	Scalar a = f - e;
	Scalar b = f + e;
	Quaternion q;
	q.x = (lhv.w + lhv.x)*(rhv.w + rhv.x) + (a - g - h) * 0.5;
	q.y = (lhv.w - lhv.x)*(rhv.y + rhv.z) + (b + g - h) * 0.5;
	q.z = (lhv.y + lhv.z)*(rhv.w - rhv.x) + (b - g + h) * 0.5;
	q.w = (lhv.z - lhv.y)*(rhv.y - rhv.z) + (a + g + h) * 0.5;
	return q;
}



Quaternion Conjugate(Quaternion quat)
{
	Quaternion q;
	q.x = -quat.x;
	q.y = -quat.y;
	q.z = -quat.z;
	q.w = quat.w;
	return q;
}



Vector QuaternionRotate(Quaternion quat, Vector vec)
{
	Quaternion vq;
	vq.x = vec.x;
	vq.y = vec.y;
	vq.z = vec.z;
	vq.w = 0;
	Quaternion r = QuaternionMul(QuaternionMul(quat, vq), Conjugate(quat));
	return MakeVector(r.x, r.y, r.z);
}



bool IsNAN(Scalar x)
{
	return x != x;
}



struct Orbit
{
	const CelestialBody* referenceBody;
	Scalar semiMajorAxis;
	Scalar eccentricity;
	Scalar meanAnomalyAtEpoch;
	Scalar timeOfPeriapsisPassage;
	Scalar inclination;
	Scalar longitudeOfAscendingNode;
	Scalar argumentOfPeriapsis;
};



Orbit MakeOrbit(const CelestialBody* referenceBody, Scalar semiMajorAxis, Scalar eccentricity, Scalar inclination, Scalar longitudeOfAscendingNode, Scalar argumentOfPeriapsis, Scalar meanAnomalyAtEpoch, Scalar timeOfPeriapsisPassage)
{
	Orbit orbit;
	orbit.referenceBody = referenceBody;
	orbit.semiMajorAxis = semiMajorAxis;
	orbit.eccentricity = eccentricity;
	orbit.meanAnomalyAtEpoch = meanAnomalyAtEpoch;
	orbit.timeOfPeriapsisPassage = timeOfPeriapsisPassage;
	orbit.inclination = Deg2Rad(inclination);
	orbit.longitudeOfAscendingNode = Deg2Rad(longitudeOfAscendingNode);
	orbit.argumentOfPeriapsis = Deg2Rad(argumentOfPeriapsis);
	return orbit;
}



struct CelestialBody
{
	Scalar mass;
	Scalar radius;
	Scalar siderealRotation;
	Orbit  orbit;
	Scalar atmPressure;
	Scalar atmScaleHeight;
	Scalar gravitationalParameter;
	Scalar sphereOfInfluence;
	Scalar atmRadius;
};



CelestialBody MakeCelestialBody(Scalar mass, Scalar radius, Scalar siderealRotation, Orbit orbit, Scalar atmPressure, Scalar atmScaleHeight)
{
	CelestialBody body;
	body.mass = mass;
	body.radius = radius;
	body.siderealRotation = siderealRotation;
	body.orbit = orbit;
	body.atmPressure = atmPressure;
	body.atmScaleHeight = atmScaleHeight;
	body.gravitationalParameter = G * mass;
	if (orbit.referenceBody)
		body.sphereOfInfluence = orbit.semiMajorAxis * pow(mass / orbit.referenceBody->mass, 0.4);
	else
		body.sphereOfInfluence = inf;
	body.atmRadius = -log(1e-6) * atmScaleHeight + radius;
	return body;
}



const CelestialBody Kerbol = MakeCelestialBody(1.756567e+28, 2.616e+08, 432000, MakeOrbit(nullptr, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0), 0.0, 0.0);
const CelestialBody Moho   = MakeCelestialBody(2.5263617e21, 250000, 1210000, MakeOrbit(&Kerbol, 5263138304, 0.2, 7.0, 70.0, 15.0, 3.14, 0.0), 0.0, 0.0);
const CelestialBody Kerbin = MakeCelestialBody(5.2915793e22, 600000, 21600, MakeOrbit(&Kerbol, 13599840256, 0.0, 0, 0, 0, 3.14, 0.0), 1, 5000);
const CelestialBody Mun    = MakeCelestialBody(9.7600236e20, 200000, 138984.38, MakeOrbit(&Kerbin, 12000000, 0.0, 0, 0, 0, 1.7, 0.0), 0.0, 0.0);
const CelestialBody Minmus = MakeCelestialBody(2.6457897e19, 60000, 40400, MakeOrbit(&Kerbin, 47000000, 0.0, 6.0, 78.0, 38.0, 0.9, 0.0), 0.0, 0.0);



bool IsHyperbolic(const Orbit& orbit)
{
	return orbit.eccentricity > 1;
};



Scalar Apoapsis(const Orbit& orbit)
{
	return orbit.semiMajorAxis * (1 + orbit.eccentricity);
};



Scalar Periapsis(const Orbit& orbit)
{
	return orbit.semiMajorAxis * (1 - orbit.eccentricity);
};



Scalar ApoapsisAltitude(const Orbit& orbit)
{
	return Apoapsis(orbit) - orbit.referenceBody->radius;
};



Scalar PeriapsisAltitude(const Orbit& orbit)
{
	return Periapsis(orbit) - orbit.referenceBody->radius;
};



Scalar SemiMinorAxis(const Orbit& orbit)
{
	Scalar e = orbit.eccentricity;
	return orbit.semiMajorAxis * sqrt(1 - e * e);
};



Scalar SemiLatusRectum(const Orbit& orbit)
{
	Scalar e = orbit.eccentricity;
	return orbit.semiMajorAxis * (1 - e * e);
};



Scalar MeanMotion(const Orbit& orbit)
{
	Scalar a = abs(orbit.semiMajorAxis);
	return sqrt(orbit.referenceBody->gravitationalParameter / (a * a * a));
};



Scalar Period(const Orbit& orbit)
{
	if (IsHyperbolic(orbit))
		return inf;
	else
		return two_π / MeanMotion(orbit);
};



Quaternion RotationToReferenceFrame(const Orbit& orbit)
{
	Vector axisOfInclination = MakeVector(
		cos(-orbit.argumentOfPeriapsis),
		sin(-orbit.argumentOfPeriapsis),
		0);
	return QuaternionMul(
		QuaternionAngleAxis(orbit.longitudeOfAscendingNode + orbit.argumentOfPeriapsis, MakeVector(0, 0, 1)),
		QuaternionAngleAxis(orbit.inclination, axisOfInclination));
};



Scalar RadiusAtAnomaly(const Orbit& orbit, Scalar a)
{
	Scalar e = orbit.eccentricity;
	return orbit.semiMajorAxis * (1 - e * e) / (1 + e * cos(a));
};



Vector PositionAtAnomaly(const Orbit& orbit, Scalar a)
{
	Scalar r = RadiusAtAnomaly(orbit, a);
	return QuaternionRotate(
		RotationToReferenceFrame(orbit),
		MakeVector(r * cos(a), r * sin(a), 0));
};



Scalar MeanAnomalyAt(const Orbit& orbit, Scalar t)
{
	if (IsHyperbolic(orbit))
	{
		return (t - orbit.timeOfPeriapsisPassage) * MeanMotion(orbit);
	}
	else
	{
	//	if (this.timeOfPeriapsisPassage != null)
		{
			Scalar M = fmod((t - orbit.timeOfPeriapsisPassage), Period(orbit)) * MeanMotion(orbit);
			if (M < 0)
				return M + two_π;
			else
				return M;
		}
	//	else
	//		return (this.meanAnomalyAtEpoch + this.meanMotion() * (t % this.period())) % TWO_PI;
	}
};



Scalar EccentricAnomalyAtTrueAnomaly(const Orbit& orbit, Scalar tA)
{
	Scalar e = orbit.eccentricity;
	if (IsHyperbolic(orbit))
	{
		Scalar cosTrueAnomaly = cos(tA);
		Scalar H = acosh((e + cosTrueAnomaly) / (1 + e * cosTrueAnomaly));
		if (tA < 0)
			return -H;
		else
			return H;
	}
	else {
		Scalar E = 2 * atan(tan(tA / 2) / sqrt((1 + e) / (1 - e)));
		if (E < 0)
			return E + TWO_PI;
		else
			return E;
	}
};



Scalar MeanAnomalyAtTrueAnomaly(const Orbit& orbit, Scalar tA)
{
	Scalar e = orbit.eccentricity;
	if (IsHyperbolic(orbit))
	{
		Scalar H = EccentricAnomalyAtTrueAnomaly(orbit, tA);
		return e * sinh(H) - H;
	}
	else {
		Scalar E = EccentricAnomalyAtTrueAnomaly(orbit, tA);
		return E - e * sin(E);
	}
};



Scalar EccentricAnomalyAt(const Orbit& orbit, Scalar t)
{
	Scalar e = orbit.eccentricity;
	Scalar M = MeanAnomalyAt(orbit, t);
	if (IsHyperbolic(orbit))
	{
		Scalar x0 = M;
		for (;;)
		{
			Scalar f = M - e * sinh(x0) + x0;
			Scalar df = 1 - e * cosh(x0);
			Scalar x = x0 - f / df;
			if (IsNAN(x) || abs(x - x0) < 1e-6)
				return x;
			x0 = x;
		}
	}
	else 
	{
		Scalar x0 = M;
		for (;;)
		{
			Scalar f = M + e * sin(x0) - x0;
			Scalar df = e * cos(x0) - 1;
			Scalar x = x0 - f / df;
			if (IsNAN(x) || abs(x - x0) < 1e-6)
				return x;
			x0 = x;
		}
	}
};



Scalar TrueAnomalyAt(const Orbit& orbit, Scalar t)
{
	Scalar e = orbit.eccentricity;
	if (IsHyperbolic(orbit))
	{
		Scalar H = EccentricAnomalyAt(orbit, t);
		Scalar tA = acos((e - cosh(H)) / (cosh(H) * e - 1));
		if (H < 0)
			return -tA;
		else
			return tA;
	}
	else
	{
		Scalar E = EccentricAnomalyAt(orbit, t);
		Scalar tA = 2 * atan2(sqrt(1 + e) * sin(E / 2), sqrt(1 - e) * cos(E / 2));
		if (tA < 0)
			return tA + two_π;
		else
			return tA;
	}
}



Orbit OrbitFromPositionAndVelocity(const CelestialBody* referenceBody, Vector position, Vector velocity, Scalar t)
{
	Scalar mu = referenceBody->gravitationalParameter;
	Scalar r = VectorLength(position);
	Scalar v = VectorLength(velocity);
	Vector specificAngularMomentum = VectorCross(position, velocity);
	Vector nodeVector;
	if (specificAngularMomentum.x != 0 || specificAngularMomentum.y != 0)
		nodeVector = VectorNormalize(MakeVector(-specificAngularMomentum.y, specificAngularMomentum.x, 0));
	else
		nodeVector = MakeVector(1, 0, 0);
	Vector eccentricityVector = VectorScale(VectorSub(VectorScale(position, v * v - mu / r), VectorScale(velocity, VectorDot(position, velocity))), 1 / mu);
	Scalar semiMajorAxis = 1 / (2 / r - v * v / mu);
	Scalar eccentricity = VectorLength(eccentricityVector);
	Orbit orbit = MakeOrbit(referenceBody, semiMajorAxis, eccentricity, 0, 0, 0, 0, 0);
	orbit.inclination = acos(specificAngularMomentum.z / VectorLength(specificAngularMomentum));
	if (eccentricity != 0)
	{
		orbit.longitudeOfAscendingNode = acos(nodeVector.x);
		if (nodeVector.y < 0)
			orbit.longitudeOfAscendingNode = two_π - orbit.longitudeOfAscendingNode;
		orbit.argumentOfPeriapsis = acos(VectorDot(nodeVector, eccentricityVector) / eccentricity);
		if (eccentricityVector.z < 0)
			orbit.argumentOfPeriapsis = two_π - orbit.argumentOfPeriapsis;
	}
	Scalar trueAnomaly = acos(VectorDot(eccentricityVector, position) / (eccentricity * r));
	if (VectorDot(position, velocity) < 0)
		trueAnomaly = -trueAnomaly;
	Scalar meanAnomaly = MeanAnomalyAtTrueAnomaly(orbit, trueAnomaly);
	orbit.timeOfPeriapsisPassage = t - meanAnomaly / MeanMotion(orbit);
	return orbit;
};



struct CelestialBodyState
{
	CelestialBodyState* parentBody;
};



void glVertexCR(Vector camPosition, Scalar camScale, Vector v)
{
	glVertex3f(
		float((v.x - camPosition.x) * camScale),
		float((v.y - camPosition.y) * camScale),
		float((v.z - camPosition.z) * camScale));
}



void DrawCelestialBody(Vector camPosition, Scalar camScale, Vector position, Scalar radius)
{
	glBegin(GL_POINTS);
	glVertexCR(camPosition, camScale, position);
	glEnd();

	for (Scalar l = -Deg2Rad(80.0); l <= Deg2Rad(85.0); l += Deg2Rad(20.0))
	{
		Vector p;
		Scalar cl = cos(l);
		p.z = position.z + sin(l) * radius;
		glBegin(GL_LINE_LOOP);
		for (uint i = 0; i < 32; ++i)
		{
			Scalar a = (i / 32.0) * two_π;
			p.x = position.x + cos(a) * radius * cl;
			p.y = position.y + sin(a) * radius * cl;
			glVertexCR(camPosition, camScale, p);
		}
		glEnd();
	}
}



void DrawOrbit(Vector camPosition, Scalar camScale, Vector parentPos, const Orbit& orbit)
{
	glBegin(GL_LINE_LOOP);
	for (uint i = 0; i < 128; ++i)
	{
		const Scalar a = (i / 128.0) * two_π;
		Vector p = VectorAdd(parentPos, PositionAtAnomaly(orbit, a));
		glVertexCR(camPosition, camScale, p);
	}
	glEnd();
}



void OrbitTest()
{
	// Orbit
#if 0
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	const float camDistance = 2.5f;
	OrbitalCamera camera{ protoGL.GetCanvas() };
	camera.SetPerspective(0.75f, 0.1f, 1024.0f*1024.0);
	camera.SetCamera(Vector3f(zero), Quaternionf(zero), camDistance);

	Vector camPosition = MakeVector(0, 0, 0);
	Scalar camScale = 1 / (Kerbin.radius * 4.0);

	Scalar UT = 0;

	TimeCounter timer;

	while (protoGL.Update())
	{
		Time delta = timer.GetElapsed();
	//	UT += delta.AsFloat() * 100000.0;

		Vector kerbolPos = MakeVector(0, 0, 0);
		Vector mohoPos = PositionAtAnomaly(Moho.orbit, TrueAnomalyAt(Moho.orbit, UT));
		Vector kerbinPos = PositionAtAnomaly(Kerbin.orbit, TrueAnomalyAt(Kerbin.orbit, UT));
		Vector munPos = VectorAdd(kerbinPos, PositionAtAnomaly(Mun.orbit, TrueAnomalyAt(Mun.orbit, UT)));
		Vector minmusPos = VectorAdd(kerbinPos, PositionAtAnomaly(Minmus.orbit, TrueAnomalyAt(Minmus.orbit, UT)));

		camPosition = kerbinPos;
		camScale *= camDistance / camera.GetDistance();
		camera.SetDistance(camDistance);
		camera.SetTarget(Vector3f(zero));

		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera.CommitToGL();

		glPointSize(8);
		glLineWidth(2.0f);

		glColor4ub(255, 128, 0, 255);
		DrawCelestialBody(camPosition, camScale, MakeVector(0, 0, 0), Kerbol.radius);

		glColor4ub(98, 64, 64, 255);
		DrawCelestialBody(camPosition, camScale, mohoPos, Moho.radius);
		DrawOrbit(camPosition, camScale, kerbolPos, Moho.orbit);

		glColor4ub(0, 128, 255, 255);
		DrawCelestialBody(camPosition, camScale, kerbinPos, Kerbin.radius);
		DrawOrbit(camPosition, camScale, kerbolPos, Kerbin.orbit);

		glColor4ub(64, 64, 64, 255);
		DrawCelestialBody(camPosition, camScale, munPos, Mun.radius);
		DrawOrbit(camPosition, camScale, kerbinPos, Mun.orbit);

		glColor4ub(0, 255, 128, 255);
		DrawCelestialBody(camPosition, camScale, minmusPos, Minmus.radius);
		DrawOrbit(camPosition, camScale, kerbinPos, Minmus.orbit);

		protoGL.Swap();
	}
#endif

	// Launch simulation
#if 1
	const Scalar dT = 1 / 10.0;
	const CelestialBody& refBody = Kerbin;

	struct Point
	{
		Vector position;
		float hv;
		float vv;
	};
	deque<Point> points;
	Orbit finalOrbit;
	{
		Vector position = MakeVector(0, refBody.radius, 0);
		Vector velocity = MakeVector(two_π / refBody.siderealRotation * refBody.radius, 0, 0);
		refBody.gravitationalParameter;

		Scalar burnTime = 550;
		for (Scalar t = 0; t < burnTime; t += dT)
		{
			// plot
			Point point;
			point.position = position;
			points.push_back(point);

			// vars
			Vector acc = MakeVector(0, 0, 0);
			Vector radial = VectorNormalize(VectorSub(position, MakeVector(0, 0, 0)));
			Vector tangent = VectorCross(radial, MakeVector(0, 0, 1));
			Scalar alt = VectorLength(position) - refBody.radius;
			Vector surfVelocity = VectorSub(velocity, VectorScale(tangent, two_π / refBody.siderealRotation * (refBody.radius + alt)));

			// Maneuvering
		//	Scalar alt = VectorLength(position) - refBody.radius;
		//	Scalar altF = max(min((70000 - alt) / (70000 - 1500), 1.0), 0.0);
		//	Scalar pitch = pow(altF, 4) * π * 0.5;
		//	Scalar thrust = 9.8 * 3 * altF + 9.8 * (1 - altF);
		//	acc = VectorScale(VectorAdd(VectorScale(tangent, cos(pitch)), VectorScale(radial, sin(pitch))), thrust);
						
			Scalar altF = max(min((80000 - alt) / (80000 - 1500), 1.0), 0.0);
			Scalar targetPitch = pow(altF, 2) * π * 0.5;
			Scalar curPitch = atan2(VectorDot(radial, surfVelocity), VectorDot(tangent, surfVelocity));
			if (curPitch < 1.0 / 128.0)
				curPitch = targetPitch;
			Scalar pitchError = targetPitch - curPitch;
			Scalar pitch = curPitch + pitchError * 0.5;
			// Scalar thrust = 9.8 * 3 * altF + 9.8 * (1 - altF);
			Scalar thrust = 9.8 * (2.5 + max(pitchError * 5.5, 1.0));
			acc = VectorScale(VectorAdd(VectorScale(tangent, cos(pitch)), VectorScale(radial, sin(pitch))), thrust);

			// Physics
			Scalar g = refBody.gravitationalParameter / VectorLengthSquared(position);
			acc = VectorAdd(acc, VectorScale(radial, -g));
		//	velocity = VectorAdd(velocity, VectorScale(acc, dT));
			surfVelocity = VectorScale(tangent, two_π / refBody.siderealRotation * (refBody.radius + alt));
		//	velocity = VectorAdd(surfVelocity, VectorScale(VectorAdd(VectorScale(tangent, cos(targetPitch)), VectorScale(radial, sin(targetPitch))), 10 + (alt / 70000) * 2300));
			velocity = VectorAdd(surfVelocity, VectorScale(VectorAdd(VectorScale(tangent, cos(targetPitch)), VectorScale(radial, sin(targetPitch))), t / burnTime * 2300));
			position = VectorAdd(position, VectorScale(velocity, dT));
		}

		finalOrbit = OrbitFromPositionAndVelocity(&refBody, position, velocity, burnTime);
		finalOrbit.argumentOfPeriapsis -= burnTime * (two_π / refBody.siderealRotation);
	}

	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera(protoGL.GetCanvas(), CameraOrtho2D::Upward);
	camera.SetCamera(Vector2f(0.0, refBody.radius), 250000.0f);

	while (protoGL.Update())
	{
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera.CommitToGL();

		glPointSize(8);
		glLineWidth(2.0f);

		glColor4ub(0, 128, 255, 255);
		glBegin(GL_LINE_LOOP);
		for (uint i = 0; i < 1024; ++i)
		{
			Scalar a = (i / 1024.0) * two_π;
			Scalar x = cos(a) * refBody.radius;
			Scalar y = sin(a) * refBody.radius;
			glVertex2f(x, y);
		}
		glEnd();

		glLineWidth(1.0f);
		glColor4ub(128, 192, 255, 255);
		glBegin(GL_LINE_LOOP);
		for (uint i = 0; i < 1024; ++i)
		{
			Scalar a = (i / 1024.0) * two_π;
			Scalar x = cos(a) * refBody.atmRadius;
			Scalar y = sin(a) * refBody.atmRadius;
			glVertex2f(x, y);
		}
		glEnd();

		glLineWidth(2.0f);
		glColor4ub(255, 128, 32, 255);
		DrawOrbit(MakeVector(0, 0, 0), 1, MakeVector(0, 0, 0), finalOrbit);
		
		glColor4ub(64, 64, 64, 255);
		glBegin(GL_LINE_STRIP);
		for (uint i = 0; i < points.size(); ++i)
		{
			Scalar t = i * dT;
			Scalar a = t * (two_π / refBody.siderealRotation);
			Scalar s = sin(a);
			Scalar c = cos(a);
			Vector p = points[i].position;
			Vector rp;
			rp.x = c * p.x - s * p.y;
			rp.y = s * p.x + c * p.y;
			glVertex2f(rp.x, rp.y);
		}
		glEnd();

		protoGL.Swap();
	}
#endif
}


}



void asvgf()
{
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera(protoGL.GetCanvas(), CameraOrtho2D::Upward);
	camera.SetCamera(Vector2f(0.0, 0.0f), 16.0f);

	const uint numSamples = 1024;
	vidf::Rand48 rand48;
	vidf::UniformReal<float> dist{ 0.0f, 1.0f };

	deque<float> samples;
	deque<float> filterV0;
	deque<float> filterV1;
	deque<float> vars;
	{
		for (uint i = 0; i < numSamples; ++i)
		{
		//	const float sample = pow(dist(rand48), 3.0f) * 2.5f;
			const float sample = pow(dist(rand48), 0.33f) * 0.0f + 0.25f;
		//	const float sample = dist(rand48) * 0.75f + 2.0f;
			samples.push_back(sample);
		}
	}
	{
		float histSamplePrev = 0.0f;
		for (uint i = 0; i < numSamples; ++i)
		{
			const float sample = samples[i];

			float alpha = 1.0f / 8.0f;
			if (i % 256 == 0)
				alpha = 1.0f;
			const float histSample = Lerp(histSamplePrev, sample, alpha);

			histSamplePrev = histSample;

			filterV0.push_back(histSample);
		}
	}
	{
		float histSamplePrev = 0.0f;
		float histLenPrev = 0.0;
		Vector2f momPrev = Vector2f(zero);
		for (uint i = 0; i < numSamples; ++i)
		{
			const float sample = samples[i];

			float histLen = Min(histLenPrev + 1.0f, 256.0f);
			float alpha = Max(1.0f / 256.0f, 1.0f / histLen);
			if (i % 256 == 0)
			{
				alpha = 1.0f;
				histLen = 1.0f;
			}
			const float histSample = Lerp(histSamplePrev, sample, alpha);

			Vector2f momCurr = Vector2f(sample, sample * sample);
			// Vector2f momCurr = Vector2f(histSample, histSample * histSample);
			Vector2f mom = Lerp(momPrev, momCurr, alpha);
			if (i % 256 == 0)
			{
			//	mom = Vector2f(3.5f, 3.5f * 3.5f);
			//	mom = Vector2f(0.0f, 0.0f);
			}
			float var = sqrt(Max(mom.y - mom.x * mom.x, 0.0f));

			histSamplePrev = histSample;
			histLenPrev = histLen;
			momPrev = mom;

			filterV1.push_back(histSample);
			vars.push_back(var);
		}
	}

	while (protoGL.Update())
	{
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera.CommitToGL();

		const float mult = 1.0f / 16.0f;
		glLineWidth(2.0f);
		glColor4ub(0, 0, 0, 255);
		glBegin(GL_LINE_STRIP);
		glVertex2f(0.0f, 5.0f);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(float(samples.size()) * mult, 0.0f);
		glEnd();

		glColor4ub(255, 128, 92, 255);
		glBegin(GL_LINE_STRIP);
		for (uint i = 0; i < samples.size(); ++i)
			glVertex2f(float(i) * mult, samples[i]);
		glEnd();

	//	glColor4ub(64, 64, 255, 255);
	//	glBegin(GL_LINE_STRIP);
	//	for (uint i = 0; i < filterV0.size(); ++i)
	//		glVertex2f(float(i) * mult, filterV0[i]);
	//	glEnd();

		glColor4ub(32, 192, 64, 255);
		glBegin(GL_LINE_STRIP);
		for (uint i = 0; i < filterV1.size(); ++i)
			glVertex2f(float(i) * mult, filterV1[i]);
		glEnd();

		glColor4ub(64, 64, 255, 255);
		glBegin(GL_LINE_STRIP);
		for (uint i = 0; i < vars.size(); ++i)
			glVertex2f(float(i) * mult, vars[i]);
		glEnd();

		protoGL.Swap();
	}
}



#include <string_view>
#include <regex>
#include <filesystem>



constexpr bool ispow2(unsigned x)
{
	return x != 0 && (x & (x - 1)) == 0;
}


namespace fs = std::experimental::filesystem;


void Stuff()
{
	/*
	std::deque<int> dq;

	wregex fileRegex{ L".*\.vi.asset" };
	for (auto& p : fs::recursive_directory_iterator(L"assets"))
	{		
		if (!regex_match(p.path().filename().wstring(), fileRegex))
			continue;
		std::cout << p << '\n';
	}
	*/
	// Lens();
	// FlameGPU();
	// PID();
	// NavBall();
	// AnalogWave();
	// LowDiscrepancy();
	// VintageConsole();
	// Platformer();
	// ClothoidTest();
	// RungeKutta();
	// SincTst();
	// BSpline();
	Logic();
	// track::RacingTrack();
	// orbit::OrbitTest();
	// asvgf();
}
