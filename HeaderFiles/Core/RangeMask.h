#pragma once
#include "Image.h"


class RangeMask {

	float m_low = 0.0;
	float m_high = 1.0;

	float m_fuzziness = 0.0;
	float m_smoothness = 0.0;

	bool m_invert = false;
	bool m_screening = false;
	bool m_lightness = true;

public:
	float low()const { return m_low; }

	void setLow(float low) { m_low = low; }

	float high()const { return m_high; }

	void setHigh(float high) { m_high = high; }

	float fuzziness()const { return m_fuzziness; }

	void setFuzziness(float fuzziness) { m_fuzziness = fuzziness; }

	float smoothness()const { return m_smoothness; }

	void setSmoothness(float smoothness) {
		if (0 <= smoothness && smoothness <= 100)
			m_smoothness = smoothness;
	}

	bool lightness()const { return m_lightness; }

	void setLightness(bool lightness) { m_lightness = lightness; }

	bool screening()const { return m_screening; }

	void setScreening(bool screening) { m_screening = screening; }

	bool invert()const { return m_invert; }

	void setInvert(bool invert) { m_invert = invert; }

	template <typename T>
	Image<T> generateMask(const Image<T>& img);

	template <typename T>
	void generateMask_overwrite(Image<T>& img);
};