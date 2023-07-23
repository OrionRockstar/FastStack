#pragma once
#include "Image.h"

enum class Component {
	red,
	green,
	blue,
	rgb_k
};

class HistogramTransformation {

	struct HistogramCurve {

		float m_shadow = 0;
		float m_midtone = 0.5;
		float m_highlights = 1.0;

		float m1 = m_midtone - 1;
		float m2 = 2 * m_midtone - 1;
		float dv = 1.0 / (m_highlights - m_shadow);

		HistogramCurve(float shadow, float midtone, float highlight) : m_shadow(shadow), m_midtone(midtone), m_highlights(highlight) {}

		HistogramCurve() = default;

		bool IsIdentity() { return (m_shadow == 0 && m_midtone == 0.5 && m_highlights == 1.0); }

		void ModifyShadow(float shadow) {
			m_shadow = shadow;
			dv = 1.0 / (m_highlights - m_shadow);
		}

		void ModifyMidtone(float midtone) {
			m_midtone = midtone;
			m1 = m_midtone - 1;
			m2 = 2 * m_midtone - 1;
		}

		void ModifyHighlight(float hightlight) {
			m_highlights = hightlight;
			dv = 1.0 / (m_highlights - m_shadow);
		}

		float ScalePixel(float pixel) {
			return (pixel - m_shadow) * dv;
		}

		float MTF(float pixel) {

			if (pixel <= 0.0f) return 0;

			else if (pixel >= 1) return 1;

			else if (pixel == m_midtone)  return 0.5;

			return (m1 * pixel) / ((m2 * pixel) - m_midtone);

		}

		float TransformPixel(float pixel) {
			pixel = (pixel - m_shadow) * dv;

			return MTF(pixel);
		}

	};

	HistogramCurve Red;
	HistogramCurve Green;
	HistogramCurve Blue;
	HistogramCurve RGB_K;

public:
	HistogramTransformation() = default;

	HistogramTransformation(Component component, float shadow, float midtone, float highlight);

	void ModifyShadow(Component component, float shadow);

	void ModifyMidtone(Component component, float midtone);

	void ModifyHighlight(Component component, float hightlight);

	template<typename Image>
	void STFStretch(Image& img);

	template<typename Image>
	void Apply(Image& img);

};

