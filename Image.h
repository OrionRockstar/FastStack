#pragma once
#include <Eigen/Dense>
#include<filesystem>

struct Stats {
	float max = std::numeric_limits<float>::min();
	float min = std::numeric_limits<float>::max();
	float median = 0;
	float mean = 0;
	float stdev = 0;
	float mad = 0;
	float avgDev = 0;
	float bwmv = 0;

};

template <typename T>
class Image {
private:
	int m_rows = 0;
	int m_cols = 0;
	int m_channels = 0;
	int m_total = m_rows * m_cols;
	uint16_t m_bitdepth = 0;

	inline static int m_max_val = 0;
	inline static int m_multiplier = 0;

	T* m_red = nullptr;
	T* m_green = nullptr;
	T* m_blue = nullptr;

public:
	std::unique_ptr<T[]> data;
	float max = std::numeric_limits<float>::min();
	float min = std::numeric_limits<float>::max();
	float median = 0;
	float mean = 0;
	float stdev = 0;
	float mad = 0;
	float avgDev = 0;
	float bwmv = 0;
	Eigen::Matrix3d homography = Eigen::Matrix3d::Identity();

	Stats red;
	Stats green;
	Stats blue;

	Image(int r, int c, int ch = 1) :m_rows(r), m_cols(c), m_channels(ch) {
		assert(ch == 1 || ch == 3);
		m_bitdepth = sizeof(T) * 8;
		data = std::make_unique<T[]>(r * c * ch);

		if (m_channels == 3) {
			m_red = data.get();
			m_green = data.get() + m_total;
			m_blue = data.get() + 2 * m_total;
		}

		switch (m_bitdepth) {
		case 32:
			m_max_val = 1.0f;
			m_multiplier = 65535;
			break;
		case 16:
			m_max_val = 65535;
			m_multiplier = 1;
			break;

		case 8:
			m_max_val = 255;
			m_multiplier = 1;
			break;

		default:
			m_max_val = 1.0f;
			m_multiplier = 65535;
			break;
		}
	}

	Image() = default;
	//Image(const Image& img);
	Image(Image&& other) {

		m_rows = other.m_rows;
		m_cols = other.m_cols;
		m_channels = other.m_channels;

		m_bitdepth = other.m_bitdepth;
		m_total = other.m_total;
		m_max_val = other.m_max_val;
		m_multiplier = other.m_multiplier;

		m_red = other.m_red;
		m_green = other.m_green;
		m_blue = other.m_blue;
		
		max = other.max;
		min = other.min;
		median = other.median;
		mean = other.mean;
		stdev = other.stdev;
		mad = other.mad;
		avgDev = other.avgDev;
		bwmv = other.bwmv;

		homography = other.homography;
		data = std::move(other.data);

	}

	~Image() {}

	struct Iterator {
		using ValueType = T;
		using PointerType = ValueType*;
		using ReferenceType = ValueType&;

		Iterator(PointerType ptr) : m_ptr(ptr) {};

		Iterator operator++() { m_ptr++; return *this; }

		Iterator operator++(int) {
			Iterator iterator = *this;
			++(*this);
			return iterator;
		}

		Iterator operator--() { m_ptr--; return *this; }

		Iterator operator--(int) {
			Iterator iterator = *this;
			--(*this);
			return iterator;
		}

		ReferenceType operator*() { return *m_ptr; }

		PointerType operator->() { return m_ptr; }

		bool operator ==(const Iterator& other) const {
			return m_ptr == other.m_ptr;
		}

		bool operator !=(const Iterator& other) const {
			return m_ptr != other.m_ptr;
		}

	private:
		PointerType m_ptr;
	};

	Image& operator=(Image&& other) {
		assert(m_channels == other.m_channels);

		if (this != &other) {
			m_rows = other.m_rows;
			m_cols = other.m_cols;
			m_channels = other.m_channels;

			m_bitdepth = other.m_bitdepth;
			m_total = other.m_total;
			m_max_val = other.m_max_val;
			m_multiplier = other.m_multiplier;

			m_red = other.m_red;
			m_green = other.m_green;
			m_blue = other.m_blue;

			max = other.max;
			min = other.min;
			median = other.median;
			mean = other.mean;
			stdev = other.stdev;
			mad = other.mad;
			avgDev = other.avgDev;
			bwmv = other.bwmv;

			homography = other.homography;
			data = std::move(other.data);
		}
		return *this;
	}

	T& operator[](int el) {
		return data[el];
	}

	T& operator[](int el) const {
		return data[el];
	}

	T& operator()(int x, int y) {
		return data[y * m_cols + x];
	}

	T& operator()(int x, int y) const {
		return data[y * m_cols + x];
	}

	T& operator()(int x, int y, int ch) {
		return data[ch * m_total + y * m_cols + x];
	}

	T& operator()(int x, int y, int ch) const {
		return data[ch * m_ total + y * m_cols + x];
	}

	Iterator begin() {
		return Iterator(this->data.get());
	}

	Iterator end() {
		return Iterator(this->data.get() + this->m_total);
	}

	Iterator begin_red() {
		return Iterator(m_red);
	}

	Iterator end_red() {
		return Iterator(m_red + m_total);
	}

	Iterator begin_green() {
		return Iterator(m_green);
	}

	Iterator end_green() {
		return Iterator(m_green + m_total);
	}

	Iterator begin_blue() {
		return Iterator(m_blue);
	}

	Iterator end_blue() {
		return Iterator(m_blue + m_total);
	}

	int Rows() const { return m_rows; }

	int Cols() const { return m_cols; }

	int Total() const { return m_total; }

	int Channels() const { return m_channels; }

	uint16_t Bitdepth() const { return m_bitdepth; }

	T& RedPixel(int x, int y) { return m_red[y * m_cols + x]; }

	T& GreenPixel(int x, int y) { return m_green[y * m_cols + x]; }

	T& BluePixel(int x, int y) { return m_blue[y * m_cols + x]; }

	static float GetLinearVal(float pixel) {
		return (pixel > 0.04045) ? powf((pixel + 0.055f) / 1.055, 2.4f) : (pixel / 12.92);
	}

	static float GetL(float Y) {
		return (Y <= 0.008856f) ? Y * 9.033f : (1.16f * powf(Y, 0.333333f)) - 0.16f;
	}

	void RGBtoGray() {
		if (m_channels == 1)
			return;

		//#pragma omp parallel for
		for (int el = 0; el < m_total; ++el) {
			float pix = 0.2126f * GetLinearVal(m_red[el]) + 0.7152f * GetLinearVal(m_green[el]) + 0.0722f * GetLinearVal(m_blue[el]);
			data[el] = GetL(pix);
		}
		realloc(data.get(), m_total * sizeof(T));
		m_channels = 1;
	}

	void CopyTo(Image& dest) const {
		if (dest.m_rows != m_rows || dest.m_cols != m_cols || dest.m_bitdepth != m_bitdepth)
			dest = Image<T>(m_rows, m_cols);
		memcpy(dest.data.get(), data.get(), m_total * sizeof(T));
	}

	bool IsInBounds(int x, int y) {
		return (0 <= y && y < m_rows && 0 <= x && x < m_cols);
	}

	void FillZero() {
		for (auto& pixel : *this)
			pixel = 0;
	}

	void FillValue(T val) {
		for (auto& pixel : *this)
			pixel = val;
	}

	static float HistogramMedian(std::vector<int>& histogram, int sum_count, uint16_t bit_depth) {
		int occurrences = 0;
		int median1 = 0, median2 = 0;
		int medianlength = sum_count / 2;

		for (int i = 0; i < 65535; ++i) {
			occurrences += histogram[i];
			if (occurrences > medianlength) {
				median1 = i;
				median2 = i;

				break;
			}
			else if (occurrences == medianlength) {
				median1 = i;
				for (int j = i + 1; j <= 65535; ++j) {
					if (histogram[j] > 0) {
						median2 = j;
						break;
					}
				}
				break;
			}
		}

		float med = (median1 + median2) / 2.0;
		if (bit_depth == 32)
			return (med + (float(medianlength - (occurrences - histogram[med])) / histogram[med]));
		return med;

	}

	bool IsClippedVal(T& pixel) {
		return (pixel <= 0.0 || m_max_val <= pixel);
	}

	void ComputeStats(bool clip = false) {
		if (m_channels == 3) { AverageRGBStats(clip); return; }

		max = std::numeric_limits<float>::min();
		min = std::numeric_limits<float>::max();

		std::vector<int> hist(65536);

		int count = 0;
		double meansum = 0;

		for (auto& pixel : *this) {

			if (clip && IsClippedVal(pixel)) continue;

			if (pixel > max)
				max = pixel;
			if (pixel < min)
				min = pixel;
			meansum += pixel;
			hist[pixel * m_multiplier]++;
			count++;

		}

		median = HistogramMedian(hist, count, Bitdepth()) / m_multiplier;
		mean = meansum / count;

		std::fill(hist.begin(), hist.end(), 0);

		double avgDevsum = 0;

		for (auto& pixel : *this) {

			if (clip && IsClippedVal(pixel)) continue;

			float t = fabs(pixel - median);
			hist[t * m_multiplier]++;
			avgDevsum += t;

		}

		avgDev = avgDevsum / count;

		mad = HistogramMedian(hist, count, Bitdepth()) / m_multiplier;

		double x9mad = 1 / (9 * mad);
		double sum1 = 0, sum2 = 0;
		double Y, a;
		double d, var = 0;

		for (auto& pixel : *this) {

			if (clip && IsClippedVal(pixel)) continue;

			d = pixel - mean;
			var += d * d;

			Y = (pixel - median) * x9mad;

			(abs(Y) < 1) ? a = 1 : a = 0;

			Y *= Y;

			sum1 += (a * pow(pixel - median, 2) * pow(1 - Y, 4));
			sum2 += (a * (1 - Y) * (1 - 5 * Y));

		}

		stdev = sqrt(var / count);
		bwmv = sqrt((count * sum1) / (abs(sum2) * abs(sum2)));
	}

	void ComputeStatsRed(bool clip = false) {
		assert(m_channels == 3);

		red.max = std::numeric_limits<float>::min();
		red.min = std::numeric_limits<float>::max();

		std::vector<int> hist(65536);

		int count = 0;
		double meansum = 0;

		for (auto rpix = begin_red(); rpix != end_red(); ++rpix) {
			if (clip && IsClippedVal(*rpix)) continue;

			if (*rpix > red.max)
				red.max = *rpix;
			if (*rpix < red.min)
				red.min = *rpix;
			meansum += *rpix;
			hist[*rpix * m_multiplier]++;
			count++;
		}

		red.median = HistogramMedian(hist, count, Bitdepth()) / m_multiplier;
		red.mean = meansum / count;

		std::fill(hist.begin(), hist.end(), 0);

		double avgDevsum = 0;

		for (auto rpix = begin_red(); rpix != end_red(); ++rpix) {

			if (clip && IsClippedVal(*rpix)) continue;

			float t = fabs(*rpix - red.median);
			hist[t * m_multiplier]++;
			avgDevsum += t;

		}

		red.avgDev = avgDevsum / count;

		red.mad = HistogramMedian(hist, count, Bitdepth()) / m_multiplier;

		double x9mad = 1 / (9 * red.mad);
		double sum1 = 0, sum2 = 0;
		double Y, a;
		double d, var = 0;

		for (auto rpix = begin_red(); rpix != end_red(); ++rpix) {

			if (clip && IsClippedVal(*rpix)) continue;

			d = *rpix - red.mean;
			var += d * d;

			Y = (*rpix - red.median) * x9mad;

			(abs(Y) < 1) ? a = 1 : a = 0;

			Y *= Y;

			sum1 += (a * pow(*rpix - red.median, 2) * pow(1 - Y, 4));
			sum2 += (a * (1 - Y) * (1 - 5 * Y));

		}

		red.stdev = sqrt(var / count);
		red.bwmv = sqrt((count * sum1) / (abs(sum2) * abs(sum2)));
	}

	void ComputeStatsGreen(bool clip = false) {
		assert(m_channels == 3);

		green.max = std::numeric_limits<float>::min();
		green.min = std::numeric_limits<float>::max();

		std::vector<int> hist(65536);

		int count = 0;
		double meansum = 0;

		for (auto gpix = begin_green(); gpix != end_green(); ++gpix) {
			if (clip && IsClippedVal(*gpix)) continue;

			if (*gpix > green.max)
				green.max = *gpix;
			if (*gpix < green.min)
				green.min = *gpix;
			meansum += *gpix;
			hist[*gpix * m_multiplier]++;
			count++;
		}

		green.median = HistogramMedian(hist, count, Bitdepth()) / m_multiplier;
		green.mean = meansum / count;

		std::fill(hist.begin(), hist.end(), 0);

		double avgDevsum = 0;

		for (auto gpix = begin_green(); gpix != end_green(); ++gpix) {

			if (clip && IsClippedVal(*gpix)) continue;

			float t = fabs(*gpix - green.median);
			hist[t * m_multiplier]++;
			avgDevsum += t;

		}

		green.avgDev = avgDevsum / count;

		green.mad = HistogramMedian(hist, count, Bitdepth()) / m_multiplier;

		double x9mad = 1 / (9 * green.mad);
		double sum1 = 0, sum2 = 0;
		double Y, a;
		double d, var = 0;

		for (auto gpix = begin_green(); gpix != end_green(); ++gpix) {

			if (clip && IsClippedVal(*gpix)) continue;

			d = *gpix - green.mean;
			var += d * d;

			Y = (*gpix - green.median) * x9mad;

			(abs(Y) < 1) ? a = 1 : a = 0;

			Y *= Y;

			sum1 += (a * pow(*gpix - green.median, 2) * pow(1 - Y, 4));
			sum2 += (a * (1 - Y) * (1 - 5 * Y));

		}

		green.stdev = sqrt(var / count);
		green.bwmv = sqrt((count * sum1) / (abs(sum2) * abs(sum2)));
	}

	void ComputeStatsBlue(bool clip = false) {
		assert(m_channels == 3);

		blue.max = std::numeric_limits<float>::min();
		blue.min = std::numeric_limits<float>::max();

		std::vector<int> hist(65536);

		int count = 0;
		double meansum = 0;

		for (auto bpix = begin_blue(); bpix != end_blue(); ++bpix) {
			if (clip && IsClippedVal(*bpix)) continue;

			if (*bpix > blue.max)
				blue.max = *bpix;
			if (*bpix < blue.min)
				blue.min = *bpix;
			meansum += *bpix;
			hist[*bpix * m_multiplier]++;
			count++;
		}

		blue.median = HistogramMedian(hist, count, Bitdepth()) / m_multiplier;
		blue.mean = meansum / count;

		std::fill(hist.begin(), hist.end(), 0);

		double avgDevsum = 0;

		for (auto bpix = begin_blue(); bpix != end_blue(); ++bpix) {

			if (clip && IsClippedVal(*bpix)) continue;

			float t = fabs(*bpix - blue.median);
			hist[t * m_multiplier]++;
			avgDevsum += t;

		}

		blue.avgDev = avgDevsum / count;

		blue.mad = HistogramMedian(hist, count, Bitdepth()) / m_multiplier;

		double x9mad = 1 / (9 * blue.mad);
		double sum1 = 0, sum2 = 0;
		double Y, a;
		double d, var = 0;

		for (auto bpix = begin_blue(); bpix != end_blue(); ++bpix) {

			if (clip && IsClippedVal(*bpix)) continue;

			d = *bpix - blue.mean;
			var += d * d;

			Y = (*bpix - blue.median) * x9mad;

			(abs(Y) < 1) ? a = 1 : a = 0;

			Y *= Y;

			sum1 += (a * pow(*bpix - blue.median, 2) * pow(1 - Y, 4));
			sum2 += (a * (1 - Y) * (1 - 5 * Y));

		}

		blue.stdev = sqrt(var / count);
		blue.bwmv = sqrt((count * sum1) / (abs(sum2) * abs(sum2)));
	}

	void ComputeStatsRGB(bool clip = false) {
		assert(m_channels = 3);

		ComputeStatsRed(clip);
		ComputeStatsGreen(clip);
		ComputeStatsBlue(clip);
	}

	void AverageRGBStats(bool clip = false) {
		ComputeStatsRGB();
		max = (red.max + green.max + blue.max) / 3.0f;
		min = (red.min + green.min + blue.min) / 3.0f;
		median = (red.median + green.median + blue.median) / 3.0f;
		mean = (red.mean + green.mean + blue.mean) / 3.0f;
		stdev = (red.stdev + green.stdev + blue.stdev) / 3.0f;
		mad = (red.mad + green.mad + blue.mad) / 3.0f;
		avgDev = (red.avgDev + green.avgDev + blue.avgDev) / 3.0f;
		bwmv = (red.bwmv + green.bwmv + blue.bwmv) / 3.0f;
	}

	void CopyStatsFrom(Image& src) {
		max = src.max;
		min = src.min;
		median = src.median;
		mean = src.mean;
		stdev = src.stdev;
		mad = src.mad;
		avgDev = src.avgDev;
		bwmv = src.bwmv;
	}

	T Median(bool clip = false) {
		std::vector<int> histogram(65536);

		if (clip) {
			int count = 0;
			for (auto& pixel : *this) {
				if (IsClippedVal(pixel)) continue;
				histogram[pixel * m_multiplier]++;
				count++;
			}
			return median = HistogramMedian(histogram, count, m_bitdepth) / m_multiplier;

		}

		for (auto& pixel : *this)
			histogram[pixel * m_multiplier]++;

		return median = HistogramMedian(histogram, m_total * m_channels, m_bitdepth) / m_multiplier;
		
	}

	float Mean(bool clip = false) {
		mean = 0;

		if (clip) {

			int count = 0;
			double sum = 0;
			for (auto& pixel : *this) {
				if (IsClippedVal(pixel)) continue;

				sum += pixel;
				count++;
			}
			return mean = sum / (m_channels * count);
		}

		double sum = 0;
		for (auto& pixel : *this)
			sum += pixel;
		return mean = sum / (m_channels * m_total);

	}

	float Standard_Deviation() {

		if (this->mean == 0)
			this->mean = this->Mean();

		float d, var = 0;
		for (int el = 0; el < this->Total(); ++el) {
			d = data[el] - mean;
			var += d * d;
		}
		return (float)sqrt(var / Total());
	}

	float MAD() {

		std::vector<T> imgbuf(Total());

		memcpy(&imgbuf[0], this->data.get(), Total() * 4);

		if (this->median == 0) {
			std::nth_element(imgbuf.begin(), imgbuf.begin() + imgbuf.size() / 2, imgbuf.end());
			this->median = imgbuf[imgbuf.size() / 2];
		}
		for (auto& pixel : imgbuf)
			pixel = fabs(pixel - this->median);

		std::nth_element(imgbuf.begin(), imgbuf.begin() + imgbuf.size() / 2, imgbuf.end());
		return imgbuf[imgbuf.size() / 2];
	}

	float nMAD() {
		return (float)1.4826 * MAD();
	}

	float BWMV() {
		//returns sqrt of biwweight midvariance
		double x9mad = 1 / (9 * this->MAD());

		if (this->median == 0)
			this->median = this->Median();

		double sum1 = 0, sum2 = 0;
		double Y, a;

		for (auto& pixel : *this) {
			Y = (pixel - this->median) * x9mad;

			(abs(Y) < 1) ? a = 1 : a = 0;

			Y *= Y;

			sum1 += (a * pow(pixel - this->median, 2) * pow(1 - Y, 4));
			sum2 += (a * (1 - Y) * (1 - 5 * Y));
		}

		return (float)sqrt((this->Total() * sum1) / (abs(sum2) * abs(sum2)));
	}

	float AvgDev(bool clip = false) {

		avgDev = 0;
		//double sum = 0;
		//int count = 0;

		Median(clip);

		if (clip) {
			for (int ch = 0; ch < m_channels; ++ch) {
				double sum = 0;
				int count = 0;
				for (auto& pixel : *this) {

					if (IsClippedVal(pixel)) continue;

					sum += fabs(pixel - median);
					count++;
				}
				avgDev += (sum / count);
			}
			return avgDev /= Channels();
		}

		for (int ch = 0; ch < m_channels; ++ch) {
			double sum = 0;
			int count = 0;
			for (auto& pixel : *this) {

				sum += fabs(pixel - median);
				count++;
			}
			avgDev += (sum / count);
		}
		return avgDev /= Channels();

	}

	T Max() {
		T max = std::numeric_limits<T>::min();
		for (T& pixel : *this)
			if (pixel > max) max = pixel;
		return max;
	}

	T Min() {
		T min = std::numeric_limits<T>::max();
		for (T& pixel: *this)
			if (pixel < min) min = pixel;
		return min;
	}

};

typedef Image<float> Image32;
typedef Image<uint16_t> Image16;
typedef Image<uint8_t> Image8;
typedef std::vector<Image32> ImageVector;
typedef std::vector<Image8> Image8Vector;
typedef std::vector<bool> WeightMap;
typedef std::function<float(Image32&, double& x_s, double& y_s, int& channel)> Interp_func;
typedef std::vector<std::filesystem::path> FileVector;

enum class FastRotate {
	rotate90CW,
	rotate90CCW,
	rotate180,
	horizontalmirror,
	verticalmirror
};

enum class ScaleEstimator {
	median,
	avgdev,
	mad,
	bwmv,
	none
};

namespace FileOP {

	void TiffRead(std::filesystem::path file, Image32& img);

	bool FitsRead(const std::filesystem::path& file, Image32& img);

	//void XISFRead(std::string file, Image32& img);

	void FitsWrite(Image32& img, const std::filesystem::path& file_path, bool overwrite = true);

	void TiffWrite(Image32& img, std::string filename);
}

namespace ImageOP {

	void AlignFrame(Image32& img, Eigen::Matrix3d homography, std::function<float(Image32&, double& x_s, double& y_s, int& channel)> interp_type);

	void DrizzleImageStack(std::vector<std::filesystem::path> light_files, Image32& output, float drop_size, ScaleEstimator scale_estimator);

	void RotateImage(Image32& img, float theta_degrees, Interp_func interp_type);

	void FastRotation(Image32& img, FastRotate type);

	void Crop(Image32& img, int top, int bottom, int left, int right);

	void Resize2x_Bicubic(Image32& img);

	void ImageResize_Bicubic(Image32& img, int new_rows, int new_cols);

	void Bin2x(Image32& img);

	void MedianBlur3x3(Image32& img);

	void B3WaveletTransform(Image32& img, ImageVector& wavelet_vector, int scale = 5);

	void B3WaveletTransformTrinerized(Image32& img, Image8Vector& wavelet_vector, float thresh, int scale_num = 5);

	void ScaleImage(Image32& ref, Image32& tgt, ScaleEstimator type);

	void ScaleImageStack(ImageVector& img_stack, ScaleEstimator type);

	void MaxMin_Normalization(Image32& img, float max, float min);

	void STFImageStretch(Image32& img);

	void ASinhStretch(Image32& img, float stretch_factor);
}