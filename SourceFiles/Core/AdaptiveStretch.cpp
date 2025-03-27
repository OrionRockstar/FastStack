#include "pch.h"
#include "AdaptiveStretch.h"
#include "FastStack.h"

template<typename T>
static Image<T> RGBtoL(const Image<T>& src) {

	if (src.channels() == 1)
		return src;

	Image<T> dst(src.rows(), src.cols());

	for (int y = 0; y < src.rows(); ++y)
		for (int x = 0; x < src.cols(); ++x) 
			dst(x, y) = ColorSpace::HSL_L(src.color(x, y));

	return dst;
}

template<typename T>
void AdaptiveStretch::computeCDF(const Image<T>& img) {

	if (img.channels() == 3) {
		Image<T> temp = RGBtoL(img);
		return computeCDF(temp);
	}

	std::vector<uint32_t> pos;
	std::vector<uint32_t> neg;

	int multiplier = (isFloatImage(img)) ? m_data_points - 1 : 1;
	int size = [&]() {
		switch (img.type()) {
		case ImageType::UBYTE:
			return 256;
		case ImageType::USHORT:
			return 65536;
		case ImageType::FLOAT:
			return m_data_points;
		default:
			return 256;
		}
	}();

	pos.resize(size, 0);
	neg.resize(size, 0);
	m_cdf_curve.resize(size);	

	T noise = Pixel<T>::toType(m_noise_thresh);
	float contrast = (m_contrast_protection) ? m_contrast_threshold : 0;

#pragma omp parallel for num_threads(4)
	for (int y = 0; y < img.rows() - 1; ++y) {
		for (int x = 0; x < img.cols() - 1; ++x) {

			T a0 = img(x, y);

			T a1 = img(x + 1, y);
			uint32_t l1 = math::min(a0, a1) * multiplier;

			T a2 = img(x - 1, y + 1);
			uint32_t l2 = math::min(a0, a2) * multiplier;

			T a3 = img(x, y + 1);
			uint32_t l3 = math::min(a0, a3) * multiplier;

			T a4 = img(x + 1, y + 1);
			uint32_t l4 = math::min(a0, a4) * multiplier;

			if (abs(a0 - a1) > noise)
				pos[l1]++;
			else
				neg[l1]++;

			if (abs(a0 - a2) > noise)
				pos[l2]++;
			else
				neg[l2]++;

			if (abs(a0 - a3) > noise)
				pos[l3]++;
			else
				neg[l3]++;

			if (abs(a0 - a4) > noise)
				pos[l4]++;
			else
				neg[l4]++;
		}
	}

	m_cdf_curve[0] = pos[0] - contrast * neg[0];
	for (int i = 1; i < m_cdf_curve.size(); ++i)
		m_cdf_curve[i] = pos[i] - contrast * neg[i] + m_cdf_curve[i - 1];

	T max = img.computeMax(0);
	T min = img.computeMin(0);

	int bm = min * multiplier, bM = max * multiplier;
	double mindiff = 0;

	//find "greatest" net_cdf decrease
	for (int k = bm; k < bM; ++k) {
		float d = m_cdf_curve[k + 1] - m_cdf_curve[k];
		if (d < mindiff)
			mindiff = d;
	}

	if (mindiff < 0) {
		mindiff = -mindiff;
		for (int k = bm; k <= bM; ++k)
			m_cdf_curve[k] += (k - bm) * (mindiff + 1 / (bM - bm));
	}

	m_cdf_curve.setMin(m_cdf_curve[bm]);
	m_cdf_curve.setMax(m_cdf_curve[bM]);
}
template void AdaptiveStretch::computeCDF(const Image8& img);
template void AdaptiveStretch::computeCDF(const Image16& img);
template void AdaptiveStretch::computeCDF(const Image32& img);


template <typename T>
void AdaptiveStretch::apply(Image<T>& img) {

	computeCDF(img);

	int multiplier = (img.type() == ImageType::FLOAT) ? m_data_points - 1 : 1;

	float n = 1.0 / (m_cdf_curve.max() - m_cdf_curve.min());

	for (auto& v : img)
		v = Pixel<T>::toType((m_cdf_curve[v * multiplier] - m_cdf_curve.min()) * n);

}
template void AdaptiveStretch::apply(Image8&);
template void AdaptiveStretch::apply(Image16&);
template void AdaptiveStretch::apply(Image32&);

template <typename T>
void AdaptiveStretch::apply_NoCDF(Image<T>& img) {

	int multiplier = (isFloatImage(img)) ? m_data_points - 1 : 1;

	float n = 1.0 / (m_cdf_curve.max() - m_cdf_curve.min());

	for (auto& v : img)
		v = Pixel<T>::toType((m_cdf_curve[v * multiplier] - m_cdf_curve.min()) * n);
}
template void AdaptiveStretch::apply_NoCDF(Image8&);
template void AdaptiveStretch::apply_NoCDF(Image16&);
template void AdaptiveStretch::apply_NoCDF(Image32&);
