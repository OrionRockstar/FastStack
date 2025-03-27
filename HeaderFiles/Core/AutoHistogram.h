#pragma once
#include "Image.h"
#include "RGBColorSpace.h"

class AutoHistogram {

public:
	enum class StretchMethod {
		gamma,
		log,
		mtf
	};

private:
	std::array<float, 3> m_shadow_clipping = { 0.05,0.05,0.05 };

	std::array<float, 3> m_highlight_clipping = { 0.00,0.00,0.00 };

	std::array<float, 3> m_target_median = { 0.15,0.15,0.15 };

	StretchMethod m_stretch_method = StretchMethod::gamma;

	bool m_histogram_clipping = true;

	bool m_stretch = true;

public:
	AutoHistogram() = default;

	void enableStretching(bool stretch) { m_stretch = stretch; }

	void enableHistogramClipping(bool histogram_clipping) { m_histogram_clipping = histogram_clipping; }

	float shadowClipping(ColorComponent comp)const;

	void setShadowClipping(ColorComponent comp, float percentage);

	float highlightClipping(ColorComponent comp)const;

	void setHighlightClipping(ColorComponent comp, float percentage);

	float targetMedian(ColorComponent comp)const;

	void setTargetMedian(ColorComponent comp,float tgt_median);

	void setStretchMethod(StretchMethod method) { m_stretch_method = method; }

private:
	float LTF(float pixel, float b);

	double computeLogMultiplier(float median, float tgt_median);

public:
	template <typename P>
	void apply(Image<P>& img);
};
