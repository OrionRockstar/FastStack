#include "pch.h"
#include "HistogramTransformation.h"
#include "Histogram.h"
#include "FastStack.h"


using HT = HistogramTransformation;

float HT::MTFCurve::MTF(float pixel)const {

	if (pixel <= 0.0f) return 0;

	else if (pixel >= 1) return 1;

	else if (pixel == m_midtone)  return 0.5;

	return (m1 * pixel) / ((m2 * pixel) - m_midtone);

}

float HT::MTFCurve::transformPixel(float pixel)const {
	pixel = (pixel - m_shadow) * dv;

	return MTF(pixel);
}

void HT::MTFCurve::generate16Bit_LUT() {
	m_lut.resize(65536);
	for (int el = 0; el < 65536; ++el)
		m_lut[el] = transformPixel(el / 65535.0f) * 65535;
}

void HT::MTFCurve::generate8Bit_LUT() {
	m_lut.resize(256);
	for (int el = 0; el < 256; ++el)
		m_lut[el] = transformPixel(el / 255.0f) * 255;
}

template <typename T>
void HT::MTFCurve::applyChannel(Image<T>& img, int ch) {

	if (isFloatImage(img))
		for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel)
			*pixel = transformPixel(*pixel);

	else {
		if (isUByteImage(img))
			generate8Bit_LUT();

		else if (isUShortImage(img))
			generate16Bit_LUT();

		for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel)
			*pixel = m_lut[*pixel];
	}
}
template void HT::MTFCurve::applyChannel(Image8&, int);
template void HT::MTFCurve::applyChannel(Image16&, int);
template void HT::MTFCurve::applyChannel(Image32&, int);




float HT::transformPixel(ColorComponent component, float pixel)const {
	return (*this)[component].transformPixel(pixel);
}

float HT::MTF(float pixel, float midtone) {

	if (pixel <= 0.0f) return 0;

	else if (pixel >= 1) return 1;

	else if (pixel == midtone)  return 0.5;

	return ((midtone - 1) * pixel) / (((2 * midtone - 1) * pixel) - midtone);

}

template<typename T>
void HT::computeSTFCurve(const Image<T>& img){

	auto rgbk = &(*this)[ColorComponent::rgb_k];
	rgbk->setMidtone(0.25);

	float median = 0, nMAD = 0;
	for (int ch = 0; ch < img.channels(); ++ch) {
		T cm = img.computeMedian(ch, true);
		median += cm;
		nMAD += img.compute_nMAD(ch, cm, true);
	}

	median = Pixel<float>::toType(T(median / img.channels()));
	nMAD = Pixel<float>::toType(T(nMAD / img.channels()));

	float shadow = (median > 2.8 * nMAD) ? median - 2.8f * nMAD : 0;
	float midtone = rgbk->MTF(median - shadow);

	rgbk->setShadow(shadow);
	rgbk->setMidtone(midtone);
}
template void HT::computeSTFCurve(const Image8&);
template void HT::computeSTFCurve(const Image16&);
template void HT::computeSTFCurve(const Image32&);

template<typename T>
void HT::apply(Image<T>& img) {

	using enum ColorComponent;

	if (!(*this)[rgb_k].IsIdentity()) {
		auto rgbk = (*this)[rgb_k];

		if (isFloatImage(img))
			for (auto& pixel : img)
				pixel = rgbk.transformPixel(pixel);

		else {
			if (isUByteImage(img))
				rgbk.generate8Bit_LUT();

			else if (isUShortImage(img))
				rgbk.generate16Bit_LUT();

			for (auto& pixel : img)
				pixel = rgbk.m_lut[pixel];
		}

	}

	if (img.channels() == 1)
		return;

	if (!(*this)[red].IsIdentity())
		(*this)[red].applyChannel(img, 0);

	if (!(*this)[green].IsIdentity())
		(*this)[green].applyChannel(img, 1);

	if (!(*this)[blue].IsIdentity())
		(*this)[blue].applyChannel(img, 2);

}
template void HT::apply(Image8&);
template void HT::apply(Image16&);
template void HT::apply(Image32&);
