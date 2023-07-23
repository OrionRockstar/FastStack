#pragma once
#include "Image.h"
#include "CurveInterpolation.h"
#include "Maths.h"

enum class CurveComponent {
	red,
	green,
	blue,
	rgb_k,
	Lightness,
	a,
	b,
	c,
	hue,
	saturation
};

class CurveTransform {
private:

public:

	Curve Red;
	Curve Green;
	Curve Blue;
	Curve RGB_K;
	Curve Lightness;
	Curve a;
	Curve b;
	Curve c;
	Curve Hue;
	Curve Saturation;

	CurveTransform(CurveComponent comp, Pointsf& vec, CurveType curve = CurveType::akima_spline);

	CurveTransform() = default;
	~CurveTransform() {};

	void AddPoint(CurveComponent comp, Pointf point);

	void ModifyPoint(CurveComponent comp, int el, Pointf point);

	void ChangeInterpolation(CurveComponent comp, CurveType type);

	void Reset();

	template<typename T>
	void Apply(Image<T>& img);
};


