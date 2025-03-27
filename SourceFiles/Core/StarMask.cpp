#include "pch.h"
#include "FastStack.h"
#include "StarMask.h"
#include "GaussianFilter.h"

template<typename T>
Image<T> StarMask::generateStarMask(const Image<T>& src) {

	StarVector sv = m_sd.applyStarDetection(src);

    Image<T> mask(src.rows(), src.cols());

    for (auto star : sv) {
        int xc = star.xc;
        int yc = star.yc;
        int r = star.radius;
        for (int j = yc - r; j <= yc + r; ++j)
            for (int i = xc - r; i <= xc + r; ++i)
                if (mask.isInBounds(i, j) && math::distance(star.xc, star.yc, i, j) <= star.radius)
                    mask(i, j) = (m_real_value) ? src(i, j) : Pixel<T>::max();
    }

    GaussianFilter(m_stddev).apply(mask);
	return mask;
}
template Image8 StarMask::generateStarMask(const Image8&);
template Image16 StarMask::generateStarMask(const Image16&);
template Image32 StarMask::generateStarMask(const Image32&);

