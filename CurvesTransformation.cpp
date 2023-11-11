#include "pch.h"
#include "CurvesTransformation.h"

CurveTransform::CurveTransform(CurveComponent comp, Pointsf& vec, CurveType curve) {
	//using enum CurveComponent;
	switch (comp) {
	case CurveComponent::red:
		if (Red.InsertPoints(vec)) {
			Red.type = curve;
			Red.SetCoeffecients();
			Red.do_interpolation = true;
		}
		break;

	case CurveComponent::green:
		if (Green.InsertPoints(vec)) {
			Green.type = curve;
			Green.SetCoeffecients();
			Green.do_interpolation = true;
		}
		break;

	case CurveComponent::blue:
		if (Blue.InsertPoints(vec)) {
			Blue.type = curve;
			Blue.SetCoeffecients();
			Blue.do_interpolation = true;
		}
		break;

	case CurveComponent::rgb_k:
		if (RGB_K.InsertPoints(vec)) {
			RGB_K.type = curve;
			RGB_K.SetCoeffecients();
			RGB_K.do_interpolation = true;
		}
		break;

	case CurveComponent::Lightness:
		if (Lightness.InsertPoints(vec)) {
			Lightness.type = curve;
			Lightness.SetCoeffecients();
			Lightness.do_interpolation = true;
		}
		break;

	case CurveComponent::a:
		if (a.InsertPoints(vec)) {
			a.type = curve;
			a.SetCoeffecients();
			a.do_interpolation = true;
		}
		break;

	case CurveComponent::b:
		if (b.InsertPoints(vec)) {
			b.type = curve;
			b.SetCoeffecients();
			b.do_interpolation = true;
		}
		break;

	case CurveComponent::c:
		if (c.InsertPoints(vec)) {
			c.type = curve;
			c.SetCoeffecients();
			c.do_interpolation = true;
		}
		break;

	case CurveComponent::hue:
		if (Hue.InsertPoints(vec)) {
			Hue.type = curve;
			Hue.SetCoeffecients();
			Hue.do_interpolation = true;
		}
		break;

	case CurveComponent::saturation:
		if (Saturation.InsertPoints(vec)) {
			Saturation.type = curve;
			Saturation.SetCoeffecients();
			Saturation.do_interpolation = true;
		}
		break;
	}
}

void CurveTransform::AddPoint(CurveComponent comp, Pointf point) {

	switch (comp) {
	case CurveComponent::red:
		if (Red.InesrtPoint(point)) {
			Red.SetCoeffecients();
			Red.do_interpolation = true;
		}
		break;

	case CurveComponent::green:
		if (Green.InesrtPoint(point)) {
			Green.SetCoeffecients();
			Green.do_interpolation = true;
		}
		break;

	case CurveComponent::blue:
		if (Blue.InesrtPoint(point)) {
			Blue.SetCoeffecients();
			Blue.do_interpolation = true;
		}
		break;

	case CurveComponent::rgb_k:
		if (RGB_K.InesrtPoint(point)) {
			RGB_K.SetCoeffecients();
			RGB_K.do_interpolation = true;
		}
		break;

	case CurveComponent::Lightness:
		if (Lightness.InesrtPoint(point)) {
			Lightness.SetCoeffecients();
			Lightness.do_interpolation = true;
		}
		break;

	case CurveComponent::a:
		if (a.InesrtPoint(point)) {
			a.SetCoeffecients();
			a.do_interpolation = true;
		}
		break;

	case CurveComponent::b:
		if (b.InesrtPoint(point)) {
			b.SetCoeffecients();
			b.do_interpolation = true;
		}
		break;

	case CurveComponent::c:
		if (c.InesrtPoint(point)) {
			c.SetCoeffecients();
			c.do_interpolation = true;
		}
		break;

	case CurveComponent::hue:
		if (Hue.InesrtPoint(point)) {
			Hue.SetCoeffecients();
			Hue.do_interpolation = true;
		}
		break;

	case CurveComponent::saturation:
		if (Saturation.InesrtPoint(point)) {
			Saturation.SetCoeffecients();
			Saturation.do_interpolation = true;
		}
		break;
	}
}

void CurveTransform::ModifyPoint(CurveComponent comp, int el, Pointf point) {

	if (RGB_K.OutRange(point.x) || RGB_K.OutRange(point.y))
		return;

	switch (comp) {
	case CurveComponent::red:
		Red.points[el].x = point.x;
		Red.points[el].y = point.y;
		Red.SetCoeffecients();
		break;

	case CurveComponent::green:
		Green.points[el].x = point.x;
		Green.points[el].y = point.y;
		Green.SetCoeffecients();
		break;

	case CurveComponent::blue:
		Blue.points[el].x = point.x;
		Blue.points[el].y = point.y;
		Blue.SetCoeffecients();
		break;

	case CurveComponent::rgb_k:
		RGB_K.points[el].x = point.x;
		RGB_K.points[el].y = point.y;
		RGB_K.SetCoeffecients();
		break;

	case CurveComponent::Lightness:
		Lightness.points[el].x = point.x;
		Lightness.points[el].y = point.y;
		Lightness.SetCoeffecients();
		break;

	case CurveComponent::a:
		a.points[el].x = point.x;
		a.points[el].y = point.y;
		a.SetCoeffecients();
		break;

	case CurveComponent::b:
		b.points[el].x = point.x;
		b.points[el].y = point.y;
		b.SetCoeffecients();
		break;

	case CurveComponent::c:
		c.points[el].x = point.x;
		c.points[el].y = point.y;
		c.SetCoeffecients();
		break;

	case CurveComponent::hue:
		Hue.points[el].x = point.x;
		Hue.points[el].y = point.y;
		Hue.SetCoeffecients();
		break;

	case CurveComponent::saturation:
		Saturation.points[el].x = point.x;
		Saturation.points[el].y = point.y;
		Saturation.SetCoeffecients();
		break;
	}
}

void CurveTransform::ChangeInterpolation(CurveComponent comp, CurveType type) {

	switch (comp) {
	case CurveComponent::red:
		Red.type = type;
		Red.SetCoeffecients();
		break;

	case CurveComponent::green:
		Green.type = type;
		Green.SetCoeffecients();
		break;

	case CurveComponent::blue:
		Blue.type = type;
		Blue.SetCoeffecients();
		break;

	case CurveComponent::rgb_k:
		RGB_K.type = type;
		RGB_K.SetCoeffecients();
		break;

	case CurveComponent::Lightness:
		Lightness.type = type;
		Lightness.SetCoeffecients();
		break;

	case CurveComponent::a:
		a.type = type;
		a.SetCoeffecients();
		break;

	case CurveComponent::b:
		b.type = type;
		b.SetCoeffecients();
		break;

	case CurveComponent::c:
		c.type = type;
		c.SetCoeffecients();
		break;

	case CurveComponent::hue:
		Hue.type = type;
		Hue.SetCoeffecients();
		break;

	case CurveComponent::saturation:
		Saturation.type = type;
		Saturation.SetCoeffecients();
		break;
	}
}

void CurveTransform::Reset() {
	Red = Curve();
	Green = Curve();
	Blue = Curve();
	RGB_K = Curve();
	Lightness = Curve();
	a = Curve();
	b = Curve();
	c = Curve();
	Hue = Curve();
	Saturation = Curve();
}

template<typename T>
void CurveTransform::Apply(Image<T>& img) {

	if (!RGB_K.IsIdentity()) {
		if (img.Channels() == 1)
			for (T& pixel : img) {
				double val = Pixel<double>::toType(pixel);
				val = Clip(RGB_K.Interpolate(val));
				pixel = Pixel<T>::toType(val);
			}

		else if (img.Channels() == 3)
#pragma omp parallel for num_threads(4)
			for (int el = 0; el < img.Total(); ++el) {
				double R, G, B;
				img.toRGBDouble(el, R, G, B);

				R = Clip(RGB_K.Interpolate(R));
				G = Clip(RGB_K.Interpolate(G));
				B = Clip(RGB_K.Interpolate(B));

				img.fromRGBDouble(el, R, G, B);
			}

	}

	if (img.Channels() == 1)
		return;

	if (!Red.IsIdentity() || !Green.IsIdentity() || !Blue.IsIdentity()) {
#pragma omp parallel for
		for (int el = 0; el < img.Total(); ++el) {
			double R, G, B;
			img.toRGBDouble(el, R, G, B);

			R = Clip(Red.Interpolate(R));
			G = Clip(Green.Interpolate(G));
			B = Clip(Blue.Interpolate(B));

			img.fromRGBDouble(el, R, G, B);
		}
	}

	if (!Lightness.IsIdentity() || !a.IsIdentity() || !b.IsIdentity()) {
#pragma omp parallel for
		for (int el = 0; el < img.Total(); ++el) {
			double R, G, B;
			img.toRGBDouble(el, R, G, B);

			double L, _a, _b;
			ColorSpace::RGBtoCIELab(R, G, B, L, _a, _b);
			ColorSpace::CIELabtoRGB(Lightness.Interpolate(L), a.Interpolate(_a), b.Interpolate(_b), R, G, B);

			img.fromRGBDouble(el, Clip(R), Clip(G), Clip(B));
		}
	}

	if (!c.IsIdentity()) {
#pragma omp parallel for
		for (int el = 0; el < img.Total(); ++el) {
			double R, G, B;
			img.toRGBDouble(el, R, G, B);

			double L, _c, _h;
			ColorSpace::RGBtoCIELch(R, G, B, L, _c, _h);
			ColorSpace::CIELchtoRGB(L, c.Interpolate(_c), _h, R, G, B);

			img.fromRGBDouble(el, Clip(R), Clip(G), Clip(B));
		}
	}

	if (!Hue.IsIdentity() || !Saturation.IsIdentity()) {
#pragma omp parallel for
		for (int el = 0; el < img.Total(); ++el) {
			double R, G, B;
			img.toRGBDouble(el, R, G, B);

			double H, S, V, L;
			ColorSpace::RGBtoHSVL(R, G, B, H, S, V, L);
			ColorSpace::HSVLtoRGB(Hue.Interpolate(H), Saturation.Interpolate(S), V, L, R, G, B);

			img.fromRGBDouble(el, Clip(R), Clip(G), Clip(B));
		}
	}

}
template void CurveTransform::Apply(Image8&);
template void CurveTransform::Apply(Image16&);
template void CurveTransform::Apply(Image32&);