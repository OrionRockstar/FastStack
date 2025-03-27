#include "pch.h"
#include "CurvesTransformation.h"
#include "FastStack.h"

void CurveTransform::setInterpolation(ColorComponent comp, Curve::Type type) {
	rCurve(comp).setInterpolation(type);
}

void CurveTransform::setDataPoints(ColorComponent comp, std::vector<QPointF> points) {
	rCurve(comp).setDataPoints(points);
}

void CurveTransform::computeCoefficients(ColorComponent comp) {
	rCurve(comp).computeCoeffecients();
}

void CurveTransform::interpolateValues(ColorComponent comp, std::vector<double>& values) {

	if (m_comp_curves[int(comp)].isIdentity())
		return;

	for (auto& val : values) {
		val = m_comp_curves[int(comp)].interpolate(val);
	}
}

template<typename T>
void CurveTransform::apply(Image<T>& img) {

	using CC = ColorComponent;

	auto applyChannel = [&, this](Curve* curve, int ch) {
		for (auto& p : ImageChannel(img, ch))
			p = p = math::clip(curve->interpolate(p));
	};

	Curve RGB_K = rCurve(CC::rgb_k);

	if (!RGB_K.isIdentity()) {

#pragma omp parallel for num_threads(img.channels())
		for (int ch = 0; ch < img.channels(); ++ch)
			applyChannel(&RGB_K, ch);
	}



	if (img.channels() == 1)
		return;

	Curve red = rCurve(CC::red);
	Curve green = rCurve(CC::green);
	Curve blue = rCurve(CC::blue);

	if (!red.isIdentity() || !green.isIdentity() || !blue.isIdentity()) {

		std::thread t0 = std::thread(applyChannel, &red, 0);
		std::thread t1 = std::thread(applyChannel, &green, 1);
		std::thread t2 = std::thread(applyChannel, &blue, 2);

		t0.join();
		t1.join();
		t2.join();
	}

	Curve Lightness = rCurve(CC::Lightness);
	Curve a = rCurve(CC::a);
	Curve b = rCurve(CC::b);

	if (!Lightness.isIdentity() || !a.isIdentity() || !b.isIdentity()) {

#pragma omp parallel for num_threads(3)
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				auto rgb = img.color<double>(x, y);

				double L, _a, _b;
				ColorSpace::RGBtoCIELab(rgb, L, _a, _b);
				rgb = ColorSpace::CIELabtoRGB(Lightness.interpolate(L), a.interpolate(_a), b.interpolate(_b));
				
				img.setColor<>(x, y, rgb);
			}
		}
	}



	Curve c = rCurve(CC::c);

	if (!c.isIdentity()) {

#pragma omp parallel for num_threads(3)
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				auto rgb = img.color<double>(x, y);

				double L, _c, _h;
				ColorSpace::RGBtoCIELch(rgb, L, _c, _h);
				rgb = ColorSpace::CIELchtoRGB(L, c.interpolate(_c), _h);

				img.setColor<>(x, y, rgb);
			}
		}
	}



	Curve Hue = rCurve(CC::hue);
	Curve Saturation = rCurve(CC::saturation);

	if (!Hue.isIdentity() || !Saturation.isIdentity()) {
#pragma omp parallel for num_threads(3)
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				auto rgb = img.color<double>(x, y);

				double H, S, V, L;
				ColorSpace::RGBtoHSVL(rgb, H, S, V, L);
				rgb = ColorSpace::HSVLtoRGB(Hue.interpolate(H), Saturation.interpolate(S), V, L);

				img.setColor(x, y, rgb);
			}
		}
	}
}
template void CurveTransform::apply(Image8&);
template void CurveTransform::apply(Image16&);
template void CurveTransform::apply(Image32&);

