#pragma once
#include "Image.h"
#include "RGBColorSpace.h"
#include "Histogram.h"

class HistogramTransformation {

	class MTFCurve {

		float m_shadow = 0;
		float m_midtone = 0.5;
		float m_highlights = 1.0;

		float m1 = m_midtone - 1;
		float m2 = 2 * m_midtone - 1;
		float dv = 1.0 / (m_highlights - m_shadow);

	public:
		std::vector<uint16_t> m_lut;

		MTFCurve(float shadow, float midtone, float highlight) : m_shadow(shadow), m_midtone(midtone), m_highlights(highlight) {}

		MTFCurve() = default;

		bool isIdentity() { return (m_shadow == 0.0f && m_midtone == 0.5f && m_highlights == 1.0f); }

		float shadow() const { return m_shadow; }

		void setShadow(float shadow) {
			m_shadow = shadow;
			dv = 1.0 / (m_highlights - m_shadow);
		}

		float midtone() const { return m_midtone; }

		void setMidtone(float midtone) {
			m_midtone = midtone;
			m1 = m_midtone - 1;
			m2 = 2 * m_midtone - 1;
		}

		float highlight() const { return m_highlights; }

		void setHighlight(float hightlight) {
			m_highlights = hightlight;
			dv = 1.0 / (m_highlights - m_shadow);
		}

		float MTF(float pixel)const;

		float transformPixel(float pixel)const;

		void generate16Bit_LUT();

		void generate8Bit_LUT();

		template <typename T>
		void applyChannel(Image<T>& img, int ch);
	};

private:
	std::array<MTFCurve, 4> m_hist_curves;

public:
	MTFCurve& operator[](ColorComponent comp) {
		return m_hist_curves[int(comp)];
	}

	const MTFCurve& operator[](ColorComponent comp) const {
		return m_hist_curves[int(comp)];
	}

	HistogramTransformation() = default;

	float shadow(ColorComponent component)const {
		return (*this)[component].shadow();
	}

	void setShadow(ColorComponent component, float shadow) {
		(*this)[component].setShadow(shadow);
	}

	float midtone(ColorComponent component)const {
		return (*this)[component].midtone();
	}

	void setMidtone(ColorComponent component, float midtone) {
		(*this)[component].setMidtone(midtone);
	}

	float highlight(ColorComponent component)const {
		return (*this)[component].highlight();
	}

	void setHighlight(ColorComponent component, float highlight) {
		(*this)[component].setHighlight(highlight);
	}

	float transformPixel(ColorComponent component, float pixel)const;

	Histogram transformHistogram(ColorComponent component, const Histogram& histogram);

	static float MTF(float pixel, float midtone);

	template<typename T>
	void computeSTFCurve(const Image<T>& img);

	template<typename T>
	void apply(Image<T>& img);
};
