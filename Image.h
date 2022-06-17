#pragma once
#include <memory>
#include <Eigen/Dense>
#include "tiffio.h"
#include "cfitsio/fitsio.h"
#include <regex>
#include <array>


class Image
{
public:
	void* data = nullptr;
	int rows = 0;
	int cols = 0;
	int total = rows * cols;
	int type = 0;

	Image(int rows, int cols, int bitdepth);
	Image() = default;
	Image(const Image& img);
	Image(Image&& img);
	~Image() {
		delete[](void*)data;
		data = nullptr;
	}

	Image& operator=(Image &&img) {
		if (this != &img) {
			rows = img.rows;
			cols = img.cols;
			type = img.type;
			total = img.total;
			if (data) {
				delete[] data;
				data = nullptr;
			}
			data = img.data;
			img.data = nullptr;
		}
		return *this;
	}

	Image& operator=(const Image &img) {
		rows = img.rows;
		cols = img.cols;
		type = img.type;
		total = img.total;
		data = img.data;

		return *this;
	}

	template <typename T> inline
		T& at(int index) {
		return ((T*)data)[index];
	}

	template <typename T> inline
		T& at(int index) const {
		return ((T*)data)[index];
	}

	template <typename T> inline
		T& at(int row, int col) {
		return ((T*)data)[row * cols + col];
	}

	template <typename T> inline
		T& at(int row, int col) const {
		return ((T*)data)[row * cols + col];
	}

	template<typename Old, typename New>
	void Convert(Image &output) {

		Old* ptr = (Old*)Image::data;

		output.data = new New[output.total];
		New* nptr = (New*)output.data;

		for (size_t el = 0; el < total; ++el)
			nptr[el] = ptr[el];

		output.type = sizeof(New) * 8;

		delete[] ptr;
	}

	Image DeepCopy();

	void Release();

};

namespace ImageOP {
	Image ImRead(char* file);

	void AlignFrame(Image& img, Eigen::Matrix3d homography);

	void MedianBlur3x3(Image& img);

	double Median(Image& img);

	double StandardDeviation(Image& img);

	double StandardDeviation256(Image& img);

	void nMAD(Image& img, double& median, double& nMAD);

	void AvgAbsDev(Image& img, double& median, double& abs_dev);

	void STFImageStretch(Image& img);

	void STFImageStretch256(Image& img);
}