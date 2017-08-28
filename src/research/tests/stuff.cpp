#include "pch.h"
#include "vidf/common/intersect.h"


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



	void DrawCircle(Vector2f center, float radius)
	{
		const uint segments = 64;
		glBegin(GL_LINE_LOOP);
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



	bool RaySphereIntersect(const Rayf& ray, const Spheref& sphere, const float n, Intersect* intersect)
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
			intersect->n = (d0 < 0.0f && sphere.radius > 0.0f) ? 1.0f : n;
		}
		return true;
	}



	bool RaySphereIntersect(const Rayf& ray, const Spheref& sphere, Vector3f axis, float theta, const float n, Intersect* intersect)
	{
		Intersect _intersect;
		const bool _result = RaySphereIntersect(ray, sphere, n, &_intersect);
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
		result |= RaySphereIntersect(ray, s1, Vector3f(-1.0f, 0.0f, 0.0f), t1, lens.n, intersect);
		result |= RaySphereIntersect(ray, s2, Vector3f( 1.0f, 0.0f, 0.0f), t2, lens.n, intersect);
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
		points.push_back(curRay.origin + curRay.direction * 25.0f);

		glLineWidth(1.5f);
		glBegin(GL_LINES);
		for (uint i = 0; i < points.size() - 1; ++i)
		{
			float n = ns[i];
			glColor4f(ns[i] - 1.0f, ns[i] - 0.5f, ns[i] * 2.0f - 1.0f, 1.0f);
			glVertex(points[i]);
			glVertex(points[i+1]);
		}
		glEnd();

		glPointSize(5.0f);
		glColor4ub(255, 128, 0, 255);
		glBegin(GL_POINTS);
		for (uint i = 0; i < points.size() - 1; ++i)
			glVertex(points[i]);
		glEnd();
	}



	void Lens()
	{
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
		{
			LensElement lens;
			lens.offset = 0.035f;
			lens.d = 0.005f;
			lens.r1 = -0.07f;
			lens.r2 = -0.07f;
			lens.diam = 0.065f;
			lens.n = 1.5f;
			lenses.push_back(lens);
		}

		{
			LensElement lens;
			lens.offset = 0.15f;
			lens.d = 0.025f;
			lens.r1 = 0.07f;
			lens.r2 = 0.07f;
			lens.diam = 0.065f;
			lens.n = 1.5f;
			lenses.push_back(lens);
		}

		while (protoGL.Update())
		{
			Vector2f cp = camera.CursorPosition();

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT);

			camera.CommitToGL();

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

			lenses[0].offset = cp.x;

			Rayf ray;
			ray.origin = Vector3f(0.0f, cp.y, 0.0f);
			ray.direction = Vector3f(1.0f, 0.0f, 0.0f);
			TraceRay(lenses.data(), lenses.size(), ray);

			ray.origin = Vector3f(0.0f, cp.y, 0.0f);
			ray.direction = Normalize(Vector3f(lenses[0].offset, lenses[0].diam * 0.25f, 0.0f) - ray.origin);
			TraceRay(lenses.data(), lenses.size(), ray);

			ray.origin = Vector3f(0.0f, cp.y, 0.0f);
			ray.direction = Normalize(Vector3f(lenses[0].offset,-lenses[0].diam * 0.25f, 0.0f) - ray.origin);
			TraceRay(lenses.data(), lenses.size(), ray);

			protoGL.Swap();
		}
	}


}


void Stuff()
{
	Lens();
}
