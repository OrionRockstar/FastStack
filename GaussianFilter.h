#pragma once
#include "Image.h"
#include <vector>

template<typename T>
struct PixelWindow {
	std::vector<T> data;
	int m_size = 0;
	int m_radius = 0;

	PixelWindow(int size) {
		data.resize(size);
		m_size = size;
		m_radius = (data.size() - 1) / 2;
	}
	~PixelWindow() {};

	T& operator[](int el) {
		return data[el];
	}

	int Size()const { return m_size; }
	int Radius()const { return m_radius; }

	void PopulateRowWindow(Image<T>& img, int y, int ch) {

		for (int j = 0; j < m_size; ++j) {
			int xx = j - m_radius;
			if (xx < 0)
				xx = -xx;

			data[j] = img(xx, y, ch);
		}
	}

	void UpdateRowWindow(Image<T>& img, int x, int y, int ch) {

		data.erase(data.begin());

		int xx = x + m_radius;
		if (xx >= img.Cols())
			xx = 2 * img.Cols() - (xx + 1);

		data.emplace_back(img(xx, y, ch));

	}

	void PopulateColWindow(Image<T>& img, int x, int ch) {

		for (int j = 0; j < m_size; ++j) {
			int yy = j - m_radius;
			if (yy < 0)
				yy = -yy;

			data[j] = img(x, yy, ch);
		}
	}

	void UpdateColWindow(Image<T>& img, int x, int y, int ch) {

		data.erase(data.begin());

		int yy = y + m_radius;
		if (yy >= img.Rows())
			yy = 2 * img.Rows() - (yy + 1);

		data.emplace_back(img(x, yy, ch));

	}

};

class GaussianFilter {
	std::vector<float> gaussian_kernel;
	int m_kernel_dim = 0;
	float m_sigma = 0;

public:
	GaussianFilter(float sigma) : m_sigma(sigma) {

		int k_rad = (3.0348 * m_sigma) + 0.5;
		m_kernel_dim = 2 * k_rad + 1;

		gaussian_kernel = std::vector<float>(m_kernel_dim);

		float s = 2 * m_sigma * m_sigma;
		float k1 = 1 / sqrtf(M_PI * s), k2 = 1 / s;
		float g_sum = 0;

		for (int j = -k_rad; j <= k_rad; ++j)
			g_sum += gaussian_kernel[j + k_rad] = k1 * expf(-k2 * (j * j));

		for (auto& val : gaussian_kernel)
			val /= g_sum;
	}

	GaussianFilter(int kernel_dimension) : m_kernel_dim(kernel_dimension) {
		assert(kernel_dimension % 2 == 1);
		int k_rad = (m_kernel_dim - 1) / 2;
		m_sigma = k_rad / 3.0348;

		gaussian_kernel = std::vector<float>(m_kernel_dim);

		float s = 2 * m_sigma * m_sigma;
		float k1 = 1 / sqrtf(M_PI * s), k2 = 1 / s;
		float g_sum = 0;

		for (int j = -k_rad; j <= k_rad; ++j)
			g_sum += gaussian_kernel[j + k_rad] = k1 * expf(-k2 * (j * j));

		for (auto& val : gaussian_kernel)
			val /= g_sum;
	}

	template<typename T>
	void ApplyGaussianBlur(Image<T>& img);

	template<typename T>
	void ApplyGaussianBlur(Image<T>& img, Image<T>& temp);

};

template<typename T>
void GaussianFilter::ApplyGaussianBlur(Image<T>& img) {

	Image<T> temp(img.Rows(), img.Cols(), img.Channels());

	for (int ch = 0; ch < img.Channels(); ++ch) {

#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			PixelWindow<T> window(m_kernel_dim);
			window.PopulateRowWindow(img, y, ch);
			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					window.UpdateRowWindow(img, x, y, ch);

				float sum = 0.0f;
				for (int j = 0; j < m_kernel_dim; ++j)
					sum += window[j] * gaussian_kernel[j];

				temp(x, y, ch) = sum;

			}
		}

#pragma omp parallel for 
		for (int x = 0; x < img.Cols(); ++x) {
			PixelWindow<T> window(m_kernel_dim);
			window.PopulateColWindow(temp, x, ch);
			for (int y = 0; y < img.Rows(); ++y) {

				if (y != 0)
					window.UpdateColWindow(temp, x, y, ch);

				float sum = 0.0f;
				for (int j = 0; j < m_kernel_dim; ++j)
					sum += window[j] * gaussian_kernel[j];

				img(x, y, ch) = sum;
			}
		}
	}
}

template<typename T>
void GaussianFilter::ApplyGaussianBlur(Image<T>& img, Image<T>& temp) {

	if (!temp.Matches(img))
		temp = Image<T>(img.Rows(), img.Cols(), img.Channels());

	for (int ch = 0; ch < img.Channels(); ++ch) {

#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			PixelWindow<T> window(m_kernel_dim);

			window.PopulateRowWindow(img, y, ch);
			for (int x = 0; x < img.Cols(); ++x) {

				if (x != 0)
					window.UpdateRowWindow(img, x, y, ch);

				float sum = 0.0f;
				for (int j = 0; j < m_kernel_dim; ++j)
					sum += window[j] * gaussian_kernel[j];

				temp(x, y, ch) = sum;

			}
		}

#pragma omp parallel for 
		for (int x = 0; x < img.Cols(); ++x) {
			PixelWindow<T> window(m_kernel_dim);
			window.PopulateColWindow(temp, x, ch);
			for (int y = 0; y < img.Rows(); ++y) {

				if (y != 0)
					window.UpdateColWindow(temp, x, y, ch);

				float sum = 0.0f;
				for (int j = 0; j < m_kernel_dim; ++j)
					sum += window[j] * gaussian_kernel[j];

				img(x, y, ch) = sum;
			}
		}
	}
}