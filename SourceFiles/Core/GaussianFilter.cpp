#include "pch.h"
#include "GaussianFilter.h"
#include "FastStack.h"

using GF = GaussianFilter;

template<typename T>
void GF::PW::populateRowWindow(const Image<T>& img, const ImagePoint& p) {

	for (int j = 0; j < m_size; ++j) {
		int xx = j - m_radius;

		if (xx < 0)
			xx = -xx;
		else if (xx >= img.cols())
			xx = 2 * img.cols() - (xx + 1);


		data[j] = Pixel<float>::toType(img.at(xx, p.y(), p.channel()));
	}
}
template void GF::PW::populateRowWindow(const Image8&, const ImagePoint&);
template void GF::PW::populateRowWindow(const Image16&, const ImagePoint&);
template void GF::PW::populateRowWindow(const Image32&, const ImagePoint&);

template<typename T>
void GF::PW::updateRowWindow(const Image<T>& img, const ImagePoint& p) {

	data.erase(data.begin());

	int xx = p.x() + m_radius;

	if (xx >= img.cols())
		xx = 2 * img.cols() - (xx + 1);

	data.emplace_back(Pixel<float>::toType(img.at(xx, p.y(), p.channel())));
}
template void GF::PW::updateRowWindow(const Image8&, const ImagePoint&);
template void GF::PW::updateRowWindow(const Image16&, const ImagePoint&);
template void GF::PW::updateRowWindow(const Image32&, const ImagePoint&);

template<typename T>
void GF::PW::populateColWindow(const Image<T>& img, const ImagePoint& p) {

	for (int j = 0; j < m_size; ++j) {
		int yy = j - m_radius;

		if (yy < 0)
			yy = -yy;

		else if (yy >= img.rows())
			yy = 2 * img.rows() - (yy + 1);

		data[j] = Pixel<float>::toType(img.at(p.x(), yy, p.channel()));
	}
}
template void GF::PW::populateColWindow(const Image8&, const ImagePoint&);
template void GF::PW::populateColWindow(const Image16&, const ImagePoint&);
template void GF::PW::populateColWindow(const Image32&, const ImagePoint&);

template<typename T>
void GF::PW::updateColWindow(const Image<T>& img, const ImagePoint& p) {

	data.erase(data.begin());

	int yy = p.y() + m_radius;

	if (yy >= img.rows())
		yy = 2 * img.rows() - (yy + 1);

	data.emplace_back(Pixel<float>::toType(img.at(p.x(), yy, p.channel())));
}
template void GF::PW::updateColWindow(const Image8&, const ImagePoint&);
template void GF::PW::updateColWindow(const Image16&, const ImagePoint&);
template void GF::PW::updateColWindow(const Image32&, const ImagePoint&);

GaussianFilter::GaussianFilter(float sigma) : m_sigma(sigma) {
	int k_rad = (3.0348 * m_sigma) + 0.5;
	m_kernel_dim = 2 * k_rad + 1;
}

void GaussianFilter::setSigma(float sigma) {
	m_sigma = sigma;
	int k_rad = (3.0348 * m_sigma) + 0.5;
	m_kernel_dim = 2 * k_rad + 1;
}

/*void GaussianFilter::setKernelDimension(int kernel_dimension) {
	m_kernel_dim = kernel_dimension;
	int k_rad = (m_kernel_dim - 1) / 2;
	m_sigma = k_rad / 3.0348;
}*/

std::vector<float> GaussianFilter::buildGaussianKernel_1D(uint32_t size, float sigma)const {

	int k_rad = (size - 1) / 2;

	std::vector<float> kernel(size);

	float s = 2 * sigma * sigma;
	float k1 = 1 / sqrtf(std::_Pi * s), k2 = 1 / s;
	float g_sum = 0;

	for (int j = -k_rad; j <= k_rad; ++j)
		g_sum += kernel[j + k_rad] = k1 * expf(-k2 * (j * j));

	for (auto& val : kernel)
		val /= g_sum;

	return kernel;
}

template<typename T>
void GaussianFilter::apply(Image<T>& img) {

	Image32 temp(img.rows(), img.cols());

	std::vector<float> gaussian_kernel = buildGaussianKernel_1D(m_kernel_dim, m_sigma);

	PixelWindow window(m_kernel_dim);

	for (uint32_t ch = 0; ch < img.channels(); ++ch) {

#pragma omp parallel for firstprivate(window)
		for (int y = 0; y < img.rows(); ++y) {
			window.populateRowWindow(img, { 0,y,ch });
			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					window.updateRowWindow(img, { x,y,ch });

				float sum = 0.0f;

				for (int j = 0; j < m_kernel_dim; ++j)
					sum += window[j] * gaussian_kernel[j];

				temp(x, y) = sum;

			}
		}

#pragma omp parallel for firstprivate(window)
		for (int x = 0; x < img.cols(); ++x) {
			window.populateColWindow(temp, { x,0,0 });
			for (int y = 0; y < img.rows(); ++y) {

				if (y != 0)
					window.updateColWindow(temp, { x,y,0 });

				float sum = 0.0f;

				for (int j = 0; j < m_kernel_dim; ++j)
					sum += window[j] * gaussian_kernel[j];

				img(x, y, ch) = Pixel<T>::toType(sum);
			}
		}
	}

	img.normalize();
}
template void GaussianFilter::apply(Image8&);
template void GaussianFilter::apply(Image16&);
template void GaussianFilter::apply(Image32&);

