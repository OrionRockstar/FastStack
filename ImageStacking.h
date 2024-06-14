#pragma once
#include "FITS.h"
#include "Maths.h"
#include "ProcessDialog.h"
#include "ImageFileReader.h"


class ImageStacking {
public:
	enum class Integration {
		average,
		median,
		min,
		max
	};

	enum class Normalization {
		additive,
		multiplicative,
		additive_scaling,
		multiplicative_scaling,
		none
	};

	enum class Rejection {
		none,
		sigma_clip,
		winsorized_sigma_clip,
		percintile_clip
	};

private:
	struct PixelRows {

	private:
		ImageStacking* m_isp;
		std::vector<float> m_pixels;
		int m_cols = 0;
		int m_num_imgs = 0;
		int m_size = 0;

		int m_max_threads = (omp_get_max_threads() < 4) ? omp_get_max_threads() : 4;

	public:
		PixelRows(int num_imgs, int cols, ImageStacking& isp);

	private:
		float& operator() (int x, int y) { return m_pixels[y * m_cols + x]; }

	public:
		int NumberofImages()const { return m_num_imgs; }

		void Fill(const Point<>& start_point);

		void FillPixelStack(std::vector<float>& pixelstack, int x, int ch);
	};

	std::vector<std::unique_ptr<ImageFile>> m_imgfile_vector;
	FileVector m_file_paths;

	Normalization m_normalization = Normalization::none;

	std::vector<std::array<float, 3>> mle; //location estimator
	std::vector<std::array<float, 3>> msf; //scale factors

	Integration m_integration = Integration::average;

	Rejection m_rejection = Rejection::none;
	float m_l_sigma = 2.0;
	float m_u_sigma = 3.0;

public:
	ImageStacking() = default;

	void setRejectionMethod(Rejection method) { m_rejection = method; }

	void setNormalation(Normalization normalization) { m_normalization = normalization; }

	void setIntegrationMethod(Integration method) { m_integration = method; }

	void setSigmaLow(float sigma_low) { m_l_sigma = sigma_low; }

	void setSigmaHigh(float sigma_high) { m_u_sigma = sigma_high; }


private:
	void RemoveOutliers(std::vector<float>& pixelstack, float l_limit, float u_limit);

	float Mean(const std::vector<float>& pixelstack);

	float StandardDeviation(const std::vector<float>& pixelstack);

	float Median(std::vector<float>& pixelstack);

	float Min(const std::vector<float>& pixelstack);

	float Max(const std::vector<float>& pixelstack);

	void PercintileClipping(std::vector<float>& pixelstack, float p_low, float p_high);

	void SigmaClip(std::vector<float>& pixelstack, float l_sigma = 2, float u_sigma = 3);

	void WinsorizedSigmaClip(std::vector<float>& pixelstack, float l_sigma = 2, float u_sigma = 3);

	void PixelRejection(std::vector<float>& pixelstack);

	float PixelIntegration(std::vector<float>& pixelstack);

	void ComputeScaleEstimators();

	void OpenFiles();

	bool isFilesSameDimenisions();

	void CloseFiles();

public:
	void GenerateWeightMaps_forDrizzle(FileVector paths);

	Status IntegrateImages(FileVector paths, Image32 & output);
};

