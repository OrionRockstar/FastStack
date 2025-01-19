#pragma once
#include "Image.h"

class Histogram {
	std::unique_ptr<uint32_t[]> m_data;
	uint32_t m_resolution = 256;
	int m_count = 0;

public:
	Histogram() = default;

	Histogram(uint32_t resolution) : m_resolution(resolution) {
		m_data = std::make_unique<uint32_t[]>(m_resolution);
	}

	uint32_t resolution()const { return m_resolution; }

	int count()const { return m_count; }

	uint32_t& operator[](int val) { return m_data[val]; }

	template<typename T>
	void constructHistogram(const Image<T>& img) {

		int k = 1;
		m_resolution = Pixel<T>::max() + 1;

		if (std::is_same<T, float>::value) {
			m_resolution = 65536;
			k = 65535;
		}

		m_data = std::make_unique<uint32_t[]>(m_resolution);

		for (auto pixel : img)
			m_data[pixel * k]++;

		m_count = 0;

		for (int i = 0; i < m_resolution; ++i)
			m_count += m_data[i];
	}

	template<typename T>
	void constructHistogram(const Image<T>& img, int ch) {

		int k = 1;
		m_resolution = Pixel<T>::max() + 1;

		if (std::is_same<T, float>::value) {
			m_resolution = 65536;
			k = 65535;
		}

		m_data = std::make_unique<uint32_t[]>(m_resolution);

		for (auto pixel : image_channel(img, ch))
			m_data[pixel * k]++;

		m_count = 0;

		for (int i = 0; i < m_resolution; ++i)
			m_count += m_data[i];	

	}

	template<typename T>
	void constructMADHistogram(const Image<T>& img, int ch, T median, bool clip) {

		int k = 1;
		m_resolution = Pixel<T>::max() + 1;

		if (std::is_same<T, float>::value) {
			m_resolution = 65536;
			k = 65535;
		}

		m_data = std::make_unique<uint32_t[]>(m_resolution);

		for (auto pixel : image_channel(img, ch))
			m_data[abs(pixel - median) * k]++;

		if (clip)
			m_data[0] = m_data[m_resolution - 1] = 0;

		m_count = 0;

		for (int i = 0; i < m_resolution; ++i)
			m_count += m_data[i];
		
	}

	void resample(uint32_t new_resolution) {

		if (m_resolution == new_resolution)
			return;

		double k = double(new_resolution - 1) / (m_resolution - 1);

		std::unique_ptr<uint32_t[]> data = std::make_unique<uint32_t[]>(new_resolution);

		for (int i = 0; i < m_resolution; ++i)
			data[i * k] += m_data[i];
		
		m_resolution = new_resolution;
		m_data = std::move(data);
	}

	template<typename T>
	T Median(bool clip = false) {

		int occurrences = 0;
		int median1 = 0, median2 = 0;
		int medianlength = m_count / 2;

		int cl = 0, cu = m_resolution;
		if (clip) { cl = 1; cu -= 1; medianlength = (m_count - m_data[0] - m_data[m_resolution - 1]) / 2; }

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

		T med = (median1 + median2) / 2;

		if (std::is_same<T, float>::value) {
			med += float(medianlength - (occurrences - m_data[median1])) / m_data[med];
			med /= 65535.0;
		}

		return med;
	}

	float medianf(bool clip = false) {

		int occurrences = 0;
		int median1 = 0, median2 = 0;
		int medianlength = m_count / 2;

		int cl = 0, cu = m_resolution;
		if (clip) { cl = 1; cu -= 1; medianlength = (m_count - m_data[0] - m_data[m_resolution - 1]) / 2; }

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

		float med = (median1 + median2) / 2;

		med += float(medianlength - (occurrences - m_data[median1])) / m_data[med];
		med /= (m_resolution - 1);

		return med;
	}

	float median(bool clip = false) {

		int occurrences = 0;
		int median1 = 0, median2 = 0;
		int medianlength = m_count / 2;

		int cl = 0, cu = m_resolution;
		if (clip) { cl = 1; cu -= 1; medianlength = (m_count - m_data[0] - m_data[m_resolution - 1]) / 2; }

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

		for (int i = clip; i < m_resolution - clip; ++i) {
			sum += (i * m_data[i]);
			count += m_data[i];
		}

		return sum / count;
	}

	uint16_t min(bool clip = false) {

		for (int i = clip; i < m_resolution - clip; ++i)
			if (m_data[i] > 0)
				return i;

		return 0;
	}

	uint16_t max(bool clip = false) {

		for (int i = m_resolution - clip - 1; i > 0; --i)
			if (m_data[i] > 0)
				return i;

		return m_resolution - 1;
	}

	float standardDeviation(float mean, bool clip = false) {

		double d = 0, var = 0;
		uint32_t count = 0;

		for (int i = clip; i < m_resolution - clip; ++i) {
			d = i - mean;
			var += d * d * m_data[i];
			count += m_data[i];
		}

		return sqrt(var / count);
	}

	uint32_t peak(bool clip = false) {

		uint32_t peak = 0;

		for (int i = clip; i < m_resolution - clip; ++i)
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
};

