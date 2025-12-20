#include "pch.h"
#include "ColorSaturation.h"
#include "RGBColorSpace.h"
#include "FastStack.h"

static void shiftHue(double& H, double amount) {

	H += amount;

	if (H >= 1)
		H -= 1;

	else if (H < 0)
		H += 1;
}

static double scalingFactor(double k) {
	k += (k < 0) ? -1 : +1;

	if (k < 0)
		k = 1 / (-k);

	return k;
}

template<typename T>
void ColorSaturation::apply(Image<T>& img) {

    if (!img.exists() || img.channels() == 1)
        return;

#pragma omp parallel for
    for (int y = 0; y < img.rows(); ++y) {
		for (int x = 0; x < img.cols(); ++x) {

			auto rgb = img.template color<double>(x, y);

			double H, S, V, L;
			ColorSpace::RGBtoHSVL(rgb, H, S, V, L);

			shiftHue(H, m_hue_shift);
			double k = scalingFactor(m_scale * m_saturation_curve.interpolate(H));
			shiftHue(H, -m_hue_shift);

			img.template setColor<>(x, y, ColorSpace::HSVLtoRGB(H, math::clip(S * k), V, L));
		}
    }
}
template void ColorSaturation::apply(Image8&);
template void ColorSaturation::apply(Image16&);
template void ColorSaturation::apply(Image32&);

