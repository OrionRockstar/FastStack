#pragma once
#include "Image.h"

namespace ImageStacking
{
	void Average(std::vector<Image32> &imgvec, Image32& final_image);

	void Median(std::vector<Image32> &imgvec, Image32& final_image);

	void SigmaClipping(std::vector<Image32> &imgvec, Image32& final_image, float l_sigma, float u_sigma);

	void KappaSigmaClipping(std::vector<Image32> &imgvec, Image32& final_image, float l_sigma, float u_sigma);

	void WinsorizedSigmaClipping(std::vector<Image32> &imgvec, Image32& final_image, float l_sigma, float u_sigma);

	void StackImages(std::vector<Image32>& imgvec, Image32& final_image, int stack_type, float l_sigma, float u_sigma);
};

