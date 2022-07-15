#pragma once
#include "Image.h"
#include <numeric>
#include <algorithm>
#include<experimental/vector>

namespace ImageStacking
{
	void Average(std::vector<Image32> &imgvec, Image32& final_image);

	void Median(std::vector<Image32> &imgvec, Image32& final_image);

	void SigmaClipping(std::vector<Image32> &imgvec, Image32& final_image, double l_sigma, double u_sigma);

	void KappaSigmaClipping(std::vector<Image32> &imgvec, Image32& final_image, double l_sigma, double u_sigma);

	void WinsorizedSigmaClipping(std::vector<Image32> &imgvec, Image32& final_image, double l_sigma, double u_sigma);
};

