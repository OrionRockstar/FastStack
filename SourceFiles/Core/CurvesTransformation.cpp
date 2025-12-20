#include "pch.h"
#include "CurvesTransformation.h"
#include "FastStack.h"

void CurveTransform::setInterpolation(ColorComponent comp, Curve::Type type) {
	curve(comp).setInterpolation(type);
}

void CurveTransform::setDataPoints(ColorComponent comp, std::vector<QPointF> points) {
	curve(comp).setDataPoints(points);
}

void CurveTransform::computeCoefficients(ColorComponent comp) {
	curve(comp).computeCoeffecients();
}

void CurveTransform::interpolateValues(ColorComponent comp, std::vector<double>& values)const {

	if (m_comp_curves[int(comp)].isIdentity())
		return;

	for (auto& val : values) 
		val = m_comp_curves[int(comp)].interpolate(val);
}

template<typename T>
void CurveTransform::apply(Image<T>& img) {

	using CC = ColorComponent;
	using CCR = const Curve&;

	auto applyChannel = [&, this](const Curve& curve, int ch) {
		for (auto& p : ImageChannel(img, ch))
			p = Pixel<T>::toType(math::clip(curve.interpolate(Pixel<float>::toType(p))));
	};

	CCR RGB_K = ccurve(CC::rgb_k);

	if (!RGB_K.isIdentity()) {

		for (int ch = 0; ch < img.channels(); ++ch)
			std::thread(applyChannel, RGB_K, ch).join();
	}

	if (img.channels() == 1)
		return;

	CCR red = ccurve(CC::red);
	CCR green = ccurve(CC::green);
	CCR blue = ccurve(CC::blue);

	if (!red.isIdentity() || !green.isIdentity() || !blue.isIdentity()) {
		std::thread(applyChannel, red, 0).join();
		std::thread(applyChannel, green, 1).join();
		std::thread(applyChannel, blue, 2).join();
	}

	CCR Lightness = ccurve(CC::Lightness);
	CCR a = ccurve(CC::a);
	CCR b = ccurve(CC::b);

	if (!Lightness.isIdentity() || !a.isIdentity() || !b.isIdentity()) {

#pragma omp parallel for num_threads(3)
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				auto rgb = img.template color<double>(x, y);

				double L, _a, _b;
				ColorSpace::RGBtoCIELab(rgb, L, _a, _b);
				rgb = ColorSpace::CIELabtoRGB(Lightness.interpolate(L), a.interpolate(_a), b.interpolate(_b));
				
				img.template setColor<>(x, y, rgb);
			}
		}
	}



	CCR c = ccurve(CC::c);

	if (!c.isIdentity()) {

#pragma omp parallel for num_threads(3)
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				auto rgb = img.template color<double>(x, y);

				double L, _c, _h;
				ColorSpace::RGBtoCIELch(rgb, L, _c, _h);
				rgb = ColorSpace::CIELchtoRGB(L, c.interpolate(_c), _h);

				img.template setColor<>(x, y, rgb);
			}
		}
	}



	CCR Hue = ccurve(CC::hue);
	CCR Saturation = ccurve(CC::saturation);

	if (!Hue.isIdentity() || !Saturation.isIdentity()) {
#pragma omp parallel for num_threads(3)
		for (int y = 0; y < img.rows(); ++y) {
			for (int x = 0; x < img.cols(); ++x) {
				auto rgb = img.template color<double>(x, y);

				double H, S, V, L;
				ColorSpace::RGBtoHSVL(rgb, H, S, V, L);
				rgb = ColorSpace::HSVLtoRGB(Hue.interpolate(H), Saturation.interpolate(S), V, L);

				img.template setColor<>(x, y, rgb);
			}
		}
	}
}
template void CurveTransform::apply(Image8&);
template void CurveTransform::apply(Image16&);
template void CurveTransform::apply(Image32&);

