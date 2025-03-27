#pragma once
#include "CurveInterpolation.h"
#include "Image.h"
#include "ProcessDialog.h"

class ColorSaturation {
	Curve m_saturation_curve = Curve({ 0.0,0.0 }, { 1.0,0.0 });
    double m_hue_shift = 0;
	int m_scale = 1;

public:
	void setHueShift(double amount = 0) {
		m_hue_shift = amount;
	}

	Curve& saturationCurve() { return m_saturation_curve; }

    void setInterpolation(Curve::Type type) {
		m_saturation_curve.setInterpolation(type);
        m_saturation_curve.computeCoeffecients();
    }

	void setScale(int scale) { m_scale = scale; }

public:
	template<typename T>
	void apply(Image<T>& img);
};
