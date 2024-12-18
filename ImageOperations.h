#pragma once
#include "Image.h"
#include "Interpolator.h"

namespace ImageOP {

	void AlignFrame(Image32& img, Matrix& homography, Interpolator::Type interp_type);

	void AlignedStats(Image32& img, Matrix& homography, Interpolator::Type interp_type);

	template<typename Image>
	extern void PowerofInvertedPixels(Image& img, float order, bool lightness_mask = true);

}