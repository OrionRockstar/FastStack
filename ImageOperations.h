#pragma once
#include "Image.h"
#include "Interpolator.h"

namespace ImageOP {

	void AlignFrame(Image32& img, Eigen::Matrix3d homography, Interpolation_Type interp_type);

	void AlignedStats(Image32& img, Eigen::Matrix3d& homography, Interpolation_Type interp_type);

	void AlignImageStack(ImageVector& img_stack, Interpolation_Type interp_type);


	void DrizzleImageStack(std::vector<std::filesystem::path> light_files, Image32& output, float drop_size, ScaleEstimator scale_estimator);

	template<typename Image>
	extern void RotateImage(Image& img, float theta_degrees, Interpolation_Type interp_type);

	template<typename Image>
	extern void FastRotation(Image& img, FastRotate type);

	void Crop(Image32& img, int top, int bottom, int left, int right);

	template<typename Image>
	extern void ImageResize_Bicubic(Image& img, int new_rows, int new_cols);

	template<typename Image>
	extern void BinImage(Image& img, int factor, int method);

	template<typename Image>
	extern void UpsampleImage(Image& img, int factor);


	template<typename Image>
	extern void SobelEdge(Image& img);

	template<typename T>
	extern void MedianBlur3x3(Image<T>& img);

	template<typename T>
	extern void MedianBlur5x5(Image<T>& img);

	template<typename Image>
	extern void GaussianBlur(Image& img, float std_dev = 1.0f);

	template<typename T>
	extern void BilateralFilter(Image<T>& img, float std_dev, float std_dev_range);


	void B3WaveletTransform(Image32& img, ImageVector& wavelet_vector, int scale = 5);

	void B3WaveletTransformTrinerized(Image32& img, Image8Vector& wavelet_vector, float thresh, int scale_num = 5);

	void B3WaveletLayerNoiseReduction(Image32& img, int scale_num = 4);


	void ScaleImage(Image32& ref, Image32& tgt, ScaleEstimator type);

	void ScaleImageStack(ImageVector& img_stack, ScaleEstimator type);


	void STFImageStretch(Image32& img);

	void ASinhStretch(Image32& img, float stretch_factor);

	template<typename Image>
	extern void HistogramTransformation(Image& img, float shadow = 0.0f, float midtone = 0.5f, float highlight = 1.0f);

	template<typename Image>
	extern void PowerofInvertedPixels(Image& img, float order, bool lightness_mask = true);

	template<typename Image>
	extern void AdaptiveStretch(Image& img, float thresh_coef = 1.0f, int thresh_exp = -3, float contrast_coef = 0.0f, int contrast_exp = -2, int num_data_points = 1'000'000);

	template<typename Image>
	extern void LocalHistogramEqualization(Image& img, int kernel_radius = 64, float contrast_limit = 1.5f, float amount = 1.0f, bool circular = false, int hist_res = 8);
}