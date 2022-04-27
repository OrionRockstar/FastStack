#pragma once
#include "Image-Math.h"
#include <numeric>
#include <algorithm>
#include<experimental/vector>

class ImageStacking
{
public:
	struct ImagePtr{
		cv::Mat img;
		unsigned short* iptr;
	};
	void Average(std::vector<ImageStacking::ImagePtr> imgvec, cv::Mat final_image);

	void Median(std::vector<ImageStacking::ImagePtr> imgvec, cv::Mat final_image);

	void SigmaClipping(std::vector<ImagePtr> imgvec, cv::Mat final_image, double l_sigma, double u_sigma);

	void KappaSigmaClipping(std::vector<ImageStacking::ImagePtr> imgvec, cv::Mat final_image, double l_sigma, double u_sigma);

	void WinsorizedSigmaClipping(std::vector<ImageStacking::ImagePtr> imgvec, cv::Mat final_image, double l_sigma, double u_sigma);
};

