#pragma once
#include "Image.h"
#include <numeric>
#include <algorithm>
#include<experimental/vector>

class ImageStacking
{
public:
	
	void Average(std::vector<Image> &imgvec, Image& final_image);

	void Median(std::vector<Image> &imgvec, Image& final_image);

	void SigmaClipping(std::vector<Image> &imgvec, Image& final_image, double l_sigma, double u_sigma);

	void KappaSigmaClipping(std::vector<Image> &imgvec, Image& final_image, double l_sigma, double u_sigma);

	void WinsorizedSigmaClipping(std::vector<Image> &imgvec, Image& final_image, double l_sigma, double u_sigma);
};

