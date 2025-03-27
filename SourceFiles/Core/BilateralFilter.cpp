#include "pch.h"
#include "FastStack.h"
#include "BilateralFilter.h"

BilateralFilter::FloatKernel BilateralFilter::generateGausianKernel(size_t dim, float sigma) {

	FloatKernel gaussian(dim);
	float k_s = 1 / (2 * sigma * sigma);
	int rad = (dim - 1) / 2;

	for (int j = -rad; j <= rad; ++j)
		for (int i = -rad; i <= rad; ++i)
			gaussian(i + rad, j + rad) = -k_s * ((j * j) + (i * i));

	return std::move(gaussian);
}

std::vector<bool> BilateralFilter::generateMask(size_t dim, bool circular) {
	std::vector<bool> mask(dim * dim, true);

	if (circular) {
		int r = (dim - 1) / 2;
		for (int j = 0; j < dim; ++j) {
			int dy2 = j - r;
			dy2 *= dy2;
			for (int i = 0; i < dim; ++i) {
				int dx = i - r;
				if (sqrt(dx * dx + dy2) > r)
					mask[j * dim + i] = false;
			}
		}
	}

	return mask;
}

template<typename T>
void BilateralFilter::apply(Image<T>& img) {

	Image<T> temp(img.rows(), img.cols(), img.channels());

	FloatKernel gaussian = generateGausianKernel(m_kernel_dim, m_sigma_s);
	FloatKernel kernel(m_kernel_dim);
	auto kmask = generateMask(m_kernel_dim, m_is_circular);

	float k_r = 1 / (2 * m_sigma_r * m_sigma_r);

	//uint32_t c = 0;

	for (uint32_t ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for firstprivate(kernel, kmask)
		for (int y = 0; y < img.rows(); ++y) {

			kernel.populate(img, { 0,y,ch });

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.update(img, { x,y,ch });

				float sum = 0.0f;
				float weight = 0.0f;
				float pixel = Pixel<float>::toType(img(x, y, ch));

				for (int el = 0; el < kernel.count(); ++el) {
					if (kmask[el]) {
						float d = pixel - kernel[el];
						float d2 = d * d;
						float w = exp(gaussian[el] - (k_r * (d2 + d2)));
						weight += w;
						sum += w * kernel[el];
					}
				}

				temp(x, y, ch) = Pixel<T>::toType(sum / weight);

			}
//#pragma omp atomic
			//++c;

			//if (omp_get_thread_num() == 0)
				//m_ps->emitProgress((c * 100) / (img.rows() * img.channels()));
		}
	}

	//m_ps->emitProgress(100);

	temp.moveTo(img);
}
template void BilateralFilter::apply(Image8&);
template void BilateralFilter::apply(Image16&);
template void BilateralFilter::apply(Image32&);

template<typename T>
void BilateralFilter::applyTo(const Image<T>& src, Image<T>& dst, float scale_factor, const QPointF& p) {

	FloatKernel gaussian = generateGausianKernel(m_kernel_dim, m_sigma_s);
	FloatKernel kernel(m_kernel_dim);
	auto kmask = generateMask(m_kernel_dim, m_is_circular);

	float k_r = 1 / (2 * m_sigma_r * m_sigma_r);
	float _s = 1 / scale_factor;

	for (uint32_t ch = 0; ch < dst.channels(); ++ch) {
#pragma omp parallel for firstprivate(kernel), num_threads(4)
		for (int y = 0; y < dst.rows(); ++y) {
			int y_s = y * _s + p.y();
			int o_x = -1;
			float value = 0.0f;
			for (int x = 0; x < dst.cols(); ++x) {
				int x_s = x * _s + p.x();

				if (x_s != o_x) {
					kernel.populate(src, { x_s,y_s,ch });

					float sum = 0.0f;
					float weight = 0.0f;
					float pixel = Pixel<float>::toType(src(x_s, y_s, ch));

					for (int el = 0; el < kernel.count(); ++el) {
						if (kmask[el]) {
							float d = pixel - kernel[el];
							float d2 = d * d;
							float w = exp(gaussian[el] - (k_r * (d2 + d2)));
							weight += w;
							sum += w * kernel[el];
						}
					}

					value = dst(x, y, ch) = Pixel<T>::toType(sum / weight);
				}
				else
					dst(x, y, ch) = value;

				o_x = x_s;
			}
		}
	}
}
template void BilateralFilter::applyTo(const Image8&, Image8&, float, const QPointF&);
template void BilateralFilter::applyTo(const Image16&, Image16&, float, const QPointF&);
template void BilateralFilter::applyTo(const Image32&, Image32&, float, const QPointF&);

