#pragma once
#include "Image.h"
//#include "ProcessDialog.h"

class BilateralFilter {

	//ProgressSignal* m_ps = new ProgressSignal();

	float m_sigma_s = 2.0; //spatial/gaussiam
	float m_sigma_r = 0.5; //pixel_intensity
	int m_kernel_dim = 3;
	bool m_is_circular = false;

public:
	//ProgressSignal* progressSignal() const { return m_ps; }
	bool isCircular()const { return m_is_circular; }

	void setCircularKernel(bool circular) { m_is_circular = circular; }

	float sigmaSpatial()const { return m_sigma_s; }

	void setSigmaSpatial(float sigma) { m_sigma_s = sigma; }

	float sigmaRange()const { return m_sigma_r; }

	void setSigmaRange(float sigma_range) { m_sigma_r = sigma_range; }

	void setKernelSize(int kernel_dim) { m_kernel_dim = math::max(kernel_dim, 1); }

	template<typename T>
	void apply(Image<T>& img);

	template<typename T>
	void applyTo(const Image<T>& src, Image<T>& dst, float scale_factor, const QRectF& r);
};

