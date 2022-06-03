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
			switch (type) {
			case 8:
				data = new unsigned char[total];
				memcpy(data, img.data, (size_t)total);
				break;
			case 16:
				data = new unsigned short[total];
				memcpy(data, img.data, (size_t)total * 2);
				break;
			case 32:
				data = new float[total];
				memcpy(data, img.data, (size_t)total * 4);
				break;
			}
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

	template<typename Old, typename New>
	void Convert(Image &output) {

		Old* ptr = (Old*)Image::data;

		output.data = new New[output.total];
		New* nptr = (New*)output.data;

		std::copy(&ptr[0], &ptr[Image::total], &nptr[0]);

		switch (sizeof(New)) {
		case 1: output.type = 8; break;
		case 2: output.type = 16; break;
		case 4: output.type = 32; break;
		default: output.type = 0; break;
		}
		delete ptr;
	}

	Image DeepCopy();

	void Release();

	void Update(int r, int c, int bd);
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