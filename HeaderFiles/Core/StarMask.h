#pragma once
#include "StarDetector.h"
#include "ProcessDialog.h"

class StarMask {

	StarDetector m_sd;
	float m_stddev = 2.0f;
	bool m_real_value = false;

public:
	template<typename T>
	Image<T> generateStarMask(const Image<T>& src);

	StarDetector& starDetector() { return m_sd; }

	float gaussianBlurStdDev()const { return m_stddev; }

	void setGaussianBlurStdDev(float stddev) { m_stddev = stddev; }

	bool realValue()const { return m_real_value; }

	void setRealValue(bool v) { m_real_value = v; }

	void reset() { m_sd = StarDetector(); }
};


