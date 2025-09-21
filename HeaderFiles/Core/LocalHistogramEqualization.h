#pragma once
#include "Image.h"
#include "Histogram.h"
#include "ProcessDialog.h"

class LocalHistogramEqualization {

	std::unique_ptr<ProgressSignal> m_ps =  std::make_unique<ProgressSignal>();

	int m_kernel_radius = 64;
	float m_contrast_limit = 2.0;
	float m_amount = 1.0;

	bool m_is_circular = false;

	Histogram::Resolution m_hist_res = Histogram::Resolution::_8bit;

public:

	LocalHistogramEqualization() = default;

	LocalHistogramEqualization(const LocalHistogramEqualization& other) {
		*this = other;
	}

	LocalHistogramEqualization& operator=(const LocalHistogramEqualization& other) {

		if (this != &other) {
			m_kernel_radius = other.kernelRadius();
			m_contrast_limit = other.contrastLimit();
			m_amount = other.amount();
			m_is_circular = other.isCircular();
			m_hist_res = other.histogramResolution();
		}
		return *this;
	}

private:
	class KernelHistogram {

		Histogram m_histogram;
		uint16_t m_multiplier = m_histogram.resolution() - 1;
		uint32_t m_count = 0;

		int m_radius = 64;
		uint32_t m_dimension = 2 * m_radius + 1;

		std::vector<int> back_pix;//contains displacement from center coloumn
		std::vector<int> front_pix;//
		std::vector<char> k_mask;

		uint16_t multiplier()const { return m_multiplier; }

		int radius()const { return m_radius; }

		uint32_t dimension()const { return m_dimension; }
	public:
		KernelHistogram(Histogram::Resolution resolution, int kernel_radius, bool circular);

		KernelHistogram(const KernelHistogram& kh);

		Histogram& histogram() { return m_histogram; }

		uint32_t count()const { return m_count; }

		template<typename T>
		void populate(Image<T>& img, int y);

		template<typename T>
		void update(Image<T>& img, int x, int y);
	};

public:
	ProgressSignal* progressSignal()const { return m_ps.get(); }

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

