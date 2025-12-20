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
	uint32_t K = size - 1;

	pos.resize(size, 0);
	neg.resize(size, 0);
	m_cdf_curve.resize(size);	

	float thresh = m_noise_thresh;
	float contrast = (m_contrast_protection) ? m_contrast_threshold : 0;

	auto tp = getTimePoint();
#pragma omp parallel for firstprivate(K, thresh) num_threads(4)
	for (int y = 0; y < img.rows(); ++y) {
		for (int x = 0; x < img.cols(); ++x) {

			float a0 = img.template pixel<float>(x, y);

			float a1 = Pixel<float>::toType(img.at_replicated(x + 1, y));
			uint32_t l1 = math::min(a0, a1) * K;

			float a2 = Pixel<float>::toType(img.at_replicated(x - 1, y + 1));
			uint32_t l2 = math::min(a0, a2) * K;

			float a3 = Pixel<float>::toType(img.at_replicated(x, y + 1));
			uint32_t l3 = math::min(a0, a3) * K;

			float a4 = Pixel<float>::toType(img.at_replicated(x + 1, y + 1));
			uint32_t l4 = math::min(a0, a4) * K;

			if (abs(a0 - a1) > thresh)
				pos[l1]++;
			else
				neg[l1]++;

			if (abs(a0 - a2) > thresh)
				pos[l2]++;
			else
				neg[l2]++;

			if (abs(a0 - a3) > thresh)
				pos[l3]++;
			else
				neg[l3]++;

			if (abs(a0 - a4) > thresh)
				pos[l4]++;
			else
				neg[l4]++;
		}
	}
	displayTimeDuration(tp);
	m_cdf_curve[0] = pos[0] - contrast * neg[0];
	for (int i = 1; i < m_cdf_curve.size(); ++i)
		m_cdf_curve[i] = pos[i] - contrast * neg[i] + m_cdf_curve[i - 1];

	T min, max;
	img.computeMinMax(min, max, 0);

	int bm = min * K, bM = max * K;
	double mindiff = 0;

	//find "greatest" net_cdf decrease
	for (int i = bm; i < bM; ++i) {
		float d = m_cdf_curve[i + 1] - m_cdf_curve[i];
		if (d < mindiff)
			mindiff = d;
	}

	if (mindiff < 0) {
		mindiff = -mindiff;
		for (int i = bm; i <= bM; ++i)
			m_cdf_curve[i] += (i - bm) * (mindiff + 1 / (bM - bm));
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
