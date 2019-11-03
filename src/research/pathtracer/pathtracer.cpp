#include "pch.h"
#include <iomanip>
#include "kdtree.h"
#include "halton.h"
#include "vidf/common/transformations.h"
#include "vidf/common/random.h"
#include "vidf/rendererdx11/renderdevice.h"
#include "vidf/rendererdx11/resources.h"
#include "proto/mesh.h"
#define POWITACQ_IMPLEMENTATION
#include "powitacq_rgb.h"


using namespace vidf;
using namespace vipt;
using namespace dx11;
using namespace proto;



namespace std
{


	constexpr inline std::size_t hash_combine(std::size_t hash1, std::size_t hash2)
	{
		return hash1 ^ (hash2 * 0x9e3779b9 + (hash1 << 6) + (hash1 >> 2));
	}


	template<> struct hash<vidf::Vector3i>
	{
		typedef vidf::Vector3i argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const& s) const noexcept
		{
			result_type const h1(std::hash<int>{}(s.x));
			result_type const h2(std::hash<int>{}(s.y));
			result_type const h3(std::hash<int>{}(s.z));
			return hash_combine(hash_combine(h1, h2), h3);
		}
	};


}



namespace
{



#if 0
class Sampler
{
public:
	Sampler(Recti bounds, uint _samplesPerPixel)
	{
	}

	void SetPixel(Vector2i px)
	{
	}

	void SetSample(uint smpIdx)
	{
	}

	float GetSample1D()
	{
		return unorm(rand48);
	}

	Vector2f GetSample2D()
	{
		return Vector2f(unorm(rand48), unorm(rand48));
	}

private:
	vidf::Rand48 rand48;
	vidf::UniformReal<float> unorm{ 0.0f, 1.0f };
};

#else

class Sampler
{
public:
	Sampler(Recti bounds, uint _samplesPerPixel)
		: sampler(_samplesPerPixel, bounds)
	{
	}

	void SetPixel(Vector2i px)
	{
		sampler.StartPixel(px);
	}

	void SetSample(uint smpIdx)
	{
		sampler.SetSample(smpIdx);
	}

	float GetSample1D()
	{
		return sampler.Get1D();
	}

	Vector2f GetSample2D()
	{
		return sampler.Get2D();
	}

private:
	HaltonSampler sampler;
};

#endif



struct NormalBasis
{
	Vector3f xx;
	Vector3f yy;
	Vector3f normal;
};



inline float Sqr(float v)
{
	return v * v;
}



Vector3f Transform(NormalBasis basis, Vector3f dir)
{
	return basis.xx * dir.x + basis.yy * dir.y + basis.normal * dir.z;
}



Vector3f InvTransform(NormalBasis basis, Vector3f dir)
{
	return Vector3f(Dot(basis.xx, dir), Dot(basis.yy, dir), Dot(basis.normal, dir));
}



Vector3f Reflect(Vector3f normal, Vector3f dir)
{
	return dir - 2.0f * Dot(dir, normal) * normal;
}



Vector3f Refract(Vector3f normal, Vector3f dir, float ior)
{
	const float ndi = Dot(normal, dir);
	if (ndi < 0.0f)
		return Refract(-normal, dir, 1.0f / ior);
	const float k = 1.0f - ior * ior * (1.f - ndi * ndi);
	if (k < 0.f)
		return Vector3f(0.0f, 0.0f, 0.0f);
	else
		return ior * dir - (ior * ndi + sqrt(k)) * normal;
}



NormalBasis NormalBasisFromNormal(Vector3f normal)
{
	NormalBasis basis;
	basis.xx = Cross(normal, Vector3f(0.0f, 0.0f, 1.0f));
	if (std::abs(Dot(basis.xx, basis.xx)) < (1.0f / 1024.0f))
		basis.xx = Vector3f(0.0f, 1.0f, 0.0f);
	else
		basis.xx = Normalize(basis.xx);
	basis.yy = Normalize(Cross(basis.xx, normal));
	basis.normal = normal;
	return basis;
}



template <int base>
static float RadicalInverseSpecialized(uint64_t a)
{
	const float OneMinusEpsilon = 1.0f - std::numeric_limits<float>::epsilon();
	const float invBase = (float)1 / (float)base;
	uint64_t reversedDigits = 0;
	float invBaseN = 1;
	while (a) {
		uint64_t next = a / base;
		uint64_t digit = a - next * base;
		reversedDigits = reversedDigits * base + digit;
		invBaseN *= invBase;
		a = next;
	}
	return std::min(reversedDigits * invBaseN, OneMinusEpsilon);
}



Vector2f SampleCircle(Vector2f sample)
{
	const float u = sample.x * PI * 2.0f;
	const float v = std::sqrt(sample.y);
	return Vector2f(std::cos(u), std::sin(u)) * v;
}



inline float PowerHeuristic(float f, float g)
{
	return (f * f) / (f * f + g * g);
}



Vector3f SampleCosineHemisphere(Vector3f normal, Vector2f sample)
{
	Vector3f xx = Cross(normal, Vector3f(0.0f, 0.0f, 1.0f));
	if (std::abs(Dot(xx, xx)) < (1.0f / 1024.0f))
		xx = Vector3f(0.0f, 1.0f, 0.0f);
	else
		xx = Normalize(xx);
	Vector3f yy = Normalize(Cross(xx, normal));
	float u = sample.x * PI * 2.0f;
	float r = std::sqrt(sample.y);
	Vector3f d;
	d.x = std::cos(u) * r;
	d.y = std::sin(u) * r;
	d.z = std::sqrt(1.0f - d.x * d.x - d.y * d.y);
	return d.x * xx + d.y * yy + d.z * normal;
}



Vector3f SampleCosineHemisphere(Vector2f sample)
{
	float u = sample.x * PI * 2.0f;
	float r = std::sqrt(sample.y);
	Vector3f d;
	d.x = std::cos(u) * r;
	d.y = std::sin(u) * r;
	d.z = std::sqrt(1.0f - d.x * d.x - d.y * d.y);
	return d;
}



inline Vector3f SphericalDirection(float sinTheta, float cosTheta, float phi)
{
	return Vector3f(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);
}



inline bool SameHemisphere(Vector3f w, Vector3f wp)
{
	return w.z * wp.z > 0;
}



Vector3f SamplePowerHemisphere(Vector2f sample, float alpha)
{
	const float logSample = log(1.0f - sample.x);
	const float tan2Theta = -Sqr(alpha) * logSample;
	const float phi = sample.y * 2 * PI;

	const float cosTheta = 1 / sqrt(1 + tan2Theta);
	const float sinTheta = sqrt(Max(0.0f, 1 - cosTheta * cosTheta));

	const Vector3f wh = SphericalDirection(sinTheta, cosTheta, phi);
	if (!SameHemisphere(Vector3f(0, 0, 1), wh))
		return -wh;
	return wh;
}



Vector3f SampleHemisphere(Vector2f sample)
{
	float z = sample.x;
	float r = std::sqrt(Max(0.0f, 1.0f - z * z));
	float phi = 2.0f * PI * sample.y;
	return Vector3f(r * std::cos(phi), r * std::sin(phi), z);
}



Vector3f SampleCone(Vector3f dir, Vector2f sample, float angleTan)
{
	Vector3f xx = Cross(dir, Vector3f(0.0f, 0.0f, 1.0f));
	if (std::abs(Dot(xx, xx)) < 0.99f)
		xx = Vector3f(0.0f, 1.0f, 0.0f);
	else
		xx = Normalize(xx);
	Vector3f yy = Normalize(Cross(xx, dir));
	float u = sample.x * PI * 2.0f;
	float r = std::sqrt(sample.y) * angleTan;
	Vector3f d;
	d.x = std::cos(u) * r;
	d.y = std::sin(u) * r;
	d.z = std::sqrt(1.0f - d.x * d.x - d.y * d.y);
	return Normalize(d.x * xx + d.y * yy + d.z * dir);
}



inline float RayPlaneIntersect(const Rayf ray, const Planef plane)
{
	Vector3f origin = ray.origin - plane.normal * plane.distance;
	float a = Dot(plane.normal, ray.direction);
	float b = -Dot(plane.normal, origin) / a;
	return b;
}



inline float Schlick(float cost, float n1, float n2)
{
	const float r0 = Sqr((n1 - n2)/(n1 + n2));
	return r0 + (1 - r0) * pow(1 - cost, 5.0f);
}


//////////////////////////////////////////////////////////////



struct Surfel
{
	Vector3f point;
	Vector3f normal;
	Color    diffuse;
	Color    emissive;
	float    radius;
};



struct Vertex
{
	Vector3f position;
	Vector3f normal;
};



struct Batch
{
	uint firstIndex;
	uint numIndices;
};



class RenderMesh
{
public:
	void AddBatch(const Vertex* _vertices, uint numVertices, const uint* _indices, uint numIndices, uint materialId);

	const std::vector<Vertex>& GetVertices() const { return vertices; }
	const std::vector<uint>&   GetIndices() const { return indices; }
	const std::vector<uint>&   GetMaterialIds() const { return materialIds; }
	const std::vector<Batch>&  GetBatches() const { return batches; }

private:
	std::vector<Vertex> vertices;
	std::vector<uint> materialIds;
	std::vector<uint> indices;
	std::vector<Batch> batches;
};



void RenderMesh::AddBatch(const Vertex* _vertices, uint numVertices, const uint* _indices, uint numIndices, uint materialId)
{
	Batch batch;
	batch.firstIndex = vertices.size();
	batch.numIndices = numIndices;

	indices.resize(indices.size() + numIndices);
	for (uint i = 0; i != batch.numIndices; ++i)
		indices[batch.firstIndex + i] = batch.firstIndex + _indices[i];

	vertices.insert(vertices.end(), _vertices, _vertices + numVertices);

	materialIds.resize(indices.size() / 3, materialId);

	batches.push_back(batch);
}



//////////////////////////////////////////////////////////////



class PhysicalCamera
{
public:
	PhysicalCamera() = default;
	PhysicalCamera(Vector3f _position, Quaternionf _rotation, float _aspectRatio, float _focusDistance, float _coc)
	{
		Prepare(_position, _rotation, _aspectRatio);
		focusDistance = _focusDistance;
		coc = _coc;
	}

	Rayf Sample(Vector2f coord, Sampler& sampler);

private:
	void Prepare(Vector3f _position, Quaternionf _quaternion, float _aspectRatio);

private:
	Vector3f origin;
	Vector3f front;
	Vector3f right;
	Vector3f up;
	float focusDistance = 300.0f;
	float coc = 5.0f;
};



Rayf PhysicalCamera::Sample(Vector2f coord, Sampler& sampler)
{
	const Vector3f camDir = Normalize(front + right * coord.x + up * coord.y);
	const Planef focusPlane = NormalPointPlane(-front, origin + front * focusDistance);

	Rayf ray{ zero };
	ray.origin = origin;
	ray.direction = Normalize(front + right * coord.x + up * coord.y);
	const Vector3f focusPt = origin + ray.direction * RayPlaneIntersect(ray, focusPlane);

	const Vector2f cocSmp = SampleCircle(sampler.GetSample2D()) * coc;
		
	ray.origin = origin + right * cocSmp.x + up * cocSmp.y;
	ray.direction = Normalize(focusPt - ray.origin);
	return ray;
}



void PhysicalCamera::Prepare(Vector3f _position, Quaternionf _rotation, float _aspectRatio)
{
	origin = _position;
	right = Rotate(_rotation, Vector3f(1, 0, 0)) * _aspectRatio;
	front = Rotate(_rotation, Vector3f(0, 1, 0));
	up = Rotate(_rotation, Vector3f(0, 0, 1));
}



//////////////////////////////////////////////////////////////



template<typename PixelType>
class FrameBuffer
{
public:
	FrameBuffer() = default;
	FrameBuffer(uint width, uint height, PixelType defaultVal);

	void  SetPixel(Vector2i coord, PixelType pixel);
	PixelType GetPixel(Vector2i coord) const;
	uint GetWidth() const { return width; }
	uint GetHeight() const { return height; }

private:
	std::vector<PixelType> data;
	uint width = 0;
	uint height = 0;
};



template<typename PixelType>
FrameBuffer<PixelType>::FrameBuffer(uint _width, uint _height, PixelType defaultVal)
	: width(_width)
	, height(_height)
{
	const uint dataSize = width * height * 3;
	data.resize(dataSize, defaultVal);
}



template<typename PixelType>
void FrameBuffer<PixelType>::SetPixel(Vector2i coord, PixelType pixel)
{
	const uint pixelOffset = coord.x + coord.y * width;
	data[pixelOffset] = pixel;
}



template<typename PixelType>
PixelType FrameBuffer<PixelType>::GetPixel(Vector2i coord) const
{
	const uint pixelOffset = coord.x + coord.y * width;
	return data[pixelOffset];
}



template<typename PixelType>
void FrameBufferCopy(FrameBuffer<PixelType>* destBuffer, Vector2i destPos, const FrameBuffer<PixelType>& srcBuffer, Vector2i srcPos)
{
	const uint width = std::min(destBuffer->GetWidth() - destPos.x, srcBuffer.GetWidth() - srcPos.x);
	const uint height = std::min(destBuffer->GetHeight() - destPos.y, srcBuffer.GetHeight() - srcPos.y);

	for (uint y = 0; y < height; ++y)
	{
		for (uint x = 0; x < width; ++x)
		{
			const Vector2i px{x, y};
			destBuffer->SetPixel(px + destPos, srcBuffer.GetPixel(px + srcPos));
		}
	}
}



//////////////////////////////////////////////////////////////


void BuildRenderMeshGeometry(RenderMesh* renderMesh, const Module& model, uint geomIdx, bool flip)
{
	const Module::Geometry& geometry = model.GetGeometry(geomIdx);

	std::vector<Vertex> vertices;
	std::vector<uint> indices;

	const bool hasNormals = model.HasNormals();
	const uint lastPolygon = geometry.firstPolygon + geometry.numPolygons;
	uint curMaterialId = -1;
	for (uint polyIdx = geometry.firstPolygon; polyIdx != lastPolygon; ++polyIdx)
	{
		if (curMaterialId != model.GetPolygonMaterialIndex(polyIdx))
		{
			if (!vertices.empty())
			{
				renderMesh->AddBatch(
					vertices.data(), vertices.size(),
					indices.data(), indices.size(),
					curMaterialId);
				vertices.clear();
				indices.clear();
			}
			curMaterialId = model.GetPolygonMaterialIndex(polyIdx);
		}

		const uint numVertices = model.GetPolygonNumVertices(polyIdx);

		Vertex vertex[3];
		vertex[0].position = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, 0));
		vertex[1].position = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, 1));
		if (hasNormals)
		{
			vertex[0].normal = model.GetNormal(model.GetPolygonNormalIndex(polyIdx, 0));
			vertex[1].normal = model.GetNormal(model.GetPolygonNormalIndex(polyIdx, 1));
		}

		for (uint vertIdx = 2; vertIdx < numVertices; ++vertIdx)
		{
			vertex[2].position = model.GetVertex(model.GetPolygonVertexIndex(polyIdx, vertIdx));
			if (hasNormals)
			{
				vertex[2].normal = model.GetNormal(model.GetPolygonNormalIndex(polyIdx, vertIdx));
			}
			else
			{
				Vector3f normal = Normalize(Cross(vertex[1].position - vertex[0].position, vertex[2].position - vertex[0].position));
				if (flip)
					normal = -normal;
				vertex[0].normal = normal;
				vertex[1].normal = normal;
				vertex[2].normal = normal;
			}
			if (!flip)
			{
				vertices.push_back(vertex[0]);
				vertices.push_back(vertex[1]);
				vertices.push_back(vertex[2]);
			}
			else
			{
				vertices.push_back(vertex[0]);
				vertices.push_back(vertex[2]);
				vertices.push_back(vertex[1]);
			}
			indices.push_back(indices.size());
			indices.push_back(indices.size());
			indices.push_back(indices.size());
			vertex[1] = vertex[2];
		}
	}

	if (!vertices.empty())
	{
		renderMesh->AddBatch(
			vertices.data(), vertices.size(),
			indices.data(), indices.size(),
			curMaterialId);
		vertices.clear();
		indices.clear();
	}
}




void BuildRenderMesh(RenderMesh* renderMesh, const Module& model, bool flip)
{
	const uint numGeoms = model.GetNumGeometries();
	for (uint geomIdx = 0; geomIdx < numGeoms; ++geomIdx)
		BuildRenderMeshGeometry(renderMesh, model, geomIdx, flip);
}



void BuildKdTree(KdTreeAccel* kdTree, const RenderMesh& renderMesh)
{
	const auto& vertices = renderMesh.GetVertices();
	const auto& indices = renderMesh.GetIndices();
	std::vector<Primitive> primitives;
	primitives.resize(indices.size() / 3);

	for (uint i = 0; i < primitives.size(); ++i)
	{
		primitives[i].vertices[0] = vertices[indices[i * 3 + 0]].position;
		primitives[i].vertices[1] = vertices[indices[i * 3 + 1]].position;
		primitives[i].vertices[2] = vertices[indices[i * 3 + 2]].position;
	}

	kdTree->Build(primitives);
}



void CopyFrameBufferWithExposureSRGB(uint32* gpuBuffer, const FrameBuffer<Color>& input, float exposure)
{
	auto Exposure = [exposure](float c) { /*return 1.0f - std::exp(-c * exposure);*/ return c; };
//	auto LinearToSRGB = [](float c) { return c < 0.0031308f ? 12.92f * c : (1 + 0.055f) * pow(c, 1 / 2.4f); };
	auto LinearToSRGB = [](float c) { return pow(c, 1.0f / 2.2f); };

	auto ColorToUint32 = [](Color color)
	{
		return
			(0xff << 24) |
			(uint8(Saturate(color.b) * 255.0f) << 16) |
			(uint8(Saturate(color.g) * 255.0f) << 8) |
			uint8(Saturate(color.r) * 255.0f);
	};

	for (uint y = 0; y < input.GetHeight(); ++y)
	{
		for (uint x = 0; x < input.GetWidth(); ++x)
		{
			Color color = input.GetPixel(Vector2i(x, y));
			color.r = LinearToSRGB(Exposure(color.r));
			color.g = LinearToSRGB(Exposure(color.g));
			color.b = LinearToSRGB(Exposure(color.b));
			*gpuBuffer = ColorToUint32(color);
			++gpuBuffer;
		}
	}
}



void DrawBukets(uint32* gpuBuffer, const std::vector<Recti>& buckets, uint width, uint height)
{
	const uint32 color = ~0u;
	for (uint i = 0; i < buckets.size(); ++i)
	{
		Recti bucket = buckets[i];
		for (uint j = bucket.min.x; j < bucket.max.x; ++j)
		{
			gpuBuffer[bucket.min.y * width + j] = color;
			gpuBuffer[bucket.max.y * width + j] = color;
		}
		for (uint j = bucket.min.y; j < bucket.max.y; ++j)
		{
			gpuBuffer[j * width] = color;
			gpuBuffer[j * width + bucket.max.x] = color;
		}
	}
}



//////////////////////////////////////////////////////////////



Color SampleBackground(Vector3f direction)
{
	const float intensity = 5.0f;
	return Color(
		intensity * 0.35f,
		intensity * 0.75f,
		intensity * 1.0f);
}



//////////////////////////////////////////////////////////////


class Renderer;
class Material;



struct Photon
{
	Vector3f point;
	Vector3f normal;
	Color    color;
	float    radius;
};



class PhotonMap
{
public:
	void AddPhoton(Photon photon)
	{
		photons.push_back(photon);
	}

	void BuildMap()
	{
		std::vector<Primitive> photonTris;
		photonTris.reserve(photons.size());

		for (Photon photon : photons)
		{
			Primitive primitive;
			primitive.vertices[0] = photon.point - Vector3f(photon.radius, photon.radius, photon.radius);
			primitive.vertices[1] = photon.point + Vector3f(photon.radius, photon.radius, photon.radius);
			primitive.vertices[2] = photon.point + Vector3f(photon.radius, 0.0f, 0.0f);
			photonTris.push_back(primitive);
		}

		kdTree.Build(photonTris);
	}

	Color SamplePoint(Vector3f point, Vector3f normal) const
	{
		std::vector<uint> indices;
		kdTree.QueryPrimitives(point, &indices);

		Color color{0.0f, 0.0f, 0.0f};
		float weigthSum = 0.0f;
		for (uint photonIdx : indices)
		{
			const Photon photon = photons[photonIdx];
			const float distance = Saturate(Distance(photon.point, point) / photon.radius);
			const float weigth = Hermite1(distance) * Max(0.0f, Dot(normal, photon.normal));
			color = color + photon.color * weigth;
			// weigthSum += weigth;
		}

		return color * (1.0f / Max(1.0f, weigthSum));
	}

// private:
	std::deque<Photon> photons;
	KdTreeAccel        kdTree;
};



class RadiosityMap
{
public:
	void BuildMap(float _radius)
	{
		radius = _radius;

		for (uint i = 0; i < surfels.size(); ++i)
		{
			Vector3i pt = Vector3i((surfels[i].point - Vector3f(radius, radius, radius)) / (radius * 2.0f));
			PushIndex(i, pt + Vector3i(0, 0, 0));
			PushIndex(i, pt + Vector3i(1, 0, 0));
			PushIndex(i, pt + Vector3i(0, 1, 0));
			PushIndex(i, pt + Vector3i(1, 1, 0));
			PushIndex(i, pt + Vector3i(0, 0, 1));
			PushIndex(i, pt + Vector3i(1, 0, 1));
			PushIndex(i, pt + Vector3i(0, 1, 1));
			PushIndex(i, pt + Vector3i(1, 1, 1));
		}

		directLight.resize(surfels.size(), Color{ zero });
		indirectLight.resize(surfels.size(), Color{ zero });
	}

	Color SamplePoint(Vector3f point, Vector3f normal) const
	{
		const Vector3i pt = Vector3i(point / (radius * 2.0f));
		auto it = hashGrid.find(pt);
		if (it == hashGrid.end())
			return Color(0.0f, 0.0f, 0.0f);

		Color color{ 0.0f, 0.0f, 0.0f };
		float weigthSum = 0.0f;
		for (uint idx : it->second)
		{
			const Surfel surfel = surfels[idx];
			const float distance = Saturate(Distance(surfel.point, point) / surfel.radius);
			const float weigth = Hermite1(distance) * Max(0.0f, Dot(normal, surfel.normal));
			color = color + (directLight[idx] + indirectLight[idx]) * weigth;
			weigthSum += weigth;
		}

		return color * (1.0f / Max(1.0f, weigthSum));
	}

	void PushIndex(uint index, Vector3i coord)
	{
		hashGrid[coord].push_back(index);
	}

// public:
	std::deque<Surfel> surfels;
	std::vector<Color> directLight;
	std::vector<Color> indirectLight;

	std::unordered_map<Vector3i, std::vector<uint>> hashGrid;
	float radius;
};



struct Fragment
{
	Material* material = nullptr;
	uint      materialId = 0;

	Vector3f  point;
	Vector3f  normal;
	float     distance = std::numeric_limits<float>::max();

	Color diffuse = Color(1.0f, 1.0f, 1.0f);
	Color emissive = Color(zero);
	float alpha = 0.15f;
	float ior = 1.33f;
	float transparency = 0.0f;
};



class Background
{
public:
	virtual Color Sample(Vector3f direction) const = 0;
};

class Light
{
public:
	virtual void Sample(Color* color, Vector3f* wi, float* visDist, Vector2f sample, const Fragment& fragment) const = 0;
};

class RenderObject
{
public:
	virtual bool Raytrace(Fragment* outFragment, Rayf ray) const = 0;
	virtual bool Raytrace(Rayf ray) const = 0;
	virtual void GenerateSurfels(const Matrix44f& worldTM, float density, float radius, const Material& material, Sampler& sampler, std::deque<Surfel>& outSurfels) const = 0;
};

class Material
{
public:
	virtual void Sample(Fragment* inOutFragment) const = 0;
	virtual std::shared_ptr<Material> GetSubMaterial(uint subMaterialIdx) const { return nullptr; }
	virtual Color Evaluate(const Fragment& fragment, const Vector3f wo, const Vector3f wi) const { return Color(0, 0, 0); }
	virtual Color Sample(const Fragment& fragment, Vector2f sample, Vector3f wo, Vector3f* wi, float* pdf) const { return Color(0, 0, 0); }
	virtual float PDF(const Fragment& fragment, Vector3f wo, Vector3f wi) const { return 0.0f; }
};



class BasicMaterial : public Material
{
public:
	BasicMaterial(Color _diffuse, Color _emissive)
		: diffuse(_diffuse)
		, emissive(_emissive) {}
	BasicMaterial(Color _diffuse, Color _emissive, float _shininess, float _ior)
		: diffuse(_diffuse)
		, emissive(_emissive)
		, shininess(_shininess)
		, ior(_ior) {}
	BasicMaterial(Color _diffuse, Color _emissive, float _shininess, float _ior, float _transparency)
		: diffuse(_diffuse)
		, emissive(_emissive)
		, shininess(_shininess)
		, ior(_ior)
		, transparency(_transparency){}

	void Sample(Fragment* inOutFragment) const override
	{
		inOutFragment->diffuse = diffuse;
		inOutFragment->emissive = emissive;
		inOutFragment->alpha = 1 / shininess;
		inOutFragment->ior = ior;
		inOutFragment->transparency = transparency;
	}

	Color Evaluate(const Fragment& fragment, const Vector3f wo, const Vector3f wi) const override
	{
		Color color = Color(0, 0, 0);

		const float fresnel = Max(0.0f, Schlick(wi.z, 1.0f, ior));

	//	color = Max(0.0f, wi.z) * diffuse;
		color = diffuse;

		//	color = color + (Max(0.0f, Dot(wi, wn)) / PI) * (1.0f - fresnel) * fragment.diffuse * lightColor;
		/*
		const Vector3f wh = Normalize(wo + wi);
		const float alpha = fragment.alpha;

		const float nh = Dot(wn, wh);
		const float ni = Dot(wn, wi);
		const float no = Dot(wn, wo);
		const float alpha2 = alpha * alpha;
		const float D = alpha2 / (PI * Sqr(Sqr(nh) * (alpha2 - 1) + 1));
		const float G = ni * no;
		const float pdf = D * G * abs(Dot(wo, wh)) / abs(no);
		color = color + (pdf / (4 * Dot(wo, wh))) * fresnel * lightColor;
		*/
		return color;
	}

	Color Sample(const Fragment& inOutFragment, Vector2f sample, Vector3f wo, Vector3f* wi, float* pdf) const override
	{
		*wi = SampleCosineHemisphere(sample);
		*pdf = abs(wi->z) * PI;
		return diffuse / PI;
	}

	virtual float PDF(const Fragment& inOutFragment, Vector3f wo, Vector3f wi) const override
	{
		return abs(wi.z) * PI;
	}

private:
	Color diffuse;
	Color emissive;
	float shininess = 1.5f;
	float ior = 1.33f;
	float transparency = 0.0f;
};


class MultiMaterial : public Material
{
public:
	void Sample(Fragment* inOutFragment) const override
	{
		materials[inOutFragment->materialId % materials.size()]->Sample(inOutFragment);
	}

	Color Evaluate(const Fragment& fragment, const Vector3f wo, const Vector3f wi) const override
	{
		return materials[fragment.materialId % materials.size()]->Evaluate(fragment, wo, wi);
	}

	Color Sample(const Fragment& fragment, Vector2f sample, Vector3f wo, Vector3f* wi, float* pdf) const override
	{
		return materials[fragment.materialId % materials.size()]->Sample(fragment, sample, wo, wi, pdf);
	}

	virtual float PDF(const Fragment& fragment, Vector3f wo, Vector3f wi) const override
	{
		return materials[fragment.materialId % materials.size()]->PDF(fragment,  wo, wi);
	}

	virtual std::shared_ptr<Material> GetSubMaterial(uint subMaterialIdx) const override
	{
		return materials[subMaterialIdx % materials.size()];
	}

	void AddMaterial(std::shared_ptr<Material> material)
	{
		materials.push_back(material);
	}

private:
	std::vector<std::shared_ptr<Material>> materials;
};



class MeasuredMaterial : public Material
{
public:
	MeasuredMaterial(const char* name)
		: brdf(name) {}

	void Sample(Fragment* inOutFragment) const override
	{
	}
	
	Color Evaluate(const Fragment& fragment, const Vector3f wo, const Vector3f wi) const override
	{
		auto value = brdf.eval(
			powitacq_rgb::Vector3f(wi.x, wi.y, wi.z),
			powitacq_rgb::Vector3f(wo.x, wo.y, wo.z));
		return Color(value.x(), value.y(), value.z());
		// return Color(pow(value.x(), 2.2), pow(value.y(), 2.2), pow(value.z(), 2.2));
	}
	
	Color Sample(const Fragment& fragment, Vector2f sample, Vector3f wo, Vector3f* wi, float* pdf) const override
	{
		powitacq_rgb::Vector3f _wi;
		float _pdf;
		auto value = brdf.sample(
			powitacq_rgb::Vector2f(sample.x, sample.y),
			powitacq_rgb::Vector3f(wo.x, wo.y, wo.z),
			&_wi,
			pdf);
		*wi = Vector3f(_wi.x(), _wi.y(), _wi.z());
		return Color(value.x(), value.y(), value.z());
		// return Color(pow(value.x(), 2.2), pow(value.y(), 2.2), pow(value.z(), 2.2));
	}

	virtual float PDF(const Fragment& fragment, Vector3f wo, Vector3f wi) const override
	{
		return brdf.pdf(
			powitacq_rgb::Vector3f(wo.x, wo.y, wo.z),
			powitacq_rgb::Vector3f(wi.x, wi.y, wi.z));
	}

private:
	powitacq_rgb::BRDF brdf;
};



class SkyLight : public Background
{
public:
	SkyLight() = default;
	explicit SkyLight(Color _color)
		: color(_color) {}

	Color Sample(Vector3f direction) const override
	{
		return color;
	};

private:
	Color color{zero};
};



class DirectLight : public Light
{
public:
	DirectLight(Color _color, Vector3f _direction, float angle)
		: color(_color)
		, direction(Normalize(_direction))
		, angleTan(std::tan(angle)) {}

	virtual void Sample(Color* color, Vector3f* wi, float* visDist, Vector2f sample, const Fragment& fragment) const override;

private:
	Color    color;
	Vector3f direction;
	float    angleTan;
};



class AreaLight : public Light
{
public:
	AreaLight(const Matrix44f& _worldTM, Color _color)
		: worldTM(_worldTM)
		, invWorldTM(Inverse(_worldTM))
		, color(_color)
	{
		Vector4f xx = Mul(Vector4f(1.0f, 0.0f, 0.0f, 0.0f), worldTM);
		Vector4f yy = Mul(Vector4f(0.0f, 1.0f, 0.0f, 0.0f), worldTM);
		Vector4f zz = Mul(Vector4f(0.0f, 0.0f, 1.0f, 0.0f), worldTM);
		area = Length(xx) * Length(yy);
		dir = Normalize(Vector3f(zz.x, zz.y, zz.z));
	}
	virtual void Sample(Color* color, Vector3f* wi, float* visDist, Vector2f sample, const Fragment& fragment) const override;

private:
	Matrix44f worldTM;
	Matrix44f invWorldTM;
	Color     color;
	Vector3f  dir;
	float     area;
};



class RenderMeshObject : public RenderObject
{
public:
	RenderMeshObject()
	{
	}

	void LoadFromObj(const char* path, bool flip=false)
	{
		std::cout << "Loading Object . . . ";
		auto model = LoadObjModuleFromFile(path);
		BuildRenderMesh(&renderMesh, *model, flip);
		BuildKdTree(&kdTree, renderMesh);
		material = std::make_shared<MultiMaterial>();
		for (uint i = 0; i < model->GetNumMaterials(); ++i)
		{
			material->AddMaterial(std::make_shared<BasicMaterial>(
				model->GetMaterial(i).diffuseColor,
				model->GetMaterial(i).emissiveColor, 2.5f, 2.55f));
		}
		std::cout << "DONE" << std::endl;
	}

	std::shared_ptr<Material> GetDefaultMaterial() const { return material; }

	bool Raytrace(Fragment* outFragment, Rayf ray) const override
	{
		SurfaceInteraction isect;
		if (!kdTree.Intersect(ray, &isect))
			return false;
		outFragment->point = isect.point;
		/*
		const Vertex v0 = renderMesh.GetVertices()[renderMesh.GetIndices()[isect.primId * 3 + 0]];
		const Vertex v1 = renderMesh.GetVertices()[renderMesh.GetIndices()[isect.primId * 3 + 1]];
		const Vertex v2 = renderMesh.GetVertices()[renderMesh.GetIndices()[isect.primId * 3 + 2]];
		outFragment->normal =
			v1.normal * isect.baryCoord.x +
			v2.normal * isect.baryCoord.y +
			v0.normal * (1 - (isect.baryCoord.x + isect.baryCoord.y));
		*/
		outFragment->normal = isect.normal;
		outFragment->distance = isect.dist;
		outFragment->materialId = renderMesh.GetMaterialIds()[isect.primId];
		return true;
	}

	bool Raytrace(Rayf ray) const override
	{
		return kdTree.IntersectP(ray);
	}

	virtual void GenerateSurfels(const Matrix44f& worldTM, float density, float radius, const Material& material, Sampler& sampler, std::deque<Surfel>& outSurfels) const override
	{
	}

private:
	RenderMesh  renderMesh;
	KdTreeAccel kdTree;
	std::shared_ptr<MultiMaterial> material;
};



class SphereRenderObject : public RenderObject
{
public:
	bool Raytrace(Fragment* outFragment, Rayf ray) const override
	{
		Vector3f center = Vector3f(zero);
		float radius2 = 1.0f;

		Vector3f L = center - ray.origin;
		float tca = Dot(L, ray.direction);
		float d2 = Dot(L, L) - tca * tca;
		if (d2 > radius2)
			return false;
		float thc = sqrt(radius2 - d2);
		float t0 = std::min(tca - thc, tca + thc);
		if (t0 < 0.0f)
			return false;

		outFragment->distance = t0;
		outFragment->point = ray.origin + ray.direction * t0;
		outFragment->normal = Normalize(outFragment->point);

		return true;
	}

	bool Raytrace(Rayf ray) const override
	{
		Vector3f center = Vector3f(zero);
		float radius2 = 1.0f;

		Vector3f L = center - ray.origin;
		float tca = Dot(L, ray.direction);
		float d2 = Dot(L, L) - tca * tca;
		if (d2 > radius2)
			return false;
		float thc = sqrt(radius2 - d2);
		float t0 = std::max(tca - thc, tca + thc);
		if (t0 < 0.0f || t0 > ray.maximum)
			return false;

		return true;
	}

	virtual void GenerateSurfels(const Matrix44f& worldTM, float density, float radius, const Material& material, Sampler& sampler, std::deque<Surfel>& outSurfels) const override
	{
	}

private:
};




class RenderNode
{
public:
	RenderNode() = default;

	RenderNode(std::shared_ptr<RenderObject> _renderObject, std::shared_ptr<Material> _material, Matrix44f _worldTM)
		: renderObject(_renderObject)
		, material(_material)
		, worldTM(_worldTM)
	{
		invWorldTM = Inverse(worldTM);
	}

	inline bool Raytrace(Fragment* outFragment, Rayf ray) const
	{
		if (!renderObject->Raytrace(outFragment, TransformRay(ray)))
			return false;
		Vector4f point = Mul(Vector4f(outFragment->point.x, outFragment->point.y, outFragment->point.z, 1.0f), worldTM);
		Vector4f normal = Mul(Vector4f(outFragment->normal.x, outFragment->normal.y, outFragment->normal.z, 0.0f), worldTM);
		outFragment->point = Vector3f(point.x, point.y, point.z);
		outFragment->normal = Normalize(Vector3f(normal.x, normal.y, normal.z));
		outFragment->distance = Distance(outFragment->point, ray.origin);
		return true;
	}

	inline bool Raytrace(Rayf ray) const
	{
		return renderObject->Raytrace(TransformRay(ray));
	}

	void GenerateSurfels(float density, float radius, Sampler& sampler, std::deque<Surfel>& outSurfels) const
	{
		renderObject->GenerateSurfels(worldTM, density, radius, *material, sampler, outSurfels);
	}

	std::shared_ptr<Material> GetMaterial() const { return material; }

private:
	inline Rayf TransformRay(Rayf ray) const
	{
		// TODO : ray maximum calculation is hacky
		Vector4f orig = Mul(Vector4f(ray.origin.x, ray.origin.y, ray.origin.z, 1.0f), invWorldTM);
		Vector4f dir = Mul(Vector4f(ray.direction.x, ray.direction.y, ray.direction.z, 0.0f), invWorldTM);
		Vector3f maxPt = ray.origin + ray.direction * ray.maximum;
		Vector4f maxPtT = Mul(Vector4f(maxPt.x, maxPt.y, maxPt.z, 1.0f), invWorldTM);
		ray.origin = Vector3f(orig.x, orig.y, orig.z);
		ray.direction = Normalize(Vector3f(dir.x, dir.y, dir.z));
		ray.maximum = Min(Distance(ray.origin, Vector3f(maxPtT.x, maxPtT.y, maxPtT.z)), numeric_limits<float>::max());
		return ray;
	}

private:
	std::shared_ptr<RenderObject> renderObject;
	std::shared_ptr<Material> material;
	Matrix44f worldTM;
	Matrix44f invWorldTM;
};



class Renderer
{
public:
	Renderer();

	void LoadCornellBox(uint width, uint height);
	void LoadAreaLightTest(uint width, uint height);
	void LoadRoomScene(uint width, uint height);
	void LoadSponzaScene(uint width, uint height);
	void StartRendering();
	void StopAndWait();

	const FrameBuffer<Color>& GetFrameBuffer() const { return frameBuffer; }
	const FrameBuffer<Color>& GetAccumColorBuffer() const { return accumColorBuffer; }
	const FrameBuffer<uint>&  GetAccumBuffer() const { return accumBuffer; }
	const std::vector<Recti>& GetThreadBuckets() const { return threadBuckets; }

	void SetMaxSamples(uint max) { maxSamples = max; }
	void SetMaxBounces(uint max) { maxDepth = max; }
	uint ProgressPercentage() const { return progress / (totalProgress / 100); }
	
	bool RaytraceScene(Rayf ray) const;
	bool RaytraceScene(Fragment* outFragment, Rayf ray) const;

private:
	void  RenderThread(uint threadId);
	Color PathTraceScene(Rayf ray, Sampler& sampler, int level) const;
	Color SampleSceneLights(Sampler& sampler, Vector3f wo, const NormalBasis& basis, float fresnel, const Fragment& fragment) const;
	void  CreatePhotonMap();
	void  CreateRadiosityMap();

private:
	uint width = 0;
	uint height = 0;
	float exposure = 1.0f;
	uint maxDepth = 16;
	uint maxSamples = 1024;

	PhysicalCamera camera;
	std::shared_ptr<Background> background;
	std::vector<RenderNode>     nodes;
	std::vector<std::shared_ptr<Light>> lights;
	PhotonMap    photonMap;
	RadiosityMap radiosityMap;

	FrameBuffer<Color> frameBuffer;
	FrameBuffer<Color> accumColorBuffer;
	FrameBuffer<uint>  accumBuffer;

	vector<std::thread> threads;
	vector<Recti> threadBuckets;
	atomic<uint> nextScanline;
	atomic<bool> rendering = false;
	atomic_uint progress = 0;
	uint        totalProgress = 0;

	bool usePhotonMapping = false;
};



Renderer::Renderer()
{
	background = std::make_shared<SkyLight>();
}



struct AffineTM
{
	Vector3f    translate{ 0.0f, 0.0f, 0.0f };
	Quaternionf rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
	Vector3f    scale{ 1.0f, 1.0f, 1.0f };
};

Matrix44f ToMatrix44(const AffineTM& tm)
{
	return Mul(Mul(NonUniformScale(tm.scale), ToMatrix44(tm.rotation)), Translate(tm.translate));
}



void Renderer::LoadCornellBox(uint _width, uint _height)
{
	width = _width;
	height = _height;
	const float aspectRatio = width / float(height);

	std::shared_ptr<BasicMaterial> whiteMaterial = std::make_shared<BasicMaterial>(Color(1.0f, 1.0f, 1.0f), Color(zero));
	// std::shared_ptr<BasicMaterial> greyMaterial = std::make_shared<BasicMaterial>(Color(0.25f, 0.25f, 0.25f), Color(zero));
	std::shared_ptr<MeasuredMaterial> greyMaterial = std::make_shared<MeasuredMaterial>("data/rgl/ilm_l3_37_metallic_rgb.bsdf");
	// shared_ptr<BasicMaterial> blueMaterial = std::make_shared<BasicMaterial>(Color(0.12f, 0.12f, 0.85f), Color(zero), 3.5f, 1.55f);
	// shared_ptr<MeasuredMaterial> blueMaterial = std::make_shared<MeasuredMaterial>("data/rgl/ilm_l3_37_metallic_rgb.bsdf");
	// shared_ptr<MeasuredMaterial> blueMaterial = make_shared<MeasuredMaterial>("data/rgl/acrylic_felt_white_rgb.bsdf");
	shared_ptr<MeasuredMaterial> blueMaterial = make_shared<MeasuredMaterial>("data/rgl/colodur_azure_4e_rgb.bsdf");
	// shared_ptr<MeasuredMaterial> blueMaterial = make_shared<MeasuredMaterial>("data/rgl/colodur_napoli_4f_rgb.bsdf");
	std::shared_ptr<BasicMaterial> glassMaterial = std::make_shared<BasicMaterial>(Color(zero), Color(zero), 100.0f, 1.5f, 1.0f);
	std::shared_ptr<BasicMaterial> selfEmittingMaterial = std::make_shared<BasicMaterial>(Color(zero), Color(1.0f, 1.0f, 1.0f) * 10.0f);

	std::shared_ptr<RenderMeshObject> model = std::make_shared<RenderMeshObject>();
	model->LoadFromObj("data/primitives/cornellbox.obj");
	nodes.emplace_back(model, model->GetDefaultMaterial(), Matrix44f(zero));

	std::shared_ptr<RenderMeshObject> box = std::make_shared<RenderMeshObject>();
//	box->LoadFromObj("data/primitives/box.obj", true);
//	nodes.emplace_back(box, greyMaterial, Mul(Scale(0.5f), Translate(Vector3f(45.0f, 0.0f, 30.0f))));
	box->LoadFromObj("data/primitives/teapot.obj", false);
	nodes.emplace_back(box, greyMaterial, Mul(Scale(2.5f), Translate(Vector3f(25.0f, -25.0f, 0.0f))));

	std::shared_ptr<SphereRenderObject> sphere = std::make_shared<SphereRenderObject>();
	nodes.emplace_back(sphere, blueMaterial, Mul(Scale(30.0f), Translate(Vector3f(-30.0f, 40.0f, 30.0f))));
//	nodes.emplace_back(sphere, glassMaterial, Mul(Scale(30.0f), Translate(Vector3f(-30.0f, 0.0f, 45.0f))));
//	nodes.emplace_back(sphere, glassMaterial, Mul(NonUniformScale(Vector3f(30.0f, 30.0f, 7.5f)), Translate(Vector3f(-30.0f, 0.0f, 45.0f))));

	{
		AffineTM tm;
		tm.translate = Vector3f(0.0f, 0.0f, 179.0f);
		tm.rotation = QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), -PI * 1.0f);
		tm.scale = Vector3f(20.0f, 20.0f, 1.0f);
		lights.emplace_back(std::make_shared<AreaLight>(ToMatrix44(tm), Color(1.0f, 1.0f, 1.0f) * 5500.0f));
	}
	
	Quaternionf camRotation = QuaternionAxisAngle(Vector3f(0.0f, 1.0f, 0.0f), PI * 1.0f);
	camera = PhysicalCamera(Vector3f(0.0f, -150.0f, 80.0f), camRotation, aspectRatio, 150.0f, 0.0f);
}



void Renderer::LoadAreaLightTest(uint _width, uint _height)
{
	width = _width;
	height = _height;
	const float aspectRatio = width / float(height);

	maxDepth = 1;
	const Color lightColor = Color(1.0f, 1.0f, 1.0f) * 150.0f;
	// shared_ptr<BasicMaterial> floorMaterial = make_shared<BasicMaterial>(Color(1.0f, 1.0f, 1.0f), Color(zero), 25.0f, 1.55f);
	// shared_ptr<BasicMaterial> floorMaterial = make_shared<BasicMaterial>(Color(1.0f, 1.0f, 1.0f), Color(zero), 5.0f, 1.55f);
	// shared_ptr<BasicMaterial> floorMaterial = make_shared<BasicMaterial>(Color(0.55f, 0.55f, 0.55f), Color(zero), 10.0f, 1.75f);
	// shared_ptr<MeasuredMaterial> floorMaterial = make_shared<MeasuredMaterial>("data/rgl/acrylic_felt_white_rgb.bsdf");
	// shared_ptr<MeasuredMaterial> floorMaterial = make_shared<MeasuredMaterial>("data/rgl/colodur_azure_4e_rgb.bsdf");
	// shared_ptr<MeasuredMaterial> floorMaterial = make_shared<MeasuredMaterial>("data/rgl/colodur_napoli_4f_rgb.bsdf");
	shared_ptr<MeasuredMaterial> floorMaterial = make_shared<MeasuredMaterial>("data/rgl/ilm_l3_37_metallic_rgb.bsdf");
	shared_ptr<BasicMaterial> selfEmittingMaterial = make_shared<BasicMaterial>(Color(zero), lightColor);

	shared_ptr<RenderMeshObject> plane = make_shared<RenderMeshObject>();
	plane->LoadFromObj("data/primitives/plane.obj", true);

	{
		AffineTM tm;
		tm.scale = Vector3f(1000.0f, 1000.0f, 1.0f);
		nodes.emplace_back(plane, floorMaterial, ToMatrix44(tm));
	}
	{
		AffineTM tm;
		tm.translate = Vector3f(-100.0f, 0.0f, 40.0f);
		tm.rotation = QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), -PI * 0.5f);
		tm.scale = Vector3f(1.0f, 4.0f, 4.0f);
		nodes.emplace_back(plane, selfEmittingMaterial, ToMatrix44(tm));
	}
	{
		AffineTM tm;
		tm.translate = Vector3f(100.0f, 0.0f, 40.0f);
		tm.rotation = QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), -PI * 0.5f);
		tm.scale = Vector3f(20.0f, 80.0f, 1.0f);
		lights.emplace_back(make_shared<AreaLight>(ToMatrix44(tm), lightColor));
	}

	Quaternionf camRotation =
		QuaternionAxisAngle(Vector3f(0.0f, 1.0f, 0.0f), PI * 1.0f) *
		QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), PI * 0.15f);
	camera = PhysicalCamera(Vector3f(0.0f, -250.0f, 140.0f), camRotation, aspectRatio, 150.0f, 0.0f);
}



void Renderer::LoadRoomScene(uint _width, uint _height)
{
	width = _width;
	height = _height;
	const float aspectRatio = width / float(height);

	std::shared_ptr<RenderMeshObject> model = std::make_shared<RenderMeshObject>();
	model->LoadFromObj("data/primitives/room.obj");

	shared_ptr<MeasuredMaterial> acrylicWhite = make_shared<MeasuredMaterial>("data/rgl/acrylic_felt_white_rgb.bsdf");
	shared_ptr<MeasuredMaterial> colodurAzure = make_shared<MeasuredMaterial>("data/rgl/colodur_azure_4e_rgb.bsdf");
	shared_ptr<MeasuredMaterial> colodurNapoli = make_shared<MeasuredMaterial>("data/rgl/colodur_napoli_4f_rgb.bsdf");
	shared_ptr<MeasuredMaterial> ilmMetalic = make_shared<MeasuredMaterial>("data/rgl/ilm_l3_37_metallic_rgb.bsdf");
	auto material = make_shared<MultiMaterial>();
	material->AddMaterial(colodurNapoli);
	material->AddMaterial(acrylicWhite);
	material->AddMaterial(acrylicWhite);

//	nodes.emplace_back(model, model->GetDefaultMaterial(), Matrix44f(zero));
	nodes.emplace_back(model, material, Matrix44f(zero));

	const float sunIntensity = 25.0f;
	lights.emplace_back(std::make_shared<DirectLight>(
		Color(1.0f, 0.75f, 0.35f) * sunIntensity,
		Vector3f(-2.5f, 3.0f, -4.0f),
		0.02f));

	const float skyIntensity = 15.0f;
	background = std::make_shared<SkyLight>(Color(
		skyIntensity * 0.35f,
		skyIntensity * 0.75f,
		skyIntensity * 1.0f));

	Quaternionf camRotation =
		QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), PI * 0.5f) *
		QuaternionAxisAngle(Vector3f(0.0f, 0.0f, 1.0f), PI * 0.75f);
	camera = PhysicalCamera(Vector3f(3.0f, 1.5f, 2.0f), camRotation, aspectRatio, 150.0f, 0.0f);
}



void Renderer::LoadSponzaScene(uint _width, uint _height)
{
	width = _width;
	height = _height;
//	usePhotonMapping = true;
	const float aspectRatio = width / float(height);

	std::shared_ptr<BasicMaterial> whiteMaterial = std::make_shared<BasicMaterial>(Color(0.65f, 0.65f, 0.65f), Color(zero));

	std::shared_ptr<RenderMeshObject> model = std::make_shared<RenderMeshObject>();
	model->LoadFromObj("data/sponza/sponza.obj");
	nodes.emplace_back(model, whiteMaterial, Matrix44f(zero));

	/*/
	Quaternionf camRotation =
		QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), PI * 0.5f) *
		QuaternionAxisAngle(Vector3f(0.0f, 0.0f, 1.0f), PI * 0.5f) *
		QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), -PI * 0.15f);
	camera = PhysicalCamera(Vector3f(600.0f, 150.0f, 0.0f), camRotation, aspectRatio, 300.0f, 2.0f);
	/**/
	/*/
	Quaternionf camRotation =
		QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), PI * 0.5f) *
		QuaternionAxisAngle(Vector3f(0.0f, 0.0f, 1.0f), PI * 0.5f) *
		QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), PI * 0.15f);
	camera = PhysicalCamera(Vector3f(600.0f, 850.0f, 0.0f), camRotation, aspectRatio, 500.0f, 0.0f);
	/**/
	/**/
	Quaternionf camRotation =
		QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), PI * 0.5f) *
		QuaternionAxisAngle(Vector3f(0.0f, 0.0f, 1.0f), PI * 0.85f);
	camera = PhysicalCamera(Vector3f(600.0f, 650.0f, 400.0f), camRotation, aspectRatio, 500.0f, 0.0f);
	/**/
	/*/
	Quaternionf camRotation =
		QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), PI * 0.5f) *
		QuaternionAxisAngle(Vector3f(0.0f, 0.0f, 1.0f), PI * 0.5f) *
		QuaternionAxisAngle(Vector3f(1.0f, 0.0f, 0.0f), PI * 0.15f);
	camera = PhysicalCamera(Vector3f(6000.0f, 8500.0f, 2500.0f) * 0.7f, camRotation, aspectRatio, 500.0f, 0.0f);
	/*/

	const float skyIntensity = 5.0f;
	background = std::make_shared<SkyLight>(Color(
		skyIntensity * 0.35f,
		skyIntensity * 0.75f,
		skyIntensity * 1.0f));
	/*/
	lights.emplace_back(std::make_shared<DirectLight>(
		Color(1.0f, 0.75f, 0.35f) * 15.5f,
		Vector3f(-3.0f, 7.0f, 2.0f),
		0.05f));
	/**/
	/**/
	lights.emplace_back(std::make_shared<DirectLight>(
		// Color(1.0f, 0.75f, 0.35f) * 15.5f,
		Color(1.0f, 0.75f, 0.35f) * 7.5f,
		Vector3f(-1.5f, 7.0f, 1.0f),
		0.05f));
	/**/
}



void Renderer::StartRendering()
{
	assert(rendering == false);
	assert(threads.empty());

	std::cout << "Start rendering..." << std::endl;

	frameBuffer = FrameBuffer<Color>{ width, height, Color{zero} };
	accumColorBuffer = FrameBuffer<Color>{ width, height, Color{ zero } };
	accumBuffer = FrameBuffer<uint>{ width, height, 0 };

	if (usePhotonMapping)
	{
		std::cout << "Creating Photon Map..." << std::endl;
		CreatePhotonMap();
	}

	std::cout << "Creating Radiosity Map..." << std::endl;
	// CreateRadiosityMap();

	std::cout << "Rendering..." << std::endl;
	nextScanline = 0;
	rendering = true;
	// const uint numCpus = 1;
	const uint numCpus = std::max(1u, std::thread::hardware_concurrency() * 2 / 3);
	// const uint numCpus = std::thread::hardware_concurrency();
	threads.reserve(numCpus);
	threadBuckets.resize(numCpus);
	for (uint i = 0; i < numCpus; ++i)
	{
		std::thread thread{ [this, i] { this->RenderThread(i); } };
		SetThreadPriority(thread.native_handle(), THREAD_MODE_BACKGROUND_BEGIN);
		threads.push_back(std::move(thread));
	}
}



void Renderer::StopAndWait()
{
	assert(rendering == true);
	assert(!threads.empty());
	rendering = false;
	for (auto& thread : threads)
		thread.join();
	threads.clear();
	threadBuckets.clear();
}



void Renderer::RenderThread(uint threadId)
{
	const float invWidth = 1.0f / width;
	const float invHeigth = 1.0f / height;
	totalProgress = maxSamples * width * height;

	Recti bounds;
	bounds.max.x = width;
	bounds.max.y = height;
	Sampler sampler = Sampler{ bounds, maxSamples };

	auto Exposure = [](float c, float exposure) { return 1.0f - std::exp(-c * exposure); };

	while (rendering)
	{
		const uint idx = progress++;
		if (idx >= totalProgress)
			break;
		const uint smpIdx = idx / (width * height);
		const uint x = idx % width;
		const uint y = (idx / width) % height;

		sampler.SetPixel(Vector2i(x, y));
		sampler.SetSample(smpIdx);
		
		Vector2i localPx = Vector2i(x, y);
		Color color = accumColorBuffer.GetPixel(localPx);

		Vector2f sample = sampler.GetSample2D();
		Vector2f coord = Vector2f(x * invWidth * 2.0f - 1.0f, y * invHeigth * 2.0f - 1.0f);
		coord.x += sample.x * invWidth * 2.0f;
		coord.y += sample.y * invHeigth * 2.0f;
		Rayf ray = camera.Sample(coord, sampler);
		Color pathColor = PathTraceScene(ray, sampler, 0);
		pathColor.r = Exposure(pathColor.r, exposure);
		pathColor.g = Exposure(pathColor.g, exposure);
		pathColor.b = Exposure(pathColor.b, exposure);
		color = color + pathColor;

		uint count = accumBuffer.GetPixel(localPx) + 1;
		accumBuffer.SetPixel(localPx, count);
		frameBuffer.SetPixel(localPx, color * (1.0f / count));
		accumColorBuffer.SetPixel(localPx, color);
	}
}



Color Renderer::PathTraceScene(Rayf ray, Sampler& sampler, int level) const
{
	const float epsilon = 1.0f / (1024.0f * 1024.0f);

	Fragment fragment;
	if (!RaytraceScene(&fragment, ray))
		return background->Sample(ray.direction);

	Color color{ zero };
	fragment.material->Sample(&fragment);

	const NormalBasis basis = NormalBasisFromNormal(fragment.normal);
	const float fresnel = Max(0.0f, Schlick(Dot(fragment.normal, -ray.direction), 1.0f, fragment.ior));
	const Vector3f wo = InvTransform(basis, -ray.direction);
	
	if (level < maxDepth)
	{
		Vector3f wi;

		float pdf;
		Color _color = fragment.material->Sample(
			fragment, sampler.GetSample2D(),
			wo, &wi, &pdf);

		Rayf newRay{ zero };
		newRay.origin = fragment.point + fragment.normal * epsilon;
		newRay.direction = Transform(basis, wi);
		const Color lightColor = PathTraceScene(newRay, sampler, level + 1);

	//	color += lightColor * (_color * Max(0.0f, wo.z) / pdf);
		color += lightColor * _color * Max(0.0f, wo.z);
	}
	
	color = color + SampleSceneLights(sampler, wo, basis, fresnel, fragment);
	color = color + fragment.emissive * Max(0.0f, Dot(fragment.normal, wo)) * (1.0f / PI);

	color.r = Max(0.0f, color.r);
	color.g = Max(0.0f, color.g);
	color.b = Max(0.0f, color.b);

	return color;
}



Color Renderer::SampleSceneLights(Sampler& sampler, Vector3f wo, const NormalBasis& basis, float fresnel, const Fragment& fragment) const
{
	const float epsilon = 1.0f / 1024.0f;

	const Vector3f wn = fragment.normal;
	const Vector2f sample = sampler.GetSample2D();

	Color color{ zero };
	for (const auto light : lights)
	{
		Color lightColor;
		Vector3f wi;
		float visDist;
		light->Sample(&lightColor, &wi, &visDist, sample, fragment);

		if (lightColor.b <= epsilon && lightColor.g < epsilon && lightColor.b < epsilon)
			continue;

		Rayf ray;
		ray.origin = fragment.point + wn * epsilon;
		ray.direction = wi;
		ray.maximum = Max(0.0f, visDist - epsilon);
		if (RaytraceScene(ray))
			continue;

		wi = InvTransform(basis, wi);

		const Color evalColor = fragment.material->Evaluate(
			fragment,
			wi,
			wo);
		const float pdf = fragment.material->PDF(
			fragment,
			wi,
			wo);
		color = color + evalColor * Max(0.0f, wo.z) * lightColor;
	}
	return color;
}



bool Renderer::RaytraceScene(Rayf ray) const
{
	for (const auto& node : nodes)
	{
		if (node.Raytrace(ray))
			return true;
	}
	return false;
}



bool Renderer::RaytraceScene(Fragment* outFragment, Rayf ray) const
{
	bool result = false;
	for (const auto& node : nodes)
	{
		Fragment fragment;
		if (node.Raytrace(&fragment, ray) && fragment.distance < outFragment->distance)
		{
			*outFragment = fragment;
			outFragment->material = node.GetMaterial().get();
			result = true;
		}
	}
	return result;
}



void Renderer::CreatePhotonMap()
{
}

void Renderer::CreateRadiosityMap()
{
}



//////////////////////////////////////////////////////////////



void DirectLight::Sample(Color* outColor, Vector3f* wi, float* visDist, Vector2f sample, const Fragment& fragment) const
{
	*wi = SampleCone(direction, sample, 0.05f);
	*outColor = color;
	*visDist = numeric_limits<float>::max();
}

void AreaLight::Sample(Color* outColor, Vector3f* wi, float* visDist, Vector2f sample, const Fragment& fragment) const
{
	const Vector3f opt = Vector3f(sample.x - 0.5f, sample.y - 0.5f, 0.0f);
	const Vector4f pt4 = Mul(Vector4f(opt.x, opt.y, opt.z, 1.0f), worldTM);
	const Vector3f pt = Vector3f(pt4.x, pt4.y, pt4.z);

	*wi = Normalize(pt - fragment.point);
	*visDist = Distance(pt, fragment.point);

	const float d2 = Sqr(*visDist);
	const float attn = 1.0f / (4.0f * PI * d2);
	*outColor = color * (Max(0.0f, Dot(dir, -*wi)) / PI) * area * attn;
}


//////////////////////////////////////////////////////////////


class RenderCanvasListener : public CanvasListener
{
public:
	virtual void Close()
	{
		PostQuitMessage();
	}
	virtual void KeyDown(KeyCode keyCode)
	{
		if (keyCode == KeyCode::Escape)
			PostQuitMessage();
	}
};



}



//////////////////////////////////////////////////////////////


void ActualPathTracer()
{
	const uint width = 1280;
	const uint height = 720;

	//////////////////////////////////////////////////////////////

	auto Format = [](chrono::duration<double> duration)
	{
		uint c = uint(duration.count());
		ostringstream oss;
		oss << setfill('0')
			<< setw(2)
			<< c / 3600
			<< "h:"
			<< setw(2)
			<< (c % 3600) / 60
			<< "m:"
			<< setw(2)
			<< c % 60 << "s";
		return oss.str();
	};

	auto startLoad = chrono::system_clock::now();
	Renderer renderer;
//	renderer.SetMaxSamples(32);
//	renderer.SetMaxBounces(2);
	// renderer.LoadCornellBox(width, height);
	// renderer.LoadAreaLightTest(width, height);
	renderer.LoadRoomScene(width, height);
	// renderer.LoadSponzaScene(width, height);
	auto startRender = chrono::system_clock::now();
	renderer.StartRendering();

	cout << "Scene loaded in: " << Format(startRender - startLoad) << endl;

	//////////////////////////////////////////////////////////////

	RenderDeviceDesc renderDeviceDesc{};
	renderDeviceDesc.enableValidation = true;
	RenderDevicePtr renderDevice = RenderDevice::Create(renderDeviceDesc);
	if (!renderDevice)
		return;
	CanvasDesc canvasDesc{};
	canvasDesc.width = width;
	canvasDesc.height = height;
	CanvasPtr canvas = Canvas::Create(canvasDesc);
	if (!canvas)
		return;
	RenderCanvasListener canvasListener;
	canvas->AddListener(&canvasListener);
	SwapChainDesc swapChainDesc{};
	swapChainDesc.width = canvasDesc.width;
	swapChainDesc.height = canvasDesc.height;
	swapChainDesc.windowHandle = canvas->GetHandle();
	SwapChainPtr swapChain = renderDevice->CreateSwapChain(swapChainDesc);
	if (!swapChain)
		return;

	GPUBufferDesc stangingBufferDesc;
	stangingBufferDesc.type = GPUBufferType::Texture2D;
	stangingBufferDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	stangingBufferDesc.usageFlags = GPUUsage_Dynamic | GPUUsage_ShaderResource;
	stangingBufferDesc.width = width;
	stangingBufferDesc.height = height;
	stangingBufferDesc.arraySize = 1;
	GPUBuffer stangingBuffer = GPUBuffer::Create(renderDevice, stangingBufferDesc);

	uint lastProgressPercent = 0;

	while (UpdateSystemMessages() == SystemMessageResult::Continue)
	{
		D3D11_MAPPED_SUBRESOURCE mapped;
		renderDevice->GetContext()->Map(stangingBuffer.buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		CopyFrameBufferWithExposureSRGB((uint32*)mapped.pData, renderer.GetFrameBuffer(), 1.0f);
	//	DrawBukets((uint32*)mapped.pData, renderer.GetThreadBuckets(), width, height);
		renderDevice->GetContext()->Unmap(stangingBuffer.buffer, 0);
		renderDevice->GetContext()->CopyResource(swapChain->GetBackBuffer().buffer, stangingBuffer.buffer);
		swapChain->Present();
		::Sleep(200);

		uint progressPercent = renderer.ProgressPercentage();
		if (progressPercent != lastProgressPercent)
		{
			auto curTime = chrono::system_clock::now();
			auto elapsed = chrono::duration<double>(curTime - startRender);

			double fraction = progressPercent / 100.0;
			chrono::duration<double> remaining = (1.0 - fraction) * elapsed / fraction;

			cout 
				<< "Progress: " << setw(3) << progressPercent
				<< "% - elapsed: " << Format(elapsed)
				<< " - remaining: " << Format(remaining)
				<< endl;
			lastProgressPercent = progressPercent;
		}
	}

	renderer.StopAndWait();
}



//////////////////////////////////////////////////////////



void TestTriangleDist()
{
	/*
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera2D(protoGL.GetCanvas(), CameraOrtho2D::Upward);
	camera2D.SetCamera(Vector2f(zero), 3.0f);

	Sampler sampler{ 0, 0 };

	while (protoGL.Update())
	{
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera2D.CommitToGL();

		glLineWidth(1.0f);
		glColor4ub(128, 128, 128, 255);
		glBegin(GL_LINE_LOOP);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(1.0f, 0.0f);
		glVertex2f(1.0f, 1.0f);
		glVertex2f(0.0f, 1.0f);
		glEnd();

		glPointSize(2.0f);
		glColor4ub(0, 0, 0, 255);
		glBegin(GL_POINTS);
		for (uint i = 0; i < 1024 * 4; ++i)
		{
			Vector2f sample = sampler.GetSample();
		//	if (sample.y > (1 - sample.x))
		//	{
		//		sample.x = 1 - sample.x;
		//		sample.y = 1 - sample.y;
		//	}
			sample.x = PowerHeuristic(1.15f, sample.x);
			glVertex2f(sample.x, sample.y);
		}
		glEnd();

		protoGL.Swap();
		glEnd();
	}
	*/
}



void TestModel()
{
	/*
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera2D(protoGL.GetCanvas(), CameraOrtho2D::Upward);
	camera2D.SetCamera(Vector2f(zero), 3.0f);

	Sampler sampler{ 0, 0 };

	while (protoGL.Update())
	{
		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera2D.CommitToGL();

		glLineWidth(1.0f);
		glColor4ub(128, 128, 128, 255);
		glBegin(GL_LINE_LOOP);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(1.0f, 0.0f);
		glVertex2f(1.0f, 1.0f);
		glVertex2f(0.0f, 1.0f);
		glEnd();

		glPointSize(2.0f);
		glColor4ub(0, 0, 0, 255);
		glBegin(GL_POINTS);
		for (uint i = 0; i < 1024 * 4; ++i)
		{
			Vector2f sample = sampler.GetSample();
			if (sample.y > (1 - sample.x))
			{
				sample.x = 1 - sample.x;
				sample.y = 1 - sample.y;
			}
			glVertex2f(sample.x, sample.y);
		}
		glEnd();

		protoGL.Swap();
		glEnd();
	}
	*/
}



void TestHemisphere()
{
	/*
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	OrbitalCamera camera3D(protoGL.GetCanvas());
	camera3D.SetPerspective(1.4f, 0.01f, 1024.0f);
	camera3D.SetCamera(Vector3f(zero), Quaternionf{zero}, 2.5f);

	TimeCounter timer;

	Sampler sampler{ 0, 0 };

	while (protoGL.Update())
	{
		Time deltaTime = timer.GetElapsed();

		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera3D.Update(deltaTime);
		camera3D.CommitToGL();

		glLineWidth(1.0f);
		glColor4ub(128, 128, 128, 255);
		glBegin(GL_LINE_LOOP);
		for (float t = 0; t < 1.0f; t += 1.0f / 64.0f)
		{
			glVertex3f(cos(t * PI * 2), sin(t * PI * 2), 0.0f);
		}
		glEnd();

		glPointSize(2.0f);
		glColor4ub(0, 0, 0, 255);
		glBegin(GL_POINTS);
		for (uint i = 0; i < 1024 * 4; ++i)
		{
			Vector3f sample = SamplePowerHemisphere(sampler.GetSample(), 1.0f);
			glVertex3f(sample.x, sample.y, sample.z);
		}
		glEnd();

		protoGL.Swap();
		glEnd();
	}
	*/
}



void TestBeckmannDist()
{
	ProtoGL protoGL;
	protoGL.Initialize(ProtoGLDesc(1280, 720));
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_BLEND);

	CameraOrtho2D camera2D(protoGL.GetCanvas(), CameraOrtho2D::Upward);
	camera2D.SetCamera(Vector2f(zero), 3.0f);

	vidf::Rand48 rand48;
	vidf::UniformReal<float> unorm{ 0.0f, 1.0f };
	
	// const uint numSamples = 256;
	const uint numSamples = 2048;
	Recti bounds;
	bounds.max.x = 32;
	bounds.max.y = 32;
	HaltonSampler sampler(numSamples, bounds);
	sampler.StartPixel(Vector2i(0, 0));
	vector<float> histogram;
	histogram.resize(numSamples);
	uint histogramSz = 0;
	uint sampleIdx = 0;

	const float alpha = 0.05f;
	const float ior = 1.55f;
	Vector2f prevcp = Vector2f(zero);

//	shared_ptr<MeasuredMaterial> material = make_shared<MeasuredMaterial>("data/rgl/acrylic_felt_white_rgb.bsdf");
	shared_ptr<MeasuredMaterial> material = make_shared<MeasuredMaterial>("data/rgl/ilm_l3_37_metallic_rgb.bsdf");

	while (protoGL.Update())
	{
		const Vector2f cp = camera2D.CursorPosition();
		const Vector3f wo = Normalize(Vector3f(cp.x, 0.0f, cp.y) - Vector3f(zero));
		const Vector3f wn = Vector3f(0, 0, 1);
		if (cp != prevcp)
		{
			fill(histogram.begin(), histogram.end(), 0.0f);
			histogramSz = 0;
			sampleIdx = 0;
		}
		prevcp = cp;

		glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		camera2D.CommitToGL();

		glLineWidth(2.0f);
		glColor4ub(255, 128, 0, 255);
		glBegin(GL_LINES);
		glVertex2f(-40.0f, 0.0f);
		glVertex2f( 40.0f, 0.0f);
		glEnd();

		glColor4ub(0, 0, 0, 255);
		glBegin(GL_LINES);
		glVertex2f(0.0f, 0.0f);
		glVertex2f(wo.x, wo.z);
		glEnd();

		glLineWidth(1.0f);
		glColor4ub(0, 0, 0, 64);
	//	glBegin(GL_LINES);
		for (uint smp = 0; smp < numSamples; ++smp)
		{
			sampler.SetSample(sampleIdx++);

#if 0
			const float fresnel = Max(0.0f, Schlick(Dot(wn, wo), 1.0f, ior));
						
			Vector3f wi;
			float strength;
			if (unorm(rand48) < 0.5f)
			{
				const Vector3f wh = SamplePowerHemisphere(sampler.Get2D(), alpha);
				wi = Reflect(wh, -wo);

				const float nh = Dot(wn, wh);
				const float ni = Dot(wn, wi);
				const float no = Dot(wn, wo);
				const float alpha2 = alpha * alpha;
				const float D = alpha2 / (PI * Sqr(Sqr(nh) * (alpha2 - 1) + 1));

				strength =
					D /
					(4 * abs(Dot(wi, wh)) * max(abs(Dot(wn, wi)), abs(Dot(wn, wo)))) *
					Schlick(Dot(wi, wh), 1.0f, ior);
			}
			else
			{
				auto pow5 = [](float v) { return (v * v) * (v * v) * v; };
				wi = SampleCosineHemisphere(sampler.Get2D());

				strength =
					(28.f / (23.f * PI)) *
					(1 - pow5(1 - 0.5f * abs(Dot(wn, wi)))) *
					(1 - pow5(1 - 0.5f * abs(Dot(wn, wo))));
			}
#endif

			Vector3f wi;
			Fragment fragment;
			float pdf;
			const Color _color = material->Sample(fragment, sampler.Get2D(), wo, &wi, &pdf);
			// const float strength = (_color * Max(0.0f, wo.z) / pdf).r;
			const float strength = (_color * Max(0.0f, wo.z)).r;

			if (abs(wi.y) > 1.0f / 64.0f)
				continue;

			float a = atan2(wi.z, wi.x);
			if (a < 0.0f)
				a += 2 * PI;
			a = fmod(a, 2 * PI);
			const uint histogramIdx = uint(a / (2 * PI) * numSamples);
			histogram[histogramIdx] += strength;
			histogramSz++;

		//	glVertex2f(0.0f, 0.0f);
		//	glVertex2f(wi.x, wi.z);
		}
	//	glEnd();

		glColor4ub(96, 96, 96, 255);
		glBegin(GL_LINE_LOOP);
		for (uint smp = 0; smp < numSamples; ++smp)
		{
			const float t = (smp / float(numSamples)) * PI * 2.0f;
			const Vector3f wi = Vector3f(cos(t), 0.0f, sin(t));
			glVertex2f(wi.x / PI, wi.z / PI);
		}
		glEnd();
		glColor4ub(192, 192, 192, 255);
		glBegin(GL_LINE_LOOP);
		for (uint smp = 0; smp < numSamples; ++smp)
		{
			const float t = (smp / float(numSamples)) * PI * 2.0f;
			const Vector3f wi = Vector3f(cos(t), 0.0f, sin(t));
			glVertex2f(wi.x, wi.z);
		}
		glEnd();

		glColor4ub(0, 192, 32, 255);
		glBegin(GL_LINE_LOOP);
		for (uint smp = 0; smp < numSamples; ++smp)
		{
			const float t = (smp / float(numSamples)) * PI * 2.0f;
			const Vector3f wi = Vector3f(cos(t), 0.0f, sin(t));

			const float brdf = (histogram[smp] / float(histogramSz)) * (numSamples / (2 * PI));

			glVertex2f(wi.x * brdf, wi.z * brdf);
		}
		glEnd();

		glColor4ub(0, 64, 255, 255);
		glBegin(GL_LINE_LOOP);
		for (uint smp = 0; smp < numSamples; ++smp)
		{
			const float t = (smp / float(numSamples)) * PI * 2.0f;
			const Vector3f wi = Vector3f(cos(t), 0.0f, sin(t));
			const Vector3f wh = Normalize(wo + wi);

			Fragment fragment;
			const Color evalColor = material->Evaluate(fragment, wo, wi);
			const float pdf = material->PDF(fragment, wo, wi);

			float brdf = (evalColor * Max(0.0f, wo.z)).r;
			
#if 0
			float brdf = 0.0f;
			const float fresnel = Schlick(Max(0.0f, Dot(wn, wo)), 1.0f, ior);

			brdf += Max(0.0f, Dot(wi, wn) / PI) * (1.0f - fresnel);
			
			/*

		//	brdf += Max(0.0f, Dot(wi, wn) / PI) * (1.0f - fresnel);

			const Vector3f wh = Normalize(wo + wi);

		//	const float alphax = Sqr(1 / shininess);
		//	const float alphax2 = alphax * alphax;
		//	const float nh = Dot(wn, wh);
		//	const float nh2 = nh * nh;
		//	const float nh4 = nh2 * nh2;
		//	const float D = (1.0f / (PI * alphax2 * nh4)) * exp((nh2 - 1) / (alphax2 * nh2));
		//	const float alpha = Sqr(1 / shininess);

		//	const float alpha = 0.9f;
			const float m = 0.1f;
			const float m2 = m * m;
			const float nh = Max(0.0f, Dot(wn, wh));
			const float nh2 = nh * nh;
			const float nh4 = nh2 * nh2;
		//	const float D = exp((nh2 - 1.0f) / (m2 * nh2)) / (PI * m2 * nh4);
			const float D = (shininess + 2) / (2 * PI) * pow(nh, shininess);
			brdf += D * abs(Dot(wo, wh)) / abs(Dot(wn, wo));
			
			const float cosTheta = Dot(wn, wo);
			const float cos2Theta = cosTheta * cosTheta;
			const float sin2Theta = Max(0.0f, 1.0f - cos2Theta);
			const float sinTheta = sqrt(sin2Theta);
			const float tanTheta = sinTheta / cosTheta;
			const float absTanTheta = abs(tanTheta);
		//	const float a = 1.0f / (alphax * absTanTheta);
		//	float lambda = (1.0f - 1.259f * a + 0.396f * a * a) / (3.535f * a + 2.181f * a * a);
		//	if (a >= 1.6f)
		//		lambda = 0.0f;
		//	const float G1 = 1.0f / (1.0f + lambda);
			
		//	const float pdf = D * G1 * abs(Dot(wo, wh)) / abs(cosTheta);
			const float pdf = D * abs(Dot(wo, wh)) / abs(cosTheta);
		//	brdf += (pdf / (4.0f * Dot(wo, wh))) * fresnel;
			*/

			const float nh = Dot(wn, wh);
			const float ni = Dot(wn, wi);
			const float no = Dot(wn, wo);
			const float alpha2 = alpha * alpha;
		//	const float D = 1.0f / (PI * alpha2) * pow(nh, 2.0f / alpha2 - 2.0f);
			const float D = alpha2 / (PI * Sqr(Sqr(nh) * (alpha2 - 1) + 1));
			const float G = ni * no;
		//	brdf += D * G * fresnel / (4 * ni * no);
		//	brdf += D * G / (4 * ni * no) * fresnel / PI;
		//	brdf += D;
			const float pdf = D * G * abs(Dot(wo, wh)) / abs(no);
			brdf += (pdf / (4 * Dot(wo, wh))) * fresnel;
#endif

			glVertex2f(wi.x * brdf, wi.z * brdf);
		}
		glEnd();

		protoGL.Swap();
	}
}



//////////////////////////////////////////////////////////

void PathTracer()
{
	ActualPathTracer();
//	TestTriangleDist();
//	TestModel();
//	TestHemisphere();
//	TestBeckmannDist();
}
