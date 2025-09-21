#pragma once
#include "Image.h"
#include <vector>
#include "ProcessDialog.h"

class GaussianFilter {
	std::vector<float> m_gaussian_kernel;
	uint32_t m_kernel_dim = 14;
	float m_sigma = 2.0;

	struct PixelWindow {
		std::vector<float> data;
		int m_size = 0;
		int m_radius = 0;

		PixelWindow(int size) {
			data.resize(size);
			m_size = size;
			m_radius = (data.size() - 1) / 2;
		}

		PixelWindow(const PixelWindow& other) {
			data = other.data;
			m_size = other.m_size;
			m_radius = other.m_radius;
		}

		~PixelWindow() {};

		float& operator[](int el) {
			return data[el];
		}

		int size()const { return m_size; }

		int radius()const { return m_radius; }

		template<typename T>
		void populateRowWindow(const Image<T>& img, const ImagePoint& p);

		template<typename T>
		void updateRowWindow(const Image<T>& img, const ImagePoint& p);

		template<typename T>
		void populateColWindow(const Image<T>& img, const ImagePoint& p);

		template<typename T>
		void updateColWindow(const Image<T>& img, const ImagePoint& p);
	};

	using PW = PixelWindow;
public:
	GaussianFilter() = default;

	GaussianFilter(float sigma);

	float sigma()const { return m_sigma; }

	void setSigma(float sigma);

	//void setKernelDimension(int kernel_dimension);

private:
	std::vector<float> buildGaussianKernel_1D(uint32_t size, float sigma)const;

public:

	template<typename T>
	void apply(Image<T>& img);

};
