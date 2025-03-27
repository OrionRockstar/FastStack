#include "pch.h"
#include "RangeMask.h"
#include "GaussianFilter.h"

template <typename T>
Image<T> RangeMask::generateMask(const Image<T>& img) {

    int ch = (m_lightness && img.channels() == 3) ? 1 : img.channels();

    Image<T> mask(img.rows(), img.cols(), ch);

    float z = (m_high - m_low) * m_fuzziness * 0.5;

    float a_p = m_low + z;
    float b_p = m_high - z;

    float base_a = m_low;
    float base_b = b_p;

#pragma omp parallel for num_threads(mask.channels())
	for (int ch = 0; ch < mask.channels(); ++ch) {
		for (int y = 0; y < mask.rows(); ++y) {
			for (int x = 0; x < mask.cols(); ++x) {
				float pixel = 0;

				if (m_lightness && img.channels() == 3)
					pixel = ColorSpace::CIEL(img.color<double>(x, y));

				else
					pixel = Pixel<float>::toType(img(x, y, ch));


				if (m_low <= pixel && pixel <= m_high) {
					if (pixel < a_p)
						pixel = (pixel - base_a) / z;
					else if (pixel > b_p)
						pixel = 1 - (pixel - base_b) / z;
					else
						pixel = 1;
				}

				else
					pixel = 0;

				if (m_screening)
					pixel *= pixel;

				if (m_invert)
					pixel = 1 - pixel;

				mask(x, y, ch) = Pixel<T>::toType(pixel);
			}
		}
    }

    if (m_smoothness != 0.0)
        GaussianFilter(m_smoothness).apply(mask);

    return mask;
}
template Image8 RangeMask::generateMask(const Image8&);
template Image16 RangeMask::generateMask(const Image16&);
template Image32 RangeMask::generateMask(const Image32&);

template <typename T>
void RangeMask::generateMask_overwrite(Image<T>& img) {

	int ch = (m_lightness && img.channels() == 3) ? 1 : img.channels();

	Image<T> mask(img.rows(), img.cols(), ch);

	float z = (m_high - m_low) * m_fuzziness * 0.5;

	float a_p = m_low + z;
	float b_p = m_high - z;

	float base_a = m_low;
	float base_b = b_p;

#pragma omp parallel for num_threads(mask.channels())
	for (int ch = 0; ch < mask.channels(); ++ch) {
		for (int y = 0; y < mask.rows(); ++y) {
			for (int x = 0; x < mask.cols(); ++x) {
				float pixel = 0;

				if (m_lightness && img.channels() == 3)
					pixel = ColorSpace::CIEL(img.color<double>(x, y));

				else
					pixel = Pixel<float>::toType(img(x, y, ch));


				if (m_low <= pixel && pixel <= m_high) {
					if (pixel < a_p)
						pixel = (pixel - base_a) / z;
					else if (pixel > b_p)
						pixel = 1 - (pixel - base_b) / z;
					else
						pixel = 1;
				}

				else
					pixel = 0;

				if (m_screening)
					pixel *= pixel;

				if (m_invert)
					pixel = 1 - pixel;

				mask(x, y, ch) = Pixel<T>::toType(pixel);
			}
		}
	}

	if (m_smoothness != 0.0)
		GaussianFilter(m_smoothness).apply(mask);

	mask.moveTo(img);
}
template void RangeMask::generateMask_overwrite(Image8&);
template void RangeMask::generateMask_overwrite(Image16&);
template void RangeMask::generateMask_overwrite(Image32&);
