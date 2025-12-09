#pragma once
#include "Image.h"

class Histogram {
public:
	enum class Resolution : uint32_t {
		_8bit = 256,
		_10bit = 1024,
		_12bit = 4096,
		_14bit = 16383,
		_16bit = 65536
	};

private:
	std::unique_ptr<uint32_t[]> m_data;
	Resolution m_resolution = Resolution::_8bit;

public:
	Histogram() { m_data = std::make_unique<uint32_t[]>(uint32_t(m_resolution)); };

	Histogram(Resolution resolution) : m_resolution(resolution) {
		m_data = std::make_unique<uint32_t[]>(uint32_t(m_resolution));
	}

	Histogram(const Histogram& histogram) {

		m_resolution = histogram.m_resolution;
		m_data = std::make_unique<uint32_t[]>(uint32_t(m_resolution));
		memcpy(m_data.get(), histogram.m_data.get(), uint32_t(m_resolution) * 4);
	}

	Histogram& operator=(const Histogram & other) {

		if (!m_data || m_resolution != other.m_resolution) {
			m_resolution = other.m_resolution;
			m_data = std::make_unique<uint32_t[]>(uint32_t(m_resolution));
		}

		memcpy(m_data.get(), other.m_data.get(), resolutionValue(m_resolution) * sizeof(uint32_t));
		return *this;
	}

	static uint32_t resolutionValue(Resolution resolution) {
		return uint32_t(resolution);
	}

	Resolution resolutionType()const { return m_resolution; }

	uint32_t resolution()const { return uint32_t(m_resolution); }

	uint32_t& operator[](int val) { return m_data[val]; }

	const uint32_t& operator[](int val)const { return m_data[val]; }

	void clear();

	void fill(uint32_t val);

	template<typename T>
	void constructHistogram(const Image<T>& img) {

		uint16_t k = 1;
		m_resolution = Resolution(Pixel<T>::max() + 1);

		if (std::is_same<T, float>::value) {
			m_resolution = Resolution::_16bit;
			k = 65535;
		}

		m_data = std::make_unique<uint32_t[]>(resolution());

		for (auto pixel : img)
			m_data[pixel * k]++;
	}

	void addPixel(uint8_t pixel) {
		m_data[Pixel<float>::toType(pixel) * resolution()]++;
	}

	void addPixel(uint16_t pixel) {
		m_data[Pixel<float>::toType(pixel) * resolution()]++;
	}

	void addPixel(float pixel) {
		math::clipf(pixel);
		m_data[pixel * resolution()]++;
	}

	template<typename T>
	void constructHistogram(const Image<T>& img, int ch) {

		uint16_t k = 1;
		m_resolution = Resolution(Pixel<T>::max() + 1);

		if (std::is_same<T, float>::value) {
			m_resolution = Resolution::_16bit;
			k = 65535;
		}

		m_data = std::make_unique<uint32_t[]>(resolution());

		for (auto pixel : image_channel(img, ch))
			m_data[pixel * k]++;
	}

	template<typename T>
	void constructMADHistogram(const Image<T>& img, int ch, T median, bool clip) {

		int k = 1;
		m_resolution = Resolution(Pixel<T>::max() + 1);

		if (std::is_same<T, float>::value) {
			m_resolution = Resolution::_16bit;
			k = 65535;
		}

		m_data = std::make_unique<uint32_t[]>(resolution());

		for (auto pixel : image_channel(img, ch))
			m_data[abs(pixel - median) * k]++;

		if (clip)
			m_data[0] = m_data[resolution() - 1] = 0;	
	}

	void resample(Resolution new_resolution);

	float medianf(bool clip = false);

	uint16_t median(bool clip = false);

	float mean(bool clip = false);

	uint16_t min(bool clip = false);

	uint16_t max(bool clip = false);

	float standardDeviation(float mean, bool clip = false);

	uint16_t mode(bool clip = false);

	uint32_t maxCount(bool clip = false);

	uint32_t minCount(bool clip = false);

	uint32_t peak(int s, int e)const;

	uint64_t count()const;

	void clip(uint32_t limit);

	void convertToCDF();
};

typedef std::vector<Histogram> HistogramVector;