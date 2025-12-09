#include "pch.h"
#include "FastStack.h"
#include "BilateralFilter.h"
#include "MorphologicalTransformation.h"

static std::vector<float> generateGausianKernel(size_t dim, float sigma) {

	std::vector<float> gaussian(dim * dim);
	float k_s = 1 / (2 * sigma * sigma);
	int rad = (dim - 1) / 2;

	for (int j = -rad; j <= rad; ++j)
		for (int i = -rad; i <= rad; ++i)
			gaussian[(j + rad) * dim + i + rad] = -k_s * ((j * j) + (i * i));

	return std::move(gaussian);
}

static std::vector<uint8_t> generateMask(size_t dim, bool circular) {
	std::vector<uint8_t> mask(dim * dim, true);

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

	auto gaussian = generateGausianKernel(m_kernel_dim, m_sigma_s);
	Kernel<T> kernel(img, m_kernel_dim);
	auto kmask = generateMask(m_kernel_dim, m_is_circular);

	float k_r = 1 / (2 * m_sigma_r * m_sigma_r);

	for (uint32_t ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for firstprivate(kernel, k_r)
		for (int y = 0; y < img.rows(); ++y) {

			kernel.populate(y, ch);

			for (int x = 0; x < img.cols(); ++x) {

				if (x != 0)
					kernel.update(x, y, ch);

				float sum = 0.0f;
				float weight = 0.0f;
				float pixel = Pixel<float>::toType(img(x, y, ch));

				for (int el = 0; el < kernel.count(); ++el) {
					if (kmask[el]) {
						float p = Pixel<float>::toType(kernel[el]);
						float d = pixel - p;
						float d2 = d * d;
						float w = exp(gaussian[el] - (k_r * (d2 + d2)));
						weight += w;
						sum += w * p;
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
void BilateralFilter::applyTo(const Image<T>& src, Image<T>& dst, float scale_factor, const QRectF& r) {

	auto gaussian = generateGausianKernel(m_kernel_dim, m_sigma_s);
	std::vector<uint8_t> kmask(m_kernel_dim * m_kernel_dim, true);

	if (dst.cols() != uint32_t(r.width() * scale_factor) || dst.rows() != uint32_t(r.height() * scale_factor) || dst.channels() != src.channels()) 
		dst = Image<T>(r.height() * scale_factor, r.width() * scale_factor, src.channels());

	int kernel_rad = (m_kernel_dim - 1) / 2;
	float k_r = 1 / (2 * m_sigma_r * m_sigma_r);
	float _s = 1 / scale_factor;

	for (uint32_t ch = 0; ch < dst.channels(); ++ch) {
#pragma omp parallel for firstprivate(kernel_rad, _s, k_r) num_threads(4)
		for (int y = 0; y < dst.rows(); ++y) {
			int y_s = y * _s + r.y();
			int o_x = -1;
			float value = 0.0f;
			for (int x = 0; x < dst.cols(); ++x) {
				int x_s = x * _s + r.x();

				if (x_s != o_x) {

					float sum = 0.0f;
					float weight = 0.0f;
					float pixel = Pixel<float>::toType(src(x_s, y_s, ch));

					for (int j = -kernel_rad, el = 0; j <= kernel_rad; ++j) {
						for (int i = -kernel_rad; i <= kernel_rad; ++i, ++el) {
							if (kmask[el]) {
								float p = Pixel<float>::toType(src.at_mirrored(x_s + i, y_s + j, ch));
								float d = pixel - p;
								float d2 = d * d;
								float w = exp(gaussian[el] - (k_r * (d2 + d2)));
								weight += w;
								sum += w * p;
							}
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
template void BilateralFilter::applyTo(const Image8&, Image8&, float, const QRectF&);
template void BilateralFilter::applyTo(const Image16&, Image16&, float, const QRectF&);
template void BilateralFilter::applyTo(const Image32&, Image32&, float, const QRectF&);
