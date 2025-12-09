#include "pch.h"
#include "Wavelet.h"
#include "MorphologicalTransformation.h"
#include "FastStack.h"
#include "FITS.h"



template<typename T>
Wavelet::Images::Images(const Image<T> src, bool to_grayscale) {

	src.copyTo(source);

	if (to_grayscale)
		source.toGrayscale();

	convolved = Image32(source.rows(), source.cols(), source.channels());
	wavelet = Image32(source.rows(), source.cols(), source.channels());
}

const std::vector<float>& Wavelet::scalingFunctionKernel(ScalingFunction sf)const {
	using enum ScalingFunction;

	switch (sf) {

	case linear_3:
		return linear;

	case smallscale3_3:
		return ss3_3;

	case b3spline_5:
		return b3;

	case gaussian_5:
		return g_5;

	default:
		return b3;
	}
}

void Wavelet::atrous(uint8_t layer, Images& images) {

	int _2i = pow(2, layer);

	const std::vector<float>& sfv = scalingFunctionKernel(m_scaling_func);

	float sfv_sum = 0;
	for (float v : sfv)
		sfv_sum += v;

	int left = -(int(sfv.size() - 1) / 2) * _2i;

	for (int ch = 0; ch < images.source.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < images.source.rows(); ++y)
			for (int x = 0; x < images.source.cols(); ++x) {

				float sum = 0;

				for (int i = 0, d = left; i < sfv.size(); ++i, d += _2i)
					sum += images.source.at_replicated(x + d, y, ch) * sfv[i];

				images.wavelet(x, y, ch) = sum;
			}

#pragma omp parallel for
		for (int y = 0; y < images.wavelet.rows(); ++y)
			for (int x = 0; x < images.wavelet.cols(); ++x) {

				float sum = 0;

				for (int i = 0, d = left; i < sfv.size(); ++i, d += _2i)
					sum += images.wavelet.at_replicated(x, y + d, ch) * sfv[i];

				images.convolved(x, y, ch) = sum;
			}
	}

	for (int el = 0; el < images.source.totalPxCount(); ++el)
		images.wavelet[el] = images.source[el] - images.convolved[el];

	images.convolved.copyTo(images.source);
}

const int8_t getSign(float val) {
	return (val < 0) ? -1 : 1;
}

void Wavelet::LinearNoiseReduction(float threshold, float amount) {

	amount = fabsf(amount - 1);

	/*for (int ch = 0; ch < m_wavelet.channels(); ++ch) {

		WaveletHistogram histogram (m_wavelet, ch);
		float median = histogram.Median();
		histogram = WaveletHistogram(m_wavelet, ch, median);
		threshold *= histogram.MAD() / 0.6745;

		for (float& w : image_channel(m_wavelet, ch)) {
			float val = fabsf(w);
			w = (val < threshold) ? w * amount : getSign(w) * (val - threshold);
		}
	}*/
}

void Wavelet::MedianNoiseReduction(float threshold, float amount) {

	/*amount = fabsf(amount - 1);

	for (int ch = 0; ch < m_wavelet.channels(); ++ch) {

		WaveletHistogram histogram(m_wavelet, ch);
		float median = histogram.Median();
		histogram = WaveletHistogram(m_wavelet, ch, median);
		threshold *= histogram.MAD() / 0.6745;

		for (float& w : image_channel(m_wavelet, ch))
			w = (abs(w) < threshold) ? w * amount : w;

	}*/

}




template<typename T>
ImageVector<float> WaveletLayerCreator::generateWaveletLayers(const Image<T>& src) {

	std::vector<Image32> imgs;
	imgs.reserve(layers());

	Images images(src);

	for (int i = 0; i < layers(); ++i) {
		atrous(i, images);
		images.wavelet.normalize();
		imgs.push_back(images.wavelet);
	}

	if (m_residual)
		imgs.push_back(images.convolved);

	return imgs;
}
template std::vector<Image32> WaveletLayerCreator::generateWaveletLayers(const Image8&);
template std::vector<Image32> WaveletLayerCreator::generateWaveletLayers(const Image16&);
template std::vector<Image32> WaveletLayerCreator::generateWaveletLayers(const Image32&);





//only works on non-normalized wavelet images
double StructureMaps::kSigma(const Image32& img, float K, float eps, int n) {

	std::vector<float> pixels(img.totalPxCount());
	memcpy(&pixels[0], img.data(), img.totalPxCount() * sizeof(float));

	double s0 = 0;
	for (int it = 0; it < n; ++it) {

		if (pixels.size() < 2)
			return 0;

		double s = math::standardDeviation(pixels);

		if (1 + s == 1)
			return 0;
		if (eps > 0 && it > 1 && (s0 - s) / s0 < eps)
			return s;
		s0 = s;

		std::vector<float> other;
		other.reserve(pixels.size());

		double Ks = K * s;

		for (float val : pixels)
			if (abs(val) < Ks)
				other.emplace_back(val);

		std::swap(pixels, other);
	}

	return 0;
}

template<typename T>
Image8Vector StructureMaps::generateMaps(const Image<T>& img) {

	Image8Vector star_imgs;
	star_imgs.reserve(layers());

	Images images(img, true);

	auto& w = images.wavelet;

	for (int i = 0; i < layers(); ++i) {

		atrous(i, images);
		w.normalize();

		float median = w.computeMedian(0);
		float sigma = w.compute_nMAD(0);

		float threshold = median + m_K * sigma;

		Image8 bi_wavelet(img.rows(), img.cols());

		for (int el = 0; el < bi_wavelet.totalPxCount(); ++el)
			bi_wavelet[el] = (w[el] >= threshold) ? 1 : 0;

		star_imgs.emplace_back(std::move(bi_wavelet));
	}

	return star_imgs;
}
template Image8Vector StructureMaps::generateMaps(const Image8&);
template Image8Vector StructureMaps::generateMaps(const Image16&);
template Image8Vector StructureMaps::generateMaps(const Image32&);
