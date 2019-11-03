#pragma once

namespace vipt
{


using namespace vidf;


class Sampler {
  public:
	// Sampler Interface
	virtual ~Sampler();
	Sampler(int64_t samplesPerPixel);
	virtual void StartPixel(const Vector2i &p);
	virtual float Get1D() = 0;
	virtual Vector2f Get2D() = 0;
	void Request1DArray(int n);
	void Request2DArray(int n);
	virtual int RoundCount(int n) const { return n; }
	const float *Get1DArray(int n);
	const Vector2f *Get2DArray(int n);
	virtual bool StartNextSample();
	virtual bool SetSampleNumber(int64_t sampleNum);
	int64_t CurrentSampleNumber() const { return currentPixelSampleIndex; }

	// Sampler Public Data
	const int64_t samplesPerPixel;

  protected:
	// Sampler Protected Data
	Vector2i currentPixel;
	int64_t currentPixelSampleIndex;
	std::vector<int> samples1DArraySizes, samples2DArraySizes;
	std::vector<std::vector<float>> sampleArray1D;
	std::vector<std::vector<Vector2f>> sampleArray2D;

  // private:
	// Sampler Private Data
	size_t array1DOffset, array2DOffset;
};



class GlobalSampler : public Sampler {
public:
	// GlobalSampler Public Methods
	bool StartNextSample();
	void StartPixel(const Vector2i &);
	bool SetSampleNumber(int64_t sampleNum);
	float Get1D();
	Vector2f Get2D();
	GlobalSampler(int64_t samplesPerPixel) : Sampler(samplesPerPixel) {}
	virtual int64_t GetIndexForSample(int64_t sampleNum) const = 0;
	virtual float SampleDimension(int64_t index, int dimension) const = 0;

// private:
	// GlobalSampler Private Data
	int dimension;
	int64_t intervalSampleIndex;
	static const int arrayStartDim = 5;
	int arrayEndDim;
};


// HaltonSampler Declarations
class HaltonSampler : public GlobalSampler
{
public:
	// HaltonSampler Public Methods
	HaltonSampler(int nsamp, const Recti &sampleBounds,
		bool sampleAtCenter = false);
	void SetSample(uint sample);
	int64_t GetIndexForSample(int64_t sampleNum) const;
	float SampleDimension(int64_t index, int dimension) const;

private:
	// HaltonSampler Private Data
	static std::vector<uint16> radicalInversePermutations;
	Vector2i baseScales, baseExponents;
	int sampleStride;
	int multInverse[2];
	mutable Vector2i pixelForOffset = Vector2i(std::numeric_limits<int>::max(),
		std::numeric_limits<int>::max());
	mutable int64_t offsetForCurrentPixel;
	// Added after book publication: force all image samples to be at the
	// center of the pixel area.
	bool sampleAtPixelCenter;

	// HaltonSampler Private Methods
	const uint16 *PermutationForDimension(int dim) const;
};


}