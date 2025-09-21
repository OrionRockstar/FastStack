#include "pch.h"
#include "Histogram.h"


void Histogram::fill(uint32_t val) {

	for (int i = 0; i < resolution(); ++i)
		m_data[i] = val;
}

void Histogram::resample(Resolution new_resolution) {

	if (resolutionType() == new_resolution)
		return;

	double k = double(uint32_t(new_resolution) - 1) / (resolution() - 1);


	std::unique_ptr<uint32_t[]> data = std::make_unique<uint32_t[]>(uint32_t(new_resolution));

	for (int i = 0; i < resolution(); ++i)
		data[i * k] += m_data[i];

	m_resolution = new_resolution;
	m_data = std::move(data);
}

float Histogram::medianf(bool clip) {

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

uint16_t Histogram::median(bool clip) {

	int occurrences = 0;
	int median1 = 0, median2 = 0;
	auto c = count();
	int medianlength = c / 2;

	int cl = 0, cu = Histogram::resolution();
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

float Histogram::mean(bool clip) {

	double sum = 0;
	uint32_t count = 0;

	for (int i = clip; i < resolution() - clip; ++i) {
		sum += (i * m_data[i]);
		count += m_data[i];
	}

	return sum / count;
}

uint16_t Histogram::min(bool clip) {

	for (int i = clip; i < resolution() - clip; ++i)
		if (m_data[i] > 0)
			return i;

	return 0;
}

uint16_t Histogram::max(bool clip) {

	for (int i = resolution() - clip - 1; i > 0; --i)
		if (m_data[i] > 0)
			return i;

	return resolution() - 1;
}

float Histogram::standardDeviation(float mean, bool clip) {

	double d = 0, var = 0;
	uint32_t count = 0;

	for (int i = clip; i < resolution() - clip; ++i) {
		d = i - mean;
		var += d * d * m_data[i];
		count += m_data[i];
	}

	return sqrt(var / count);
}

uint32_t Histogram::maxCount(bool clip) {

	uint32_t max_count = 0;

	for (int i = clip; i < resolution() - clip; ++i)
		if (m_data[i] > max_count)
			max_count = m_data[i];

	return max_count;
}

uint32_t Histogram::minCount(bool clip) {

	uint32_t min_count = std::numeric_limits<uint32_t>::max();

	for (int i = clip; i < resolution() - clip; ++i)
		if (m_data[i] < min_count && m_data[i] != 0)
			min_count = m_data[i];

	return min_count;
}

uint32_t Histogram::peak(int s, int e)const {

	if (e < 0 || s < 0 || this->resolution() < 0)
		return 0;

	uint32_t p = 0;
	for (int i = s; i <= e; ++i)
		if (m_data[i] > p)
			p = m_data[i];

	return p;
}

uint64_t Histogram::count()const {

	uint64_t sum = 0;
	for (auto i = 0; i < resolution(); ++i)
		sum += m_data[i];
	return sum;
}

void Histogram::clip(uint32_t limit) {

	for (int iter = 0; iter < 5; ++iter) {
		int clip_count = 0;

		for (int i = 0; i < resolution(); ++i)
			if (m_data[i] > limit) {
				clip_count += (m_data[i] - limit);
				m_data[i] = limit;
			}

		int d = clip_count / resolution(); // evenly distributes clipped values to histogram
		int r = clip_count % resolution(); // dristubues remainder of clipped values 

		if (d != 0) {
			for (int i = 0; i < resolution(); ++i)
				m_data[i] += d;
		}

		if (r != 0) {
			int skip = (resolution() - 1) / r;
			for (int i = 0; i < resolution(); i += skip)
				m_data[i]++;
		}

		if (r == 0 && d == 0)
			break;
	}

}

void Histogram::convertToCDF() {

	for (int i = 1; i < resolution(); ++i)
		m_data[i] += m_data[i - 1];
}