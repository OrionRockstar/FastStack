#pragma once
#include "Image.h"
#include "CurveInterpolation.h"
#include "Maths.h"
#include "RGBColorSpace.h"




class CurveTransform {
private:
	std::array<Curve, 10> m_comp_curves;

	Curve& rCurve(ColorComponent comp) { return m_comp_curves[int(comp)]; }

public:

	CurveTransform() = default;

	~CurveTransform() {};

	void setInterpolation(ColorComponent comp, Curve::Type type);

	void setDataPoints(ColorComponent comp, std::vector<QPointF> points);

	void computeCoefficients(ColorComponent comp);

	void interpolateValues(ColorComponent comp, std::vector<double>& values);

	template<typename T>
	void apply(Image<T>& img);
};

