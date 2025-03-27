#include "pch.h"
#include "Wavelet.h"
#include "MorphologicalTransformation.h"
#include "FastStack.h"
#include "FITS.h"

/*float Wavelet::WaveletHistogram::Median() {
	int occurrences = 0;
	int median1 = 0, median2 = 0;
	int medianlength = m_count / 2;

	for (int i = 0; i < m_size; ++i) {
		occurrences += histogram[i];
		if (occurrences > medianlength) {
			median1 = i;
			median2 = i;

			break;
		}
		else if (occurrences == medianlength) {
			median1 = i;
			for (int j = i + 1; j < m_size; ++j) {
				if (histogram[j] > 0) {
					median2 = j;
					break;
				}
			}
			break;
		}
	}

	float med = (median1 + median2) / 2.0;

	if (histogram[med] == 0) {
		float k1 = float(medianlength - (occurrences - histogram[median1])) / histogram[median1];
		float k2 = float(medianlength - occurrences) / histogram[median2];
		med += (k1 + k2) / 2;

		return med / 65535;
	}

	med += float(medianlength - (occurrences - histogram[median1])) / histogram[med];

	return (med - 65535) / 65535;
}

float Wavelet::WaveletHistogram::MAD() {
	return Median() + 1;
}*/

const std::vector<float>& Wavelet::scalingFunctionKernel(ScalingFunction sf) const {
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

void Wavelet::atrous(uint8_t layer) {

	int _2i = pow(2, layer);

	const std::vector<float>& sfv = scalingFunctionKernel(m_scaling_func);

	float sfv_sum = 0;
	for (float v : sfv)
		sfv_sum += v;
	
	int left = -(int(sfv.size() - 1) / 2) * _2i;

	for (int ch = 0; ch < m_source.channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < m_source.rows(); ++y)
			for (int x = 0; x < m_source.cols(); ++x) {

				float sum = 0;

				for (int i = 0, d = left; i < sfv.size(); ++i, d += _2i)
					sum += m_source.at_replicated(x + d, y, ch) * sfv[i];

				m_wavelet(x, y, ch) = sum;
			}

#pragma omp parallel for
		for (int y = 0; y < m_wavelet.rows(); ++y)
			for (int x = 0; x < m_wavelet.cols(); ++x) {

				float sum = 0;

				for (int i = 0, d = left; i < sfv.size(); ++i, d += _2i)
					sum += m_wavelet.at_replicated(x, y + d, ch) * sfv[i];

				m_convolved(x, y, ch) = sum;
			}
	}

	for (int el = 0; el < m_source.totalPxCount(); ++el)
		m_wavelet[el] = m_source[el] - m_convolved[el];

	m_convolved.copyTo(m_source);
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
std::vector<Image32> WaveletLayerCreator::generateWaveletLayers(const Image<T>& src) {

	std::vector<Image32> imgs;
	imgs.reserve(m_layers);

	waveletInit(src);

	for (int i = 0; i < m_layers; ++i) {
		atrous(i);
		m_wavelet.normalize();
		imgs.push_back(m_wavelet);
	}

	if (m_residual)
		imgs.push_back(m_convolved);

	cleanUp();
	return imgs;
}
template std::vector<Image32> WaveletLayerCreator::generateWaveletLayers(const Image8&);
template std::vector<Image32> WaveletLayerCreator::generateWaveletLayers(const Image16&);
template std::vector<Image32> WaveletLayerCreator::generateWaveletLayers(const Image32&);





/*template<typename Image>
void Wavelet::WaveletLayerNR(Image& img, NRVector nrvector, ScalingFunction sf, int scale_num) {
	assert(scale_num <= 4 && nrvector.size() <= scale_num);
	if (!img.Exists())
		return;

	*this = Wavelet(img);

	Image32 result(img.rows(), img.cols(), img.channels());

	for (int i = 0; i < scale_num; ++i) {

		Atrous(i, sf);

		if (i < nrvector.size() && nrvector[i].layer)
			LinearNoiseReduction(nrvector[i].threshold, nrvector[i].amount);

		result += wavelet;
	}

	result += convolved;

	result.truncate(0, 1);

	CopyData(result, img);
}
template void Wavelet::WaveletLayerNR(Image8&, NRVector, ScalingFunction, int);
template void Wavelet::WaveletLayerNR(Image16&, NRVector, ScalingFunction, int);
template void Wavelet::WaveletLayerNR(Image32&, NRVector, ScalingFunction, int);

template<typename Image>
void Wavelet::MultiscaleLinearNR(Image& img, NRVector nrvector, int scale_num) {
	assert(scale_num <= 4 && nrvector.size() <= scale_num);
	if (!img.Exists())
		return;

	*this = Wavelet(img);
	source.copyTo(convolved);

	Image32 result(img.rows(), img.cols(), img.channels());
	GaussianFilter gf;

	for (int i = 0; i < scale_num; ++i) {

		gf.setKernelDimension(2 * int(pow(2, i)) + 1);
		gf.Apply(convolved);

		//GaussianFilter(2 * int(pow(2, i)) + 1).ApplyGaussianBlur(convolved, wavelet);
		GetWaveletLayer();

		if (i < nrvector.size() && nrvector[i].layer)
			LinearNoiseReduction(nrvector[i].threshold, nrvector[i].amount);

		result += wavelet;

	}

	result += convolved;

	result.truncate(0, 1);

	CopyData(result, img);
}
template void Wavelet::MultiscaleLinearNR(Image8&, NRVector, int);
template void Wavelet::MultiscaleLinearNR(Image16&, NRVector, int);
template void Wavelet::MultiscaleLinearNR(Image32&, NRVector, int);

template<typename Image>
void Wavelet::MultiscaleMedianNR(Image& img, NRVector nrvector, int scale_num) {
	assert(scale_num <= 4 && nrvector.size() <= scale_num);
	if (!img.Exists())
		return;

	*this = Wavelet(img);

	Image32 result(img.rows(), img.cols(), img.channels());

	for (int i = 0; i < scale_num; ++i) {

		source.copyTo(convolved);
		
		MorphologicalTransformation mt(2 * (i + 1) + 1);
		mt.setMorphologicalFilter(MorphologicalFilter::median);
		mt.Apply(convolved);
		//ImageOP::Morphology(2 * (i + 1) + 1).Median(convolved);
		GetWaveletLayer();


		if (i < nrvector.size() && nrvector[i].layer)
			MedianNoiseReduction(nrvector[i].threshold, nrvector[i].amount);

		result += wavelet;

	}

	result += convolved;

	result.truncate(0, 1);

	CopyData(result, img);
}
template void Wavelet::MultiscaleMedianNR(Image8&, NRVector, int);
template void Wavelet::MultiscaleMedianNR(Image16&, NRVector, int);
template void Wavelet::MultiscaleMedianNR(Image32&, NRVector, int);*/

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
	star_imgs.reserve(m_layers);

	img.copyTo(m_source);
	m_source.RGBtoGray();

	if (m_median_blur) {
		MorphologicalTransformation mt(3);
		mt.setMorphologicalFilter(MorphologicalFilter::median);
		mt.apply(m_source);
	}

	m_convolved = Image32(img.rows(), img.cols());
	m_wavelet = Image32(img.rows(), img.cols());

	for (int i = 0; i < m_layers; ++i) {

		atrous(i);

		m_wavelet.normalize();

		float median = m_wavelet.computeMedian(0, true);
		float avgdev = 1.4836 * m_wavelet.computeMAD(0, median, true);//m_wavelet.computeAvgDev(0, median, true);

		float threshold = median + 3 * avgdev;

		Image8 bi_wavelet(img.rows(), img.cols());

		for (int el = 0; el < bi_wavelet.totalPxCount(); ++el)
			bi_wavelet[el] = (m_wavelet[el] >= threshold) ? 1 : 0;

		star_imgs.emplace_back(std::move(bi_wavelet));
	}

	cleanUp();
	return star_imgs;
}
template Image8Vector StructureMaps::generateMaps(const Image8&);
template Image8Vector StructureMaps::generateMaps(const Image16&);
template Image8Vector StructureMaps::generateMaps(const Image32&);

