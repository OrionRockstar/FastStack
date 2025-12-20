#include "pch.h"
#include "SCNR.h"


float SCNR::remove(float color, float v1, float v2)const {

	using enum Method;
	switch (protectionMethod()) {

	case maximum_mask: {
		float m = math::max(v1, v2);
		return maskProtection(color, m);
	}
	case additive_mask: {
		float m = math::min(1.0f, v1 + v2);
		return maskProtection(color, m);
	}
	case average_neutral: {
		float m = 0.5f * (v1 + v2);
		return math::min(color, m);
	}
	case minimum_neutral: {
		float m = math::min(v1, v2);
		return math::min(color, m);
	}
	case maximum_neutral: {
		float m = math::max(v1, v2);
		return math::min(color, m);
	}
	default:
		return color;
	}
}

void SCNR::removeColor(Color<float>& color)const {

	switch (removeColor()) {
	case Colors::red: 
		color.red = remove(color.red, color.green, color.blue);
		return;
	case Colors::green:
		color.green = remove(color.green, color.red, color.blue);
		return;
	case Colors::blue:
		color.blue = remove(color.blue, color.red, color.green);
		return;
	default:
		return;
	}
}

template<typename T>
void SCNR::apply(Image<T>& img) {

	if (img.channels() != 3)
		return;

	for (int y = 0; y < img.rows(); ++y) {
		for (int x = 0; x < img.cols(); ++x) {
			auto c = img.template color<float>(x, y);

			if (preserveLightness()) {
				float L = ColorSpace::CIEL(c);
				removeColor(c);
				c = ColorSpace::CIELabtoRGBf(L, ColorSpace::CIEa(c), ColorSpace::CIEb(c));
			}

			else
				removeColor(c);

			img.template setColor<>(x, y, c);
		}
	}
}
template void SCNR::apply(Image8&);
template void SCNR::apply(Image16&);
template void SCNR::apply(Image32&);