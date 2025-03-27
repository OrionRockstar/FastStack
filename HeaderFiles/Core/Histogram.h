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
	//int m_count = 0;

public:


	Histogram() = default;

	Histogram(Resolution resolution) : m_resolution(resolution) {
		m_data = std::make_unique<uint32_t[]>(uint32_t(m_resolution));
	}

	Histogram(const Histogram& histogram) {

		m_resolution = histogram.m_resolution;
		m_data = std::make_unique<uint32_t[]>(uint32_t(m_resolution));
		memcpy(m_data.get(), histogram.m_data.get(), uint32_t(m_resolution) * 4);
	}

	Histogram& operator=(const Histogram & other) {

		m_resolution = other.m_resolution;
		m_data = std::make_unique<uint32_t[]>(uint32_t(m_resolution));
		memcpy(m_data.get(), other.m_data.get(), uint32_t(m_resolution) * 4);
		return *this;
	}

	static uint32_t resolutionValue(Resolution resolution) {
		return uint32_t(resolution);
	}

	Resolution resolutionType()const { return m_resolution; }

	uint32_t resolution()const { return uint32_t(m_resolution); }

	uint32_t& operator[](int val) { return m_data[val]; }

	void fill(uint32_t val) {

		for (int i = 0; i < resolution(); ++i)
			m_data[i] = val;
	}

	template<typename T>
	void constructHistogram(const Image<T>& img) {

		int k = 1;
		m_resolution = Resolution(Pixel<T>::max() + 1);

		if (std::is_same<T, float>::value) {
			m_resolution = Resolution::_16bit;
			k = 65535;
		}

		m_data = std::make_unique<uint32_t[]>(resolution());

		for (auto pixel : img)
			m_data[pixel * k]++;
	}

	template<typename T>
	void constructHistogram(const Image<T>& img, int ch) {

		int k = 1;
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

	void resample(Resolution new_resolution) {

		if (m_resolution == new_resolution)
			return;

		double k = double(uint32_t(new_resolution) - 1) / (uint32_t(m_resolution) - 1);


		std::unique_ptr<uint32_t[]> data = std::make_unique<uint32_t[]>(uint32_t(new_resolution));

		for (int i = 0; i < resolution(); ++i)
			data[i * k] += m_data[i];
		
		m_resolution = new_resolution;
		m_data = std::move(data);
	}

	float medianf(bool clip = false) {

		int occurrences = 0;
		int median1 = 0, median2 = 0;
		auto c = count();
		int medianlength = c / 2;

		int cl = 0, cu = resolution();
		if (clip) { cl = 1; cu -= 1; medianlength = (c - m_data[0] - m_data[resolution() - 1]) / 2; }

		for (int i = cl; i < cu; ++i) {
			occurrences += m_data[i];
			if (occurrences > medianlength) {
				median1 = i;
				median2 = i;

				break;
			}
			else if (occurrences == medianlength) {
				median1 = i;
				for (int j = i + 1; j < cu; j++) {
					if (m_data[j] > 0) {
						median2 = j;
						break;
					}
				}
				break;
			}
		}

		float med = (median1 + median2) / 2.0;

		med += float(medianlength - (occurrences - m_data[median1])) / m_data[med];
		med /= (resolution() - 1);

		return med;
	}

	uint16_t median(bool clip = false) {

		int occurrences = 0;
		int median1 = 0, median2 = 0;
		auto c = count();
		int medianlength = c / 2;

		int cl = 0, cu = resolution();
		if (clip) { cl = 1; cu -= 1; medianlength = (c - m_data[0] - m_data[resolution() - 1]) / 2; }

		for (int i = cl; i < cu; ++i) {
			occurrences += m_data[i];
			if (occurrences > medianlength) {
				median1 = i;
				median2 = i;

				break;
			}
			else if (occurrences == medianlength) {
				median1 = i;
				for (int j = i + 1; j < cu; ++j) {
					if (m_data[j] > 0) {
						median2 = j;
						break;
					}
				}
				break;
			}
		}

		uint16_t med = (median1 + median2) / 2;
		return med;
	}

	float mean(bool clip = false) {

		double sum = 0;
		uint32_t count = 0;

		for (int i = clip; i < resolution() - clip; ++i) {
			sum += (i * m_data[i]);
			count += m_data[i];
		}

		return sum / count;
	}

	uint16_t min(bool clip = false) {

		for (int i = clip; i < resolution() - clip; ++i)
			if (m_data[i] > 0)
				return i;

		return 0;
	}

	uint16_t max(bool clip = false) {

		for (int i = resolution() - clip - 1; i > 0; --i)
			if (m_data[i] > 0)
				return i;

		return resolution() - 1;
	}

	float standardDeviation(float mean, bool clip = false) {

		double d = 0, var = 0;
		uint32_t count = 0;

		for (int i = clip; i < resolution() - clip; ++i) {
			d = i - mean;
			var += d * d * m_data[i];
			count += m_data[i];
		}

		return sqrt(var / count);
	}

	uint32_t peak(bool clip = false) {

		uint32_t peak = 0;

		for (int i = clip; i < resolution() - clip; ++i)
			if (m_data[i] > peak)
				peak = m_data[i];

		return peak;
	}

	uint32_t peak(int s, int e)const {

		if (e < 0 || s < 0 || this->resolution() < 0)
			return 0;

		uint32_t p = 0;
		for (int i = s; i <= e; ++i)
			if (m_data[i] > p)
				p = m_data[i];

		return p;
	}

	uint64_t count()const {

		uint64_t sum = 0;
		for (auto i = 0; i < resolution(); ++i)
			sum += m_data[i];
		return sum;
	}

	void clip(uint32_t limit) {

		for (int iter = 0; iter < 5; ++iter) {
			int clip_count = 0;

			for (int el = 0; el < resolution(); ++el)
				if (m_data[el] > limit) {
					clip_count += (m_data[el] - limit);
					m_data[el] = limit;
				}

			int d = clip_count / resolution(); // evenly distributes clipped values to histogram
			int r = clip_count % resolution(); // dristubues remainder of clipped values 

			if (d != 0)
				for (int el = 0; el < resolution(); ++el)
					m_data[el] += d;

			if (r != 0) {
				int skip = (resolution() - 1) / r;
				for (int el = 0; el < resolution(); el += skip)
					m_data[el]++;
			}

			if (r == 0 && d == 0)
				break;
		}
	}
};

