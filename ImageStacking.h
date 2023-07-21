#pragma once
#include "Image.h"

enum class Integration {
	average,
	median,
	min,
	max
};

enum class Rejection {
	none,
	sigma_clip,
	winsorized_sigma_clip
};

enum class ScaleEstimator {
	median,
	avgdev,
	mad,
	bwmv,
	none
};


class ImageStacking {

	ScaleEstimator m_scale_est = ScaleEstimator::none;

	Integration m_integration = Integration::average;

	Rejection m_rejection = Rejection::none;
	float m_l_sigma = 2;
	float m_u_sigma = 3;

	void ScaleImage(Image32& ref, Image32& tgt, ScaleEstimator scale_est);

	void ScaleImageStack(ImageVector& img_stack, ScaleEstimator type);


	//typedef std::vector<float> PS;
	void RemoveOutliers(std::vector<float>& pixelstack, float l_limit, float u_limit);

	float Mean(const std::vector<float>& pixelstack);

	float StandardDeviation(const std::vector<float>& pixelstack);

	float Median(std::vector<float>& pixelstack);

	float Min(const std::vector<float>& pixelstack);

	void SigmaClip(std::vector<float>& pixelstack, float l_sigma = 2, float u_sigma = 3);

	void WinsorizedSigmaClip(std::vector<float>& pixelstack, float l_sigma = 2, float u_sigma = 3);


public:

	ImageStacking() = default;

	ImageStacking(Integration integration) :m_integration(integration) {}

	ImageStacking(ScaleEstimator scale_est) :m_scale_est(scale_est) {}

	ImageStacking(Rejection method, float l_sigma = 2, float u_sigma = 3) :m_rejection(method), m_l_sigma(l_sigma), m_u_sigma(u_sigma) {}

	ImageStacking(Integration integration, ScaleEstimator scale_est, Rejection method, float l_sigma, float u_sigma)
		:m_integration(integration), m_scale_est(scale_est), m_rejection(method), m_l_sigma(l_sigma), m_u_sigma(u_sigma) {}

	//create modifiers to change sclae_est, rejection, integration

	void IntegrateImages(ImageVector& img_stack, Image32& output);
};

