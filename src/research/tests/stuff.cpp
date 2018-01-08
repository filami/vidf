#include "pch.h"
#include "vidf/common/intersect.h"
#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/debug.h"
#include "vidf/rendererdx11/resources.h"
#include "vidf/rendererdx11/shaders.h"
#include "vidf/rendererdx11/pipeline.h"


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


namespace
{


using namespace vidf;
using namespace dx11;
using namespace proto;


typedef uint8 Color8[3];

// 445 040222-02 
const Color8 paletteColors[] =
{ { 140, 109, 204 },{ 117, 101, 203 },{ 109, 100, 208 },{ 102, 99, 213 },
{ 98, 93, 210 },{ 95, 88, 208 },{ 101, 89, 205 },{ 107, 90, 203 },
{ 133, 104, 206 },{ 150, 116, 217 },{ 167, 128, 229 },{ 184, 136, 240 },
{ 202, 144, 252 },{ 216, 146, 252 },{ 230, 149, 253 },{ 235, 149, 253 },
{ 240, 150, 254 },{ 247, 153, 254 },{ 240, 150, 254 },{ 233, 148, 254 },
{ 218, 141, 252 },{ 204, 134, 250 },{ 195, 128, 244 },{ 186, 122, 239 },
{ 151, 98, 207 },{ 132, 86, 188 },{ 114, 74, 170 },{ 96, 65, 156 },
{ 79, 57, 143 },{ 69, 52, 135 },{ 59, 47, 127 },{ 43, 39, 110 },
{ 29, 31, 91 },{ 8, 14, 53 },{ 5, 9, 42 },{ 2, 4, 32 },
{ 2, 4, 34 },{ 2, 5, 36 },{ 4, 8, 43 },{ 6, 12, 51 },
{ 16, 28, 82 },{ 20, 35, 97 },{ 24, 43, 112 },{ 23, 42, 112 },
{ 22, 42, 113 },{ 21, 42, 113 },{ 21, 42, 114 },{ 21, 42, 114 },
{ 21, 42, 114 },{ 24, 45, 118 },{ 21, 41, 108 },{ 19, 37, 99 },
{ 14, 29, 82 },{ 10, 21, 65 },{ 8, 17, 57 },{ 6, 13, 49 },
{ 1, 3, 26 },{ 0, 2, 24 },{ 0, 1, 22 },{ 0, 3, 27 },
{ 1, 6, 33 },{ 3, 9, 41 },{ 5, 13, 49 },{ 11, 21, 67 },
{ 17, 29, 85 },{ 29, 43, 118 },{ 30, 47, 126 },{ 32, 51, 134 },
{ 32, 52, 136 },{ 33, 53, 138 },{ 33, 54, 139 },{ 32, 54, 139 },
{ 35, 55, 142 },{ 37, 58, 149 },{ 39, 61, 157 },{ 45, 62, 160 },
{ 51, 63, 164 },{ 54, 62, 164 },{ 57, 62, 164 },{ 63, 60, 163 },
{ 71, 58, 161 },{ 90, 65, 176 },{ 108, 75, 193 },{ 127, 85, 211 },
{ 138, 89, 219 },{ 149, 94, 228 },{ 168, 103, 241 },{ 186, 108, 251 },
{ 208, 114, 254 },{ 214, 116, 254 },{ 221, 118, 254 },{ 222, 119, 254 },
{ 224, 120, 254 },{ 223, 121, 254 },{ 219, 121, 254 },{ 208, 119, 251 },
{ 191, 116, 242 },{ 156, 99, 212 },{ 137, 88, 193 },{ 119, 78, 175 },
{ 110, 73, 166 },{ 102, 68, 158 },{ 83, 60, 144 },{ 65, 52, 128 },
{ 32, 35, 93 },{ 21, 26, 74 },{ 10, 17, 55 },{ 7, 13, 47 },
{ 5, 10, 40 },{ 2, 4, 29 },{ 1, 1, 23 },{ 0, 0, 21 },
{ 0, 0, 20 },{ 0, 0, 20 },{ 0, 0, 21 },{ 0, 0, 22 },
{ 1, 0, 24 },{ 1, 2, 28 },{ 3, 5, 35 },{ 7, 12, 52 },
{ 24, 28, 89 },{ 35, 35, 107 },{ 47, 43, 126 },{ 52, 46, 133 },
{ 58, 49, 140 },{ 68, 53, 150 },{ 76, 60, 167 },{ 83, 67, 184 },
{ 87, 74, 200 },{ 88, 80, 212 },{ 88, 79, 211 },{ 88, 79, 210 },
{ 85, 76, 200 },{ 84, 73, 194 },{ 83, 73, 194 },{ 84, 75, 201 },
{ 82, 77, 205 },{ 74, 75, 200 },{ 67, 74, 195 },{ 62, 70, 186 },
{ 57, 67, 178 },{ 49, 62, 164 },{ 45, 58, 155 },{ 44, 55, 150 },
{ 47, 52, 145 },{ 47, 49, 137 },{ 45, 46, 129 },{ 43, 43, 122 },
{ 34, 37, 105 },{ 23, 29, 86 },{ 13, 21, 66 },{ 7, 13, 47 },
{ 1, 3, 21 },{ 1, 2, 18 },{ 1, 1, 16 },{ 1, 1, 14 },
{ 0, 0, 12 },{ 1, 0, 12 },{ 1, 1, 14 },{ 1, 1, 16 },
{ 2, 4, 22 },{ 8, 15, 46 },{ 10, 18, 54 },{ 12, 22, 63 },
{ 18, 29, 81 },{ 24, 37, 99 },{ 27, 43, 114 },{ 30, 48, 125 },
{ 29, 49, 128 },{ 28, 49, 128 },{ 27, 49, 129 },{ 28, 49, 129 },
{ 29, 48, 129 },{ 29, 47, 127 },{ 29, 47, 124 },{ 26, 41, 110 },
{ 22, 35, 96 },{ 15, 27, 78 },{ 10, 20, 61 },{ 5, 12, 46 },
{ 2, 5, 34 },{ 0, 2, 28 },{ 0, 0, 23 },{ 0, 0, 22 },
{ 0, 0, 21 },{ 0, 0, 21 },{ 0, 0, 22 },{ 0, 0, 23 },
{ 0, 0, 24 },{ 0, 0, 26 },{ 0, 0, 27 },{ 0, 1, 28 },
{ 0, 1, 29 },{ 0, 1, 30 },{ 0, 2, 30 },{ 0, 2, 30 },
{ 0, 2, 30 },{ 0, 2, 31 },{ 0, 2, 32 },{ 0, 3, 35 },
{ 1, 6, 40 },{ 3, 10, 51 },{ 9, 18, 67 },{ 18, 26, 85 },
{ 30, 35, 103 },{ 43, 44, 120 },{ 54, 51, 135 },{ 64, 56, 149 },
{ 68, 61, 161 },{ 69, 67, 174 },{ 69, 71, 183 },{ 70, 73, 188 },
{ 71, 76, 192 },{ 73, 79, 194 },{ 79, 82, 200 },{ 81, 83, 202 },
{ 81, 85, 206 },{ 76, 85, 210 },{ 71, 83, 209 },{ 65, 82, 206 },
{ 60, 78, 199 },{ 59, 78, 198 },{ 60, 79, 200 },{ 63, 82, 205 },
{ 66, 86, 215 },{ 66, 87, 215 },{ 64, 85, 212 },{ 60, 78, 198 },
{ 55, 71, 184 },{ 51, 64, 169 },{ 47, 59, 158 },{ 44, 58, 156 },
{ 42, 59, 155 },{ 43, 61, 158 },{ 47, 62, 159 },{ 55, 63, 159 },
{ 68, 63, 158 },{ 83, 64, 159 },{ 96, 67, 162 },{ 106, 73, 173 },
{ 116, 81, 190 },{ 125, 91, 208 },{ 134, 100, 225 },{ 149, 107, 239 },
{ 168, 113, 251 },{ 187, 119, 254 },{ 202, 122, 254 },{ 211, 126, 254 },
{ 216, 131, 254 },{ 216, 138, 254 },{ 200, 135, 252 },{ 186, 131, 250 },
{ 175, 129, 247 },{ 163, 122, 237 },{ 151, 114, 224 },{ 139, 105, 211 }
};


class Dx11CanvasListener : public CanvasListener
{
public:
	Dx11CanvasListener(ShaderManager& _shaderManager)
		: shaderManager(_shaderManager), restartRender(false) {}

	virtual void Close()
	{
		PostQuitMessage();
	}
	virtual void KeyDown(KeyCode keyCode)
	{
		if (keyCode == KeyCode::Escape)
			PostQuitMessage();
		else if (keyCode == KeyCode::F5)
		{
			shaderManager.RecompileShaders();
			restartRender = true;
		}
	}
	ShaderManager& shaderManager;
	bool restartRender;
};



class ComputeAverageBrightness
{
private:
	struct CBuffer
	{
		Vector2f invSize;
		Vector2f _;
	};

	struct Pass
	{
		ConstantBuffer cbuffer;
		RenderPassPtr renderPass;
		GraphicsPSOPtr pso;
		PD3D11ShaderResourceView inputSRV;
		RenderTarget output;
	};

public:
	void Prepare(RenderDevicePtr renderDevice, ShaderManager& shaderManager, uint width, uint height)
	{
		vsShader = shaderManager.CompileShaderFile("data/shaders/flame/display.hlsl", "vsFullscreen", ShaderType::VertexShader);
		psShader = shaderManager.CompileShaderFile("data/shaders/flame/display.hlsl", "psAverage", ShaderType::PixelShader);
		psShaderColor = shaderManager.CompileShaderFile("data/shaders/flame/display.hlsl", "psAverageColor", ShaderType::PixelShader);

		D3D11_SAMPLER_DESC samplerDesc{};
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		// samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = 0;
		renderDevice->GetDevice()->CreateSamplerState(&samplerDesc, &sampler.Get());

		bool first = true;
		Vector2i outputSize{ width, height };
		do
		{
			Vector2i inputSize = outputSize;
			outputSize.x = Max(1, outputSize.x / 4);
			outputSize.y = Max(1, outputSize.y / 4);

			CBuffer cbData;
			cbData.invSize = Vector2f(1.0f / inputSize.x, 1.0f / inputSize.y);
			ConstantBufferDesc cbDesc(sizeof(CBuffer), "cabCB");
			ConstantBuffer cbuffer = ConstantBuffer::Create(renderDevice, cbDesc);
			cbuffer.Update(renderDevice->GetContext(), cbData);

			RenderTargetDesc outputDesc{ DXGI_FORMAT_R32_FLOAT, uint(outputSize.x), uint(outputSize.y), "cabOutput" };
			RenderTarget output = RenderTarget::Create(renderDevice, outputDesc);

			GraphicsPSODesc psoDesc;
			psoDesc.rasterizer.CullMode = D3D11_CULL_NONE;
			psoDesc.rasterizer.FillMode = D3D11_FILL_SOLID;
			psoDesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			psoDesc.vertexShader = vsShader;
			psoDesc.pixelShader = first ? psShaderColor : psShader;
			GraphicsPSOPtr pso = GraphicsPSO::Create(renderDevice, psoDesc);

			D3D11_VIEWPORT viewport{};
			viewport.TopLeftX = 0;
			viewport.TopLeftY = 0;
			viewport.Width = uint(outputSize.x);
			viewport.Height = uint(outputSize.y);
			viewport.MinDepth = 0.0f;
			viewport.MaxDepth = 1.0f;

			RenderPassDesc renderPassDesc;
			renderPassDesc.viewport = viewport;
			renderPassDesc.rtvs[0] = output.rtv;
			RenderPassPtr renderPass = RenderPass::Create(renderDevice, renderPassDesc);

			Pass pass;
			pass.cbuffer = cbuffer;
			pass.output = output;
			pass.pso = pso;
			pass.renderPass = renderPass;
			if (!passes.empty())
				pass.inputSRV = passes.back().output.srv;
			passes.push_back(pass);

			first = false;
		} while (!(outputSize.x == 1 && outputSize.y == 1));
	}

	void Draw(CommandBuffer& commandBuffer, PD3D11ShaderResourceView inputSRV)
	{
		VIDF_GPU_EVENT(commandBuffer.GetRenderDevice(), ComputeAverageBrightness);

		passes[0].inputSRV = inputSRV;

		for (auto& pass : passes)
		{
			commandBuffer.BeginRenderPass(pass.renderPass);
			commandBuffer.SetGraphicsPSO(pass.pso);
			commandBuffer.SetConstantBuffer(0, pass.cbuffer.buffer);
			commandBuffer.GetContext()->PSSetSamplers(0, 1, &sampler.Get());
			commandBuffer.SetSRV(0, pass.inputSRV);
			commandBuffer.Draw(3, 0);
			commandBuffer.EndRenderPass();
		}
	}

	PD3D11ShaderResourceView GetOutputSRV() const { return passes.back().output.srv; }

private:
	std::vector<Pass> passes;
	ShaderPtr vsShader;
	ShaderPtr psShader;
	ShaderPtr psShaderColor;
	PD3D11SamplerState sampler;
};



void FlameGPU()
{
	const uint width = 1280;
	const uint height = 720;

	RenderDevicePtr renderDevice = RenderDevice::Create(RenderDeviceDesc());
	if (!renderDevice)
		return;
	ShaderManager shaderManager(renderDevice);
	CommandBuffer commandBuffer(renderDevice);

	Dx11CanvasListener canvasListener{ shaderManager };

	CanvasDesc canvasDesc{};
	canvasDesc.width = width;
	canvasDesc.height = height;
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	canvas->AddListener(&canvasListener);
	if (!canvas)
		return;

	SwapChainDesc swapChainDesc{};
	swapChainDesc.width = canvasDesc.width;
	swapChainDesc.height = canvasDesc.height;
	swapChainDesc.windowHandle = canvas->GetHandle();
	SwapChainPtr swapChain = renderDevice->CreateSwapChain(swapChainDesc);
	if (!swapChain)
		return;

	D3D11_VIEWPORT viewport{};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = canvasDesc.width;
	viewport.Height = canvasDesc.height;
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	RenderPassDesc finalizePassDesc;
	finalizePassDesc.viewport = viewport;
	finalizePassDesc.rtvs[0] = swapChain->GetBackBufferRTV();
	RenderPassPtr finalizePass = RenderPass::Create(renderDevice, finalizePassDesc);

	//

	ShaderPtr flameCS = shaderManager.CompileShaderFile("data/shaders/flame/flameCS.hlsl", "csFlame", ShaderType::ComputeShader);
	ShaderPtr fullscreenVS = shaderManager.CompileShaderFile("data/shaders/flame/display.hlsl", "vsFullscreen", ShaderType::VertexShader);
	ShaderPtr displayPS = shaderManager.CompileShaderFile("data/shaders/flame/display.hlsl", "psDisplay", ShaderType::PixelShader);

	std::vector<uint32> paletteData;
	paletteData.resize(sizeof(paletteColors)/sizeof(paletteColors[0]));
	for (uint i = 0; i < paletteData.size(); ++i)
		paletteData[i] = (paletteColors[i][2] << 16) | (paletteColors[i][1] << 8) | (paletteColors[i][0]);
	Texture2DDesc paletteDesc{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, uint(paletteData.size()), 1, "Palette" };
	paletteDesc.dataSize = paletteData.size() * sizeof(uint32);
	paletteDesc.dataPtr = paletteData.data();
	Texture2D palette = Texture2D::Create(renderDevice, paletteDesc);

	PD3D11SamplerState paletteSS;
	D3D11_SAMPLER_DESC paletteSSDesc{};
	paletteSSDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	paletteSSDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	paletteSSDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	paletteSSDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	paletteSSDesc.MinLOD = 0;
	paletteSSDesc.MaxLOD = 0;
	renderDevice->GetDevice()->CreateSamplerState(&paletteSSDesc, &paletteSS.Get());

	RWTexture2DDesc historgramDesc{ DXGI_FORMAT_R32G32B32A32_FLOAT, width, height, "Historgram" };
	RWTexture2D historgram = RWTexture2D::Create(renderDevice, historgramDesc);

	RenderPassDesc flameCSPassDesc;
	flameCSPassDesc.uavs[0] = historgram.uav;
	RenderPassPtr flameCSPass = RenderPass::Create(renderDevice, flameCSPassDesc);

	GraphicsPSODesc displayPSODesc;
	displayPSODesc.rasterizer.CullMode = D3D11_CULL_NONE;
	displayPSODesc.rasterizer.FillMode = D3D11_FILL_SOLID;
	displayPSODesc.topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	displayPSODesc.vertexShader = fullscreenVS;
	displayPSODesc.pixelShader = displayPS;
	GraphicsPSOPtr displayPSO = GraphicsPSO::Create(renderDevice, displayPSODesc);

	struct alignas(16) Float3
	{
		float x, y, z;
	};
	typedef std::array<Float3, 2> Float2x3;

	struct ViewConsts
	{
		Matrix44f projViewTM;
		Vector3f cameraPos;
		float time;
		Vector2i viewport;
		uint rngOffset;
		float aspectRatio;
	};

	ConstantBufferDesc viewCBDesc(sizeof(ViewConsts), "viewCB");
	ConstantBuffer viewCB = ConstantBuffer::Create(renderDevice, viewCBDesc);

	//

	TimeCounter counter;
	
	const uint numThreads = 64;
	// const uint dispatchSize = 1024 * 8;
	const uint dispatchSize = 1024 * 32;
	
	ViewConsts viewConsts;
	viewConsts.viewport = Vector2i(width, height);
	viewConsts.rngOffset = 0;
	viewConsts.aspectRatio = width / float(height);
	// viewConsts.cameraTM[0] = { 0.45f * (float(height)/ width), 0.0f, 0.5f };
	// viewConsts.cameraTM[1] = { 0.0f, 0.45f, 0.5f };

	// const Vector3f camPos = Vector3f(0.0f, 1.0f, 0.75f);
	// const Vector3f camTarget = Vector3f(0.0f, 0.0f, 0.0f);

	// const Vector3f camPos = Vector3f(0.0f, 1.0f, 0.35f);
	// const Vector3f camTarget = Vector3f(0.0f, 0.2f, 0.1f);

	Vector3f camPos = Vector3f(0.0f, 1.0f, 0.75f);
	Vector3f camTarget = Vector3f(0.0f, -0.2f, 0.1f);

	ComputeAverageBrightness computeAverageBrightness;
	computeAverageBrightness.Prepare(renderDevice, shaderManager, width, height);

	// bool animate = true;
	bool animate = false;
		
	Time time;
	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		Time deltaTime = counter.GetElapsed();
		time += deltaTime;

		if (animate || canvasListener.restartRender)
		{
			VIDF_GPU_EVENT(renderDevice, ClearHistorgram);
			const float zero[] = {0, 0, 0, 0};
			commandBuffer.GetContext()->ClearUnorderedAccessViewFloat(historgram.uav, zero);
			viewConsts.rngOffset = 0;
			canvasListener.restartRender = false;
		}

		if (animate)
		{
			time += counter.GetElapsed();

			const float angle = time.AsFloat() * 0.25f;
			camPos = Vector3f(std::cos(angle)*0.75f, std::sin(angle)*0.75f, 0.5f) * 1.5f;
			camTarget = Vector3f(0.0f, 0.0f, 0.1f);
			// camPos = Vector3f(0.5f, 0.5f, 0.15f);
			// camTarget = Vector3f(0.0f, 0.0f, 0.0f);
		}
		else
		{
			time = Time(0.0f);
			// camPos = Vector3f(0.0f, 0.75f, 1.5f) * 1.5f;
			camPos = Vector3f(0.25f, 0.75f, 0.5f) * 1.75f;
			camTarget = Vector3f(0.0f, 0.0f, 0.0f);
		}
				
		Matrix44f projTM = PerspectiveFovLH(1.4f, width / float(height), 0.01f, 100.0f);
		Matrix44f viewTM = LookAtLH(camPos, camTarget, Vector3f(0.0f, 0.0f, 1.0f));
		viewConsts.projViewTM = Mul(viewTM, projTM);
		viewConsts.cameraPos = camPos;
		viewConsts.time = time.AsFloat();
		viewCB.Update(renderDevice->GetContext(), viewConsts);
		viewConsts.rngOffset += numThreads * dispatchSize;

		{
			VIDF_GPU_EVENT(renderDevice, FlameCS);

			commandBuffer.BeginComputePass(flameCSPass);

			commandBuffer.SetCompute(flameCS);
			commandBuffer.SetConstantBuffer(0, viewCB.buffer);
			commandBuffer.SetSRV(0, palette.srv);
			commandBuffer.GetContext()->CSSetSamplers(0, 1, &paletteSS.Get());
			commandBuffer.Dispatch(Vector3i(int(dispatchSize), 1, 1));

			commandBuffer.EndComputePass();
		}

		computeAverageBrightness.Draw(commandBuffer, historgram.srv);

		{
			VIDF_GPU_EVENT(renderDevice, Display);

			commandBuffer.BeginRenderPass(finalizePass);

			commandBuffer.SetGraphicsPSO(displayPSO);
			commandBuffer.SetConstantBuffer(0, viewCB.buffer);
			commandBuffer.SetSRV(0, historgram.srv);
			commandBuffer.SetSRV(1, computeAverageBrightness.GetOutputSRV());
			commandBuffer.Draw(3, 0);

			commandBuffer.EndRenderPass();
		}

		const bool vsync = !animate;
		swapChain->Present(vsync);
	}
}



}


void Stuff()
{
	// Lens();
	FlameGPU();
}
