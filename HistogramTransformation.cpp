#include "pch.h"
#include "HistogramTransformation.h"

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
		if (img.Channels() == 1)
			for (auto& pixel : img)
				pixel = img.ToType(RGB_K.TransformPixel(img.ToFloat(pixel)));

		else {
			int num = (omp_get_max_threads() > 2) ? 2 : omp_get_max_threads();
#pragma omp parallel for num_threads(num)
			for (int el = 0; el < img.Total(); ++el) {

				float R, G, B;
				img.ToRGBFloat(el, R, G, B);

				R = RGB_K.TransformPixel(R);
				G = RGB_K.TransformPixel(G);
				B = RGB_K.TransformPixel(B);

				img.ToRGBType(el, R, G, B);
			}
		}
	}

	if (img.Channels() == 1)
		return;


	if (!Red.IsIdentity() && Green.IsIdentity() && Blue.IsIdentity()) {
		for (auto pixel = img.begin(0); pixel != img.end(0); ++pixel)
			*pixel = img.ToType(Red.TransformPixel(img.ToFloat(*pixel)));
		return;
	}

	else if (!Green.IsIdentity() && Red.IsIdentity() && Blue.IsIdentity()) {
		for (auto pixel = img.begin(1); pixel != img.end(1); ++pixel)
			*pixel = img.ToType(Green.TransformPixel(img.ToFloat(*pixel)));
		return;
	}

	else if (!Blue.IsIdentity() && Red.IsIdentity() && Green.IsIdentity()) {
		for (auto pixel = img.begin(2); pixel != img.end(2); ++pixel)
			*pixel = img.ToType(Blue.TransformPixel(img.ToFloat(*pixel)));
		return;
	}

	else if (!Blue.IsIdentity() || !Red.IsIdentity() || !Green.IsIdentity()) {

		int num = (omp_get_max_threads() > 2) ? 2 : omp_get_max_threads();
#pragma omp parallel for num_threads(num)
		for (int el = 0; el < img.Total(); ++el) {

			float R, G, B;
			img.ToRGBFloat(el, R, G, B);

			R = Red.TransformPixel(R);
			G = Green.TransformPixel(G);
			B = Blue.TransformPixel(B);

			img.ToRGBType(el, R, G, B);
		}
	}
}
template void HistogramTransformation::Apply(Image8&);
template void HistogramTransformation::Apply(Image16&);
template void HistogramTransformation::Apply(Image32&);