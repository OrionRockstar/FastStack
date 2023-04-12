#pragma once
#include "Image.h"
#include "Interpolator.h"
#include "ImageOperations.h"

class Drizzle
{

	static void SigmaClipWM(ImageVector& imgvec, float l_sigma, float u_sigma);

	static float DrizzlePix(float inp, float out, float area, float s2, int pix_weight, int out_weight);

	static void DrizzleFrame(Image32& input, Image32& output, float drop);

	public:
	static void DrizzleImageStack(std::vector<std::filesystem::path> light_files, Image32& output, float drop_size, ScaleEstimator scale_est, bool reject);

};

