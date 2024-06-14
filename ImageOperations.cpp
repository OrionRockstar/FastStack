#include "pch.h"
#include "ImageOperations.h"



void ImageOP::AlignFrame(Image32& img, Matrix& homography, Interpolator::Type interp_type) {
	//Interpolator interpolator;

	Image32 temp(img);
	temp.homography = img.homography;

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			double yx = y * homography(0, 1);
			double yy = y * homography(1, 1);
			for (int x = 0; x < img.Cols(); ++x) {
				double x_s = x * homography(0, 0) + yx + homography(0, 2);
				double y_s = x * homography(1, 0) + yy + homography(1, 2);

				temp(x, y, ch) = Interpolator().InterpolatePixel(img, x_s, y_s, ch, interp_type);//img.ClipPixel(interp_type(img, x_s, y_s, ch));

			}
		}
	}

	temp.MoveTo(img);
	//img.ComputeStatistics(true);
}

void ImageOP::AlignedStats(Image32& img, Matrix& homography, Interpolator::Type interp_type) {

	Interpolator interpolator;

	Image32 temp(img);

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {

			double yx = y * homography(0, 1);
			double yy = y * homography(1, 1);

			for (int x = 0; x < img.Cols(); ++x) {
				double x_s = x * homography(0, 0) + yx + homography(0, 2);
				double y_s = x * homography(1, 0) + yy + homography(1, 2);

				temp(x, y, ch) = interpolator.InterpolatePixel(img, x_s, y_s, ch, interp_type);

			}
		}
	}
	//temp.ComputeStatistics(true);
	//img.MoveStatsFrom(temp);
	
}


template<typename T>
void ImageOP::RotateImage(Image<T>& img, float theta, Interpolator::Type interp_type) {

	Interpolator interpolator;

	theta *= (M_PI / 180);
	float s = sin(theta);
	float c = cos(theta);
	Image<T> temp(fabs(img.Rows() * c) + fabs(img.Cols() * s), fabs(img.Cols() * c) + fabs(img.Rows() * s), img.Channels());

	float hc = temp.Cols() / 2;
	float hr = temp.Rows() / 2;

	float offsetx = hc - (temp.Cols() - img.Cols()) / 2;
	float offsety = hr - (temp.Rows() - img.Rows()) / 2;
	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.Rows(); ++y) {

			double yx = (y - hr) * s;
			double yy = (y - hr) * c;

			for (int x = 0; x < temp.Cols(); ++x) {

				double x_s = ((x - hc) * c - yx) + offsetx;
				double y_s = ((x - hc) * s + yy) + offsety;

				temp(x, y, ch) = interpolator.InterpolatePixel(img, x_s, y_s, ch, interp_type);

			}
		}
	}

	temp.MoveTo(img);
}
template void ImageOP::RotateImage(Image8&, float, Interpolator::Type);
template void ImageOP::RotateImage(Image16&, float, Interpolator::Type);
template void ImageOP::RotateImage(Image32&, float, Interpolator::Type);



template<typename T>
void ImageOP::ImageResize_Bicubic(Image<T>& img, int new_rows, int new_cols) {

	Interpolator interpolator;

	Image<T> temp(new_rows, new_cols, img.Channels());

	double ry = double(img.Rows()) / temp.Rows();
	double rx = double(img.Cols()) / temp.Cols();

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < temp.Rows(); ++y) {
			double y_s = y * ry;

			for (int x = 0; x < temp.Cols(); ++x) {
				double x_s = x * rx;

				temp(x, y, ch) = interpolator.InterpolatePixel(img, x_s, y_s, ch, Interpolator::Type::bicubic_spline);

			}
		}
	}
	img = std::move(temp);
}
template void ImageOP::ImageResize_Bicubic(Image8&, int, int);
template void ImageOP::ImageResize_Bicubic(Image16&, int, int);
template void ImageOP::ImageResize_Bicubic(Image32&, int, int);




template<typename Image>
void ImageOP::SobelEdge(Image& img) {

	std::array<int, 9> sx = { -1,0,1,
							  -2,0,2,
							  -1,0,1 };
	std::array<int, 9> sy = { -1,-2,-1,0,0,0,1,2,1 };

	Image temp(img.Rows(), img.Cols(), img.Channels());

	for (int ch = 0; ch < img.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < img.Rows(); ++y) {
			for (int x = 0; x < img.Cols(); ++x) {
				float sumx = 0, sumy = 0;

				for (int j = -1, el = 0; j < 2; ++j)
					for (int i = -1; i < 2; ++i) {
						if (img.IsInBounds(x + i, y + j)) {
							sumx += img(x + i, y + j, ch) * sx[el];
							sumy += img(x + i, y + j, ch) * sy[el];
						}

						else {
							sumx += img(x, y, ch) * sx[el];
							sumy += img(x, y, ch) * sy[el];
						}
						el++;
					}
				temp(x, y, ch) = sqrtf(sumx * sumx + sumy * sumy);
			}
		}
	}

	temp.CopyTo(img);
	img.Normalize();
}
template void ImageOP::SobelEdge(Image8&);
template void ImageOP::SobelEdge(Image16&);
template void ImageOP::SobelEdge(Image32&);


template<typename Image>
void ImageOP::PowerofInvertedPixels(Image& img, float order, bool lightness_mask) {

#pragma omp parallel for
	for (int el = 0; el < img.PxCount(); ++el)
		img[el] = (lightness_mask) ? (double(img[el]) * img[el]) + (1 - img[el]) * pow(img[el], pow(1 - img[el], order)) : pow(img[el], pow(1 - img[el], order));
}
template void ImageOP::PowerofInvertedPixels(Image8&, float, bool);
template void ImageOP::PowerofInvertedPixels(Image16&, float, bool);
template void ImageOP::PowerofInvertedPixels(Image32&, float, bool);

