#pragma once
#include <memory>
#include <Eigen/Dense>
#include "tiffio.h"
#include "cfitsio/fitsio.h"
#include <array>

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
	Image(Image&& img) {
		rows = img.rows;
		cols = img.cols;
		bitdepth = img.bitdepth;
		total = img.total;
		median = img.median;
		mean = img.mean;
		stdev = img.stdv;
		data = std::move(img.data);
	}

	~Image() {}

	Image& operator=(Image&& img) {
		if (this != &img) {
			rows = img.rows;
			cols = img.cols;
			bitdepth = img.bitdepth;
			total = img.total;
			median = img.median;
			mean = img.mean;
			stdev = img.stdev;
			data = std::move(img.data);
		}
		return *this;
	}

	T& operator[](int el) {
		return data[el];
	}

	T& operator[](int el) const {
		return data[el];
	}

	T& operator()(int y, int x) {
		return data[y * cols + x];
	}

	T& operator()(int y, int x) const {
		return data[y * cols + x];
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
		float sum = 0;
		for (size_t el = 0; el < temp.size(); ++el) {
			sum += temp[el];
			if (temp[el] > this->max)
				this->max = temp[el];
			else if (temp[el] < this->min)
				this->min = temp[el];
		}
		this->mean = sum / this->Total();
		float d, var = 0;
		for (size_t el = 0; el < temp.size(); ++el) {
			d = temp[el] - this->mean;
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
		float sum = 0;
		for (int el = 0; el < total; ++el)
			sum += data[el];
		return sum / total;
	}

	float Standard_Deviation(float mean) {
		float d, var = 0;
		for (int el = 0; el < this->total; ++el) {
			d = data[el] - mean;
			var += d * d;
		}
		return sqrt(var / total);
	}

	float nMAD() {

		std::vector<T> imgbuf(total);

		memcpy(&imgbuf[0], this->data.get(), total * 4);

		if (this->median == 0) {
			std::nth_element(imgbuf.begin(), imgbuf.begin() + imgbuf.size() / 2, imgbuf.end());
			this->median = imgbuf[imgbuf.size() / 2];
		}

		for (size_t i = 0; i < imgbuf.size(); ++i)
			imgbuf[i] = fabs(imgbuf[i] - this->median);

		std::nth_element(imgbuf.begin(), imgbuf.begin() + imgbuf.size() / 2, imgbuf.end());
		return (float)1.4826 * imgbuf[imgbuf.size() / 2];
	}

	float AvgDev_trimmed() {

		float sum = 0;
		int count = 0;
		for (int el = 0; el < total; ++el) {
			if (data[el] < 0.00002f || data[el] > 0.99998f)
				continue;
			sum += fabs(data[el] - this->median);
			count++;
		}
		return sum / count;
	}

	T Max() {
		T max = std::numeric_limits<T>::min();
		for (int el = 0; el < total; ++el)
			if (data[el] > max) max = data[el];
		return max;
	}

	T Min() {
		T min = std::numeric_limits<T>::max();
		for (int el = 0; el < this->Total(); ++el)
			if (this->data[el] < min) min = this->data[el];
		return min;
	}

	bool IsZero() {
		for (int el = 0; el < this->Total(); ++el)
			if (this->data[el] != 0)
				return false;
		return true;
	}
};

typedef Image<float> Image32;
typedef Image<uint16_t> Image16;
typedef Image<uint8_t> Image8;

namespace ImageOP {

	void TiffRead(std::string file, Image32& img);

	void FitsRead(std::string file, Image32& img);

	//void XISFRead(std::string file, Image32& img);

	void FitsWrite(Image32& img, std::string filename);

	void AlignFrame(Image32& img, Eigen::Matrix3d homography, float (*interp_type)(Image32&, double& x_s, double& y_s));

	void DrizzleFrame(Image32& input, Image32& output, float drop);

	void Resize2x_Bicubic(Image32& img);

	void ImageResize_Bicubic(Image32& img, int new_rows, int new_cols);

	void Bin2x(Image32& img);

	void MedianBlur3x3(Image32& img);

	void TrimHighLow(Image32& img, float high, float low);

	void STFImageStretch(Image32& img);

}