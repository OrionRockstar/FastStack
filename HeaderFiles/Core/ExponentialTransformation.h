#pragma once
#include "Image.h"
#include "GaussianFilter.h"

class ExponentialTransformation {

public:
	enum class Method : uint8_t {
		power_inverted_pixels,
		screen_mask_invert
	};

	Method m_method = Method::power_inverted_pixels;
	float m_order = 1.0;
	bool m_lightness_mask = true;
	GaussianFilter m_gf = GaussianFilter(0.0);

public:
	ExponentialTransformation() = default;

	float order()const { return m_order; }

	void setOrder(float order) { m_order = order; }

	bool lightnessMask()const { return m_lightness_mask; }

	void applyLightnessMask(bool lightness_mask) { m_lightness_mask = lightness_mask; }

	Method method()const { return m_method; }

	void setMethod(Method method) { m_method = method; }

	float sigma()const { return m_gf.sigma(); }

	void setSigma(float sigma) { m_gf.setSigma(sigma); }

	template<typename T>
	void apply(Image<T>&img);
};

