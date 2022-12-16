#pragma once
#include <Eigen/Dense>

template <typename T>
class Image {
private:
	int rows = 0;
	int cols = 0;
	int total = rows * cols;
	char bitdepth = 0;

public:
	std::unique_ptr<T[]> data;
	float max = std::numeric_limits<float>::min();
	float min = std::numeric_limits<float>::max();
	float median = 0;
	float mean = 0;
	float stdev = 0;
	Eigen::Matrix3d homography = Eigen::Matrix3d::Identity();


	Image(int r, int c) :rows(r), cols(c) {
		bitdepth = sizeof(T) * 8;
		data = std::make_unique<T[]>(r * c);
	}
	Image() = default;
	//Image(const Image& img);
	Image(Image&& other) {
		rows = other.rows;
		cols = other.cols;
		bitdepth = other.bitdepth;
		total = other.total;
		max = other.max;
		min = other.min;
		median = other.median;
		mean = other.mean;
		stdev = other.stdv;
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
		if (this != &other) {
			rows = other.rows;
			cols = other.cols;
			bitdepth = other.bitdepth;
			total = other.total;
			max = other.max;
			min = other.min;
			median = other.median;
			mean = other.mean;
			stdev = other.stdev;
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
		return data[y * cols + x];
	}

	T& operator()(int x, int y) const {
		return data[y * cols + x];
	}

	Iterator begin() {
		return Iterator(this->data.get());
	}

	Iterator end() {
		return Iterator(this->data.get() + this->total);
	}

	int Rows() const { return rows; }

	int Cols() const { return cols; }

	int Total() const { return total; }

	void CopyTo(Image& img) const {
		if (img.total != total || img.bitdepth != bitdepth)
			img = Image<T>(rows, cols);
		memcpy(img.data.get(), data.get(), total * sizeof(T));
	}

	void ComputeStats() {
		std::vector<T> temp(this->Total());
		memcpy(&temp[0], this->data.get(), this->Total() * sizeof(T));
		std::nth_element(temp.begin(), temp.begin() + temp.size() / 2, temp.end());

		this->median = temp[temp.size() / 2];
		double sum = 0;
		for (T& pixel : temp ) {
			sum += pixel;
			if (pixel > this->max)
				this->max = pixel;
			else if (pixel < this->min)
				this->min = pixel;
		}

		this->mean = sum / this->Total();

		double d, var = 0;
		for (T& pixel : temp) {
			d = pixel - this->mean;
			var += d * d;
		}
		this->stdev = sqrt(var / this->Total());

	}

	T Median() {
		std::vector<T> temp(total);
		memcpy(&temp[0], data.get(), total * sizeof(T));
		std::nth_element(temp.begin(), temp.begin() + total / 2, temp.end());
		return temp[total / 2];
	}

	float Mean() {
		double sum = 0;
		for (auto& pixel : *this)
			sum += pixel;

		return float(sum / this->Total());
	}

	float Standard_Deviation() {

		if (this->mean == 0)
			this->mean = this->Mean();

		float d, var = 0;
		for (int el = 0; el < this->total; ++el) {
			d = data[el] - mean;
			var += d * d;
		}
		return (float)sqrt(var / total);
	}

	float MAD() {

		std::vector<T> imgbuf(total);

		memcpy(&imgbuf[0], this->data.get(), total * 4);

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

	float AvgDev_trimmed() {

		double sum = 0;
		int count = 0;

		if (this->median == 0)
			this->median = (*this).Median();

		for (auto& pixel : *this) {
			if (pixel < 0.00002f || pixel > 0.99998f)
				continue;

			sum += fabs(pixel - this->median);
			count++;
		}

		return float(sum / count);
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

namespace ImageOP {

	void TiffRead(std::string file, Image32& img);

	void FitsRead(std::string file, Image32& img);

	//void XISFRead(std::string file, Image32& img);

	void FitsWrite(Image32& img, std::string filename);

	void AlignFrame(Image32& img, Eigen::Matrix3d homography, std::function<float(Image32&, double& x_s, double& y_s)> interp_type);

	void DrizzleFrame(Image32& input, Image32& output, float drop);

	void Resize2x_Bicubic(Image32& img);

	void ImageResize_Bicubic(Image32& img, int new_rows, int new_cols);

	void Bin2x(Image32& img);

	void MedianBlur3x3(Image32& img);

	void TrimHighLow(Image32& img, float high, float low);

	void MaxMin_Normalization(Image32& img, float max, float min);

	void STFImageStretch(Image32& img);

	void ASinhStretch(Image32& img, float stretch_factor);
}