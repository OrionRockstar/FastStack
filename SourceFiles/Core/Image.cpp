#include "pch.h"
#include "Image.h"
#include "Histogram.h"

//class Histogram;

template<typename T>
Image<T>::Image(uint32_t rows, uint32_t cols, uint32_t ch) :m_rows(rows), m_cols(cols), m_channels(ch) {
	assert(ch == 1 || ch == 3);

	m_type = getImageType<T>();

	m_data = std::make_unique<T[]>(rows * cols * ch);

	if (m_channels == 3) {
		m_red = m_data.get();
		m_green = m_data.get() + m_pixel_count;
		m_blue = m_data.get() + 2 * m_pixel_count;
	}

}

template<typename T>
Image<T>::Image(const Image<T>& other) {

	m_rows = other.m_rows;
	m_cols = other.m_cols;
	m_channels = other.m_channels;

	m_type = other.m_type;
	m_pixel_count = other.m_pixel_count;
	m_total_pixel_count = other.m_total_pixel_count;

	//homography = other.homography;
	//
	m_data = std::make_unique<T[]>(m_total_pixel_count);
	memcpy(m_data.get(), other.m_data.get(), m_total_pixel_count * sizeof(T));

	if (m_channels == 3) {
		m_red = m_data.get();
		m_green = m_data.get() + m_pixel_count;
		m_blue = m_data.get() + 2 * m_pixel_count;
	}
}

template<typename T>
Image<T>::Image(Image&& other)noexcept {

	m_rows = other.m_rows;
	m_cols = other.m_cols;
	m_channels = other.m_channels;

	m_type = other.m_type;
	m_pixel_count = other.m_pixel_count;
	m_total_pixel_count = other.m_total_pixel_count;

	m_red = other.m_red;
	m_green = other.m_green;
	m_blue = other.m_blue;

	m_data = std::move(other.m_data);
}





template<typename T>
void Image<T>::truncate(T a, T b) {
	for (T& pixel : *this) {
		if (pixel < a)
			pixel = a;
		else if (pixel > b)
			pixel = b;
	}
}

template<typename T>
void Image<T>::normalize() {
	T max, min;

	computeMinMax(min, max);

	if (min == max)
		return;

	if (min < Pixel<T>::min() || Pixel<T>::max() < max)
		rescale(min, max);
}

template<typename T>
void Image<T>::rescale(T a, T b) {
	if (b == a)
		return;

	float dba = 1 / float(b - a);

	for (T& pixel : *this) {
		if (pixel < a)
			pixel = 0;
		else if (pixel > b)
			pixel = b;
		else
			pixel = (pixel - a) * dba;
	}

}

template<typename T>
void Image<T>::binerize(T threshold) {
	for (auto& pixel : *this)
		pixel = (pixel >= threshold) ? Pixel<T>::max() : 0;
}

template<typename T>
uint32_t Image<T>::computeClippedPxCount(int ch)const {

	uint32_t count = 0;
	for (auto pixel : image_channel(*this, ch))
		if (!isClipped(pixel))
			count++;

	return count;
}

template<typename T>
T Image<T>::computeMax_clipped(int ch)const {

	T max = std::numeric_limits<T>::min();

	for (T pixel : image_channel(*this, ch)) {

		if (isClipped(pixel))
			continue;

		if (pixel > max)
			max = pixel;
	}

	return max;
}

template<typename T>
T Image<T>::computeMin_clipped(int ch)const {

	T min = std::numeric_limits<T>::max();

	for (const T& pixel : image_channel(*this, ch)) {
		if (isClipped(pixel))
			continue;
		if (pixel < min)
			min = pixel;
	}

	return min;
}

template<typename T>
float Image<T>::computeMean_Clipped(int ch)const {

	double sum = 0;
	int count = 0;

	for (T pixel : image_channel(*this, ch)) {
		if (isClipped(pixel))
			continue;
		sum += pixel;
		count++;
	}

	return (count != 0) ? sum / count : 0;
}

template<typename T>
float Image<T>::computeStdDev_Clipped(int ch, float mean)const {

	double d, var = 0;
	int count = 0;

	for (const T& pixel : image_channel(*this, ch)) {
		if (isClipped(pixel))
			continue;
		d = pixel - mean;
		var += d * d;
		count++;
	}

	return (count != 0) ? sqrt(var / count) : 0;
}

template<typename T>
float Image<T>::computeAvgDev_Clipped(int ch, T median)const {

	double sum = 0;
	int count = 0;

	for (const T& pixel : image_channel(*this, ch)) {
		if (isClipped(pixel))
			continue;
		sum += fabs(pixel - median);
		count++;
	}

	return (count != 0) ? sum / count : 0;
}

template<typename T>
void Image<T>::computeMinMax(T& min, T& max)const {

	max = std::numeric_limits<T>::min();
	min = std::numeric_limits<T>::max();

	for (auto pixel : *this) {
		if (pixel > max)
			max = pixel;
		else if (pixel < min)
			min = pixel;
	}
}

template<typename T>
T Image<T>::computeMax(int ch, bool clip)const {

	if (clip)
		return computeMax_clipped(ch);

	T max = std::numeric_limits<T>::min();

	for (T pixel : image_channel(*this, ch))
		if (pixel > max)
			max = pixel;

	return max;
}

template<typename T>
T Image<T>::computeMin(int ch, bool clip)const {

	if (clip)
		return computeMin_clipped(ch);

	T min = std::numeric_limits<T>::max();

	for (T pixel : image_channel(*this, ch))
		if (pixel < min)
			min = pixel;

	return min;
}

template<typename T>
float Image<T>::computeMean(int ch, bool clip)const {
	
	if (clip)
		return computeMean_Clipped(ch);

	double sum = 0;

	for (T pixel : image_channel(*this, ch))
		sum += pixel;

	return (pxCount() != 0) ? sum / pxCount() : 0;
}

template<typename T>
T Image<T>::computeMedian(int ch, bool clip)const {

	Histogram histogram;
	histogram.constructHistogram(*this, ch);

	return (m_type == ImageType::FLOAT) ? histogram.medianf(clip) : histogram.median(clip);
}

template<typename T>
float Image<T>::computeStdDev(int ch, bool clip)const {

	float mean = computeMean(ch, clip);

	return computeStdDev(ch, mean, clip);

}

template<typename T>
float Image<T>::computeStdDev(int ch, float mean, bool clip)const {

	if (clip)
		return computeStdDev_Clipped(ch, mean);

	double d, var = 0;

	for (T pixel : image_channel(*this, ch)) {
		d = pixel - mean;
		var += d * d;
	}

	return (pxCount() != 0) ? sqrt(var / pxCount()) : 0;
}

template<typename T>
float Image<T>::computeAvgDev(int ch, bool clip)const {

	T median = computeMedian(ch, clip);

	return computeAvgDev(ch, median, clip);
}

template<typename T>
float Image<T>::computeAvgDev(int ch, T median, bool clip)const {

	if (clip)
		return computeAvgDev_Clipped(ch, median);

	double sum = 0;

	for (T pixel : image_channel(*this, ch))
		sum += fabs(pixel - median);

	return (pxCount() != 0) ? sum / pxCount() : 0;
}

template<typename T>
T Image<T>::computeMAD(int ch, bool clip)const {

	T median = computeMedian(ch, clip);

	Histogram histogram;
	histogram.constructMADHistogram(*this, ch, median, clip);

	return (m_type == ImageType::FLOAT) ? histogram.medianf(clip) : histogram.median(clip);
}

template<typename T>
T Image<T>::computeMAD(int ch, T median, bool clip)const {

	Histogram histogram;
	histogram.constructMADHistogram(*this, ch, median, clip);

	return (m_type == ImageType::FLOAT) ? histogram.medianf(clip) : histogram.median(clip);
}

template<typename T>
float Image<T>::compute_nMAD(int ch, bool clip)const {

	return 1.4826 * computeMAD(ch, clip);
}

template<typename T>
float Image<T>::compute_nMAD(int ch, T median, bool clip)const {

	return 1.4826 * computeMAD(ch, median, clip);
}

template<typename T>
float Image<T>::computeBWMV(int ch, bool clip)const {

	T median = computeMedian(ch, clip);

	return computeBWMV(ch, median, clip);
}

template<typename T>
float Image<T>::computeBWMV(int ch, T median, bool clip)const {

	T mad = computeMAD(ch, median, clip);

	double x9mad = 1 / (9 * mad);
	double sum1 = 0, sum2 = 0;
	double Y, a;
	int count = 0;

	for (T pixel : image_channel(*this, ch)) {

		if (clip && isClipped(pixel)) continue;

		Y = (pixel - median) * x9mad;

		(abs(Y) < 1) ? a = 1 : a = 0;

		Y *= Y;

		sum1 += (a * pow(pixel - median, 2) * pow(1 - Y, 4));
		sum2 += (a * (1 - Y) * (1 - 5 * Y));
		count++;
	}

	return sqrt((count * sum1) / (abs(sum2) * abs(sum2)));
}

template class Image<uint8_t>;
template class Image<uint16_t>;
template class Image<float>;

