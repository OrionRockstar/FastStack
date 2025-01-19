#include "pch.h"
#include "ImageOperations.h"



void ImageOP::AlignFrame(Image32& img, Matrix& homography, Interpolator::Type interp_type) {
	//Interpolator interpolator;

	Image32 temp(img);
   //temp.homography = img.homography;

	for (uint32_t ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {

			double yx = y * homography(0, 1);
			double yy = y * homography(1, 1);
			for (int x = 0; x < img.cols(); ++x) {
				double x_s = x * homography(0, 0) + yx + homography(0, 2);
				double y_s = x * homography(1, 0) + yy + homography(1, 2);

				//temp(x, y, ch) = Interpolator().InterpolatePixel(img, { x_s, y_s, ch }, interp_type);//img.ClipPixel(interp_type(img, x_s, y_s, ch));
			}
		}
	}

	temp.moveTo(img);
	//img.ComputeStatistics(true);
}

void ImageOP::AlignedStats(Image32& img, Matrix& homography, Interpolator::Type interp_type) {

	//Interpolator interpolator;

	Image32 temp(img);

	for (int ch = 0; ch < img.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.rows(); ++y) {

			double yx = y * homography(0, 1);
			double yy = y * homography(1, 1);

			for (int x = 0; x < img.cols(); ++x) {
				double x_s = x * homography(0, 0) + yx + homography(0, 2);
				double y_s = x * homography(1, 0) + yy + homography(1, 2);

				//temp(x, y, ch) = interpolator.InterpolatePixel(img, { x_s, y_s, ch }, interp_type);
			}
		}
	}
	//temp.ComputeStatistics(true);
	//img.MoveStatsFrom(temp);
	
}


template<typename Image>
void ImageOP::PowerofInvertedPixels(Image& img, float order, bool lightness_mask) {

#pragma omp parallel for
	for (int el = 0; el < img.pxCount(); ++el)
		img[el] = (lightness_mask) ? (double(img[el]) * img[el]) + (1 - img[el]) * pow(img[el], pow(1 - img[el], order)) : pow(img[el], pow(1 - img[el], order));
}
template void ImageOP::PowerofInvertedPixels(Image8&, float, bool);
template void ImageOP::PowerofInvertedPixels(Image16&, float, bool);
template void ImageOP::PowerofInvertedPixels(Image32&, float, bool);

