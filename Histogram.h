#pragma once

class Histogram {
	std::unique_ptr<uint32_t[]> m_data;
	int m_resolution = 8;
	int m_count = 0;
	int m_size = 0;
	uint32_t m_peak = 0;

public:
	Histogram() = default;

	int Resolution()const { return m_resolution; }

	constexpr int MaxValue() { return m_size - 1; }

	int Count()const { return m_count; }

	int Size()const { return m_size; }

	uint32_t Peak()const { return m_peak; }

	uint32_t& operator[](int val) { return m_data[val]; }

	template<class ImageIterator>
	void ConstructHistogram(const ImageIterator begin,const ImageIterator end) {
		using T = ImageIterator::ValueType;

		int k = 1;
		m_resolution = (sizeof(T) * 8);

		if (std::is_same<T, float>::value) {
			m_resolution = 16;
			k = 65535;
		}

		m_size = (1 << m_resolution);

		m_data = std::make_unique<uint32_t[]>(m_size);

		for (auto pixel = begin; pixel != end; ++pixel)
			m_data[*pixel * k]++;

		m_peak = 0;
		m_count = 0;

		for (int i = 0; i < m_size; ++i) {

			if (m_data[i] > m_peak)
				m_peak = m_data[i];

			m_count += m_data[i];
		}
	}

	template<typename ImageIter, typename T>
	void ConstructMADHistogram(const ImageIter begin, const ImageIter end, T median, bool clip) {

		int k = 1;
		m_resolution = (sizeof(T) * 8);

		if (std::is_same<T, float>::value) {
			m_resolution = 16;
			k = 65535;
		}

		m_size = (1 << m_resolution);

		m_data = std::make_unique<uint32_t[]>(m_size);

		for (auto pixel = begin; pixel != end; ++pixel)
			m_data[abs(*pixel - median) * k]++;
		
		if (clip)
			m_data[0] = m_data[m_size - 1] = 0;
		
		m_peak = 0;
		m_count = 0;

		for (int i = 0; i < m_size; ++i) {

			if (m_data[i] > m_peak)
				m_peak = m_data[i];

			m_count += m_data[i];
		}
	}

	void Resample(int new_resolution) {
		if (m_resolution == new_resolution)
			return;

		m_resolution = new_resolution;
		int new_size = (1 << m_resolution);

		double k = double(new_size) / m_size;

		std::unique_ptr<uint32_t[]> data = std::make_unique<uint32_t[]>(new_size);

		for (int i = 0; i < m_size; ++i)
			data[i * k] += m_data[i];
		
		m_data = std::move(data);
		m_size = new_size;

		m_peak = 0;
		for (int i = 0; i < m_size; ++i)
			if (m_data[i] > m_peak)
				m_peak = m_data[i];
	}

	template<typename T>
	T Median(bool clip = false) {

		int occurrences = 0;
		int median1 = 0, median2 = 0;
		int medianlength = m_count / 2;

		int cl = 0, cu = m_size;
		if (clip) { cl = 1; cu -= 1; medianlength = (m_count - m_data[0] - m_data[m_size - 1]) / 2; }

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
};

