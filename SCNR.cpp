#include "pch.h"
#include "SCNR.h"

void SCNR::removeRed(Color<float>& c)const {

	using enum Method;

	switch (protectionMethod()) {

	case maximum_mask: {
		float m = math::max(c.green(), c.blue());
		c.rRed() = maskProtection(c.red(), m);
		break;
	}
	case additive_mask: {
		float m = math::min(1.0f, c.green() + c.blue());
		c.rRed() = maskProtection(c.red(), m);
		break;
	}
	case average_neutral: {
		float m = 0.5 * (c.green() + c.blue());
		c.rRed() = math::min(c.red(), m);
		break;
	}

	case maximum_neutral: {
		float m = math::max(c.green(), c.blue());
		c.rRed() = math::min(c.red(), m);
		break;
	}
	}
}

void SCNR::removeGreen(Color<float>& c)const {

	using enum Method;

	switch (protectionMethod()) {

	case maximum_mask: {
		float m = math::max(c.red(), c.blue());
		c.rGreen() = maskProtection(c.green(), m);
		break;
	}
	case additive_mask: {
		float m = math::min(1.0f, c.red() + c.blue());
		c.rGreen() = maskProtection(c.green(), m);
		break;
	}
	case average_neutral: {
		float m = 0.5 * (c.red() + c.blue());
		c.rGreen() = math::min(c.green(), m);
		break;
	}

	case maximum_neutral: {
		float m = math::max(c.red(), c.blue());
		c.rGreen() = math::min(c.green(), m);
		break;
	}
	}
}

void SCNR::removeBlue(Color<float>& c)const {

	using enum Method;

	switch (protectionMethod()) {

	case maximum_mask: {
		float m = math::max(c.red(), c.green());
		c.rBlue() = maskProtection(c.blue(), m);
		break;
	}
	case additive_mask: {
		float m = math::min(1.0f, c.red() + c.green());
		c.rBlue() = maskProtection(c.blue(), m);
		break;
	}
	case average_neutral: {
		float m = 0.5 * (c.red() + c.green());
		c.rBlue() = math::min(c.blue(), m);
		break;
	}

	case maximum_neutral: {
		float m = math::max(c.red(), c.green());
		c.rBlue() = math::min(c.blue(), m);
		break;
	}
	}
}

void SCNR::removeColor(Color<float>& c)const {

	switch (removeColor()) {
	case Colors::red:
		return removeRed(c);

	case Colors::green:
		return removeGreen(c);

	case Colors::blue:
		return removeBlue(c);
	}
}

template<typename T>
void SCNR::apply(Image<T>& img) {

	if (img.channels() != 3)
		return;

	for (int y = 0; y < img.rows(); ++y) {
		for (int x = 0; x < img.cols(); ++x) {
			auto c = img.color<float>(x, y);

			if (preserveLightness()) {
				double L = ColorSpace::CIEL(c);
				removeColor(c);
				c = ColorSpace::CIELabtoRGBf(L, ColorSpace::CIEa(c), ColorSpace::CIEb(c));
			}

			else
				removeColor(c);

			img.setColor(x, y, c);
		}
	}
}
template void SCNR::apply(Image8&);
template void SCNR::apply(Image16&);
template void SCNR::apply(Image32&);