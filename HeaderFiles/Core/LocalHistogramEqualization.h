#pragma once
#include "Image.h"
#include "Histogram.h"
#include "ProcessDialog.h"

class LocalHistogramEqualization {

	ProgressSignal m_ps;

	int m_kernel_radius = 64;
	float m_contrast_limit = 2.0;
	float m_amount = 1.0;

	bool m_is_circular = false;

	Histogram::Resolution m_hist_res = Histogram::Resolution::_8bit;

public:

	LocalHistogramEqualization() = default;

private:
	struct KernelHistogram {

		Histogram m_histogram;
		int m_multiplier = m_histogram.resolution() - 1;
		int m_count = 0;

		int m_radius = 64;
		int m_dimension = 2 * m_radius + 1;
		bool m_is_circular = false;

		std::vector<int> back_pix;//contains displacement from center coloumn
		std::vector<int> front_pix;//
		std::vector<char> k_mask;

		KernelHistogram(Histogram::Resolution resolution, int kernel_radius, bool circular);

		KernelHistogram(const KernelHistogram& kh);

		Histogram& histogram() { return m_histogram; }

		uint32_t count()const { return m_count; }

		int multiplier()const { return m_multiplier; }

		template<typename T>
		void populate(Image<T>& img, int y);

		template<typename T>
		void update(Image<T>& img, int x, int y);
	};

public:
	const ProgressSignal* progressSignal()const { return &m_ps; }

	int kernelRadius()const { return m_kernel_radius; }

	void setKernelRadius(int kernel_radius) { m_kernel_radius = kernel_radius; }

	float contrastLimit()const { return m_contrast_limit; }

	void setContrastLimit(float contrast_limit) { m_contrast_limit = contrast_limit; }

	float amount()const { return m_amount; }

	void setAmount(float amount) { m_amount = amount; }

	bool isCircular()const { return m_is_circular; }

	void setCircularKernel(bool is_circular) { 
		m_is_circular = is_circular; }

	Histogram::Resolution histogramResolution()const { return m_hist_res; }

	void setHistogramResolution(Histogram::Resolution resolution) { m_hist_res = resolution; }

	template<typename T>
	void apply(Image<T>&img);
};

