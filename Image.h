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
	float median = 0;
	float mean = 0;
	float stdev = 0;

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

	void nMAD(float& median, float& nMAD) {
		std::vector<float> imgbuf(total);

		memcpy(&imgbuf[0], data.get(), total * 4);

		std::nth_element(imgbuf.begin(), imgbuf.begin() + imgbuf.size() / 2, imgbuf.end());
		median = imgbuf[imgbuf.size() / 2];

		for (size_t i = 0; i < imgbuf.size(); ++i)
			imgbuf[i] = fabs(imgbuf[i] - median);

		std::nth_element(imgbuf.begin(), imgbuf.begin() + imgbuf.size() / 2, imgbuf.end());
		nMAD = 1.4826 * imgbuf[imgbuf.size() / 2];
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
};

typedef Image<float> Image32;
typedef Image<uint16_t> Image16;
typedef Image<uint8_t> Image8;

namespace ImageOP {

	void TiffRead(std::string file, Image32& img);

	void FitsRead(std::string file, Image32& img);

	//void XISFRead(std::string file, Image32& img);

	void FitsWrite(Image32& img, std::string filename);

	void AlignFrame_Bilinear(Image32& img, Eigen::Matrix3d homography);

	void AlignFrame_Bicubic(Image32& img, Eigen::Matrix3d homography);

	void MedianBlur3x3(Image32& img);

	void TrimHighLow(Image32& img, float high, float low);

	void STFImageStretch(Image32& img);

}