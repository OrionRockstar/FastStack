#include "pch.h"
#include "HistogramTransformation.h"

template <typename Image>
void HistogramTransformation::HistogramCurve::ApplyChannel(Image& img, int ch) {
	if (img.is_float())

		for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel)
			*pixel = TransformPixel(*pixel);

	else {
		if (img.is_uint8())
			Generate8Bit_LUT();

		if (img.is_uint16())
			Generate16Bit_LUT();

		for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel)
			*pixel = m_lut[*pixel];
	}
}
template void HistogramTransformation::HistogramCurve::ApplyChannel(Image8&, int);
template void HistogramTransformation::HistogramCurve::ApplyChannel(Image16&, int);
template void HistogramTransformation::HistogramCurve::ApplyChannel(Image32&, int);

HistogramTransformation::HistogramTransformation(Component component, float shadow, float midtone, float highlight) {
	using enum Component;
	switch (component) {
	case red:
		Red = HistogramCurve(shadow, midtone, highlight);
		break;
	case green:
		Green = HistogramCurve(shadow, midtone, highlight);
		break;
	case blue:
		Blue = HistogramCurve(shadow, midtone, highlight);
		break;
	case rgb_k:
		RGB_K = HistogramCurve(shadow, midtone, highlight);
		break;
	}
}

void HistogramTransformation::ModifyShadow(Component component, float shadow) {
	using enum Component;
	switch (component) {
	case red:
		Red.ModifyShadow(shadow);
		return;
	case green:
		Green.ModifyShadow(shadow);
		return;
	case blue:
		Blue.ModifyShadow(shadow);
		return;
	case rgb_k:
		RGB_K.ModifyShadow(shadow);
		return;
	}
}

void HistogramTransformation::ModifyMidtone(Component component, float midtone) {
	using enum Component;
	switch (component) {
	case red:
		Red.ModifyMidtone(midtone);
		return;
	case green:
		Green.ModifyMidtone(midtone);
		return;
	case blue:
		Blue.ModifyMidtone(midtone);
		return;
	case rgb_k:
		RGB_K.ModifyMidtone(midtone);
		return;
	}
}

template<typename T>
static T AverageMedian(Image<T>& img) {
	float sum = 0;
	for (int ch = 0; ch < img.Channels(); ++ch)
		sum += img.Median(ch);
	return sum / img.Channels();
}

template<typename T>
static T AverageMAD(Image<T>& img) {
	float sum = 0;
	for (int ch = 0; ch < img.Channels(); ++ch)
		sum += img.MAD(ch);
	return sum / img.Channels();
}


template<typename Image>
void HistogramTransformation::STFStretch(Image& img) {
	ModifyMidtone(Component::rgb_k, 0.25);

	img.ComputeMAD(true);
	float nMAD = img.ToFloat(1.4826f * AverageMAD(img));
	float median = img.ToFloat(AverageMedian(img));

	float shadow = (median > 2.8 * nMAD) ? median - 2.8f * nMAD : 0;
	float midtone = RGB_K.MTF(median - shadow);

	RGB_K.ModifyShadow(shadow);
	RGB_K.ModifyMidtone(midtone);
	Apply(img);

}
template void HistogramTransformation::STFStretch(Image8&);
template void HistogramTransformation::STFStretch(Image16&);
template void HistogramTransformation::STFStretch(Image32&);

template<typename Image>
void HistogramTransformation::Apply(Image& img) {

	if (!RGB_K.IsIdentity()) {

		if (img.is_float())
			for (auto& pixel : img)
				pixel = RGB_K.TransformPixel(pixel);

		else {
			if (img.is_uint8())
				RGB_K.Generate8Bit_LUT();

			if (img.is_uint16())
				RGB_K.Generate16Bit_LUT();

			for (auto& pixel : img)
				pixel = RGB_K.m_lut[pixel];
		}

	}

	if (img.Channels() == 1)
		return;

	if (!Red.IsIdentity())
		Red.ApplyChannel(img, 0);

	if (!Green.IsIdentity())
		Green.ApplyChannel(img, 1);

	if (!Blue.IsIdentity())
		Blue.ApplyChannel(img, 2);

}
template void HistogramTransformation::Apply(Image8&);
template void HistogramTransformation::Apply(Image16&);
template void HistogramTransformation::Apply(Image32&);