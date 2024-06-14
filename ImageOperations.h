#pragma once
#include "Image.h"
#include "Interpolator.h"

namespace ImageOP {

	void AlignFrame(Image32& img, Matrix& homography, Interpolator::Type interp_type);

	void AlignedStats(Image32& img, Matrix& homography, Interpolator::Type interp_type);


	template<typename T>
	extern void RotateImage(Image<T>& img, float theta_degrees, Interpolator::Type interp_type);


	template<typename T>
	extern void ImageResize_Bicubic(Image<T>& img, int new_rows, int new_cols);

	template<typename Image>
	extern void SobelEdge(Image& img);

	template<typename Image>
	extern void PowerofInvertedPixels(Image& img, float order, bool lightness_mask = true);

}