#include "pch.h"
#include "Wavelet.h"

float Wavelet::WaveletHistogram::Median() {
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
}

std::vector<float> Wavelet::GetScalingFunction(ScalingFunction sf) {
	using enum ScalingFunction;

	switch (sf) {

	case linear_3:
		return linear;

	case b3spline_5:
		return b3;

	case smallscale3_3:
		return ss3_3;

	case gaussian_5:
		return g_5;

	default:
		return b3;
	}
}

void Wavelet::GetWaveletLayer() {

	for (auto s = source.begin(), c = convolved.begin(), w = wavelet.begin(); s != source.end(); ++s, ++c, ++w)
		*w = *s - *c;

	convolved.CopyTo(source);
}

void Wavelet::Atrous(int scale_num, ScalingFunction sf) {
	int _2i = pow(2, scale_num);

	std::vector<float> sfv = GetScalingFunction(sf);

	float sfv_sum = 0;
	for (float v : sfv)
		sfv_sum += v;

	int left = -(int(sfv.size() - 1) / 2) * _2i;

	for (int ch = 0; ch < source.Channels(); ++ch) {
#pragma omp parallel for
		for (int y = 0; y < source.Rows(); ++y)
			for (int x = 0; x < source.Cols(); ++x) {

				float sum = 0;

				for (int i = 0, d = left; i < sfv.size(); ++i, d += _2i)
					sum += source.MirrorEdgePixel(x + d, y, ch) * sfv[i];

				wavelet(x, y, ch) = sum / sfv_sum;
			}

#pragma omp parallel for
		for (int y = 0; y < wavelet.Rows(); ++y)
			for (int x = 0; x < wavelet.Cols(); ++x) {

				float sum = 0;

				for (int i = 0, d = left; i < sfv.size(); ++i, d += _2i)
					sum += wavelet.MirrorEdgePixel(x, y + d, ch) * sfv[i];

				convolved(x, y, ch) = sum / sfv_sum;
			}
	}

	GetWaveletLayer();

}

const int Wavelet::GetSign(float val) {
	return (val < 0) ? -1 : 1;
}

void Wavelet::LinearNoiseReduction(float threshold, float amount) {

	amount = fabsf(amount - 1);

	for (int ch = 0; ch < wavelet.Channels(); ++ch) {

		WaveletHistogram histogram(wavelet, ch);
		float median = histogram.Median();
		histogram = WaveletHistogram(wavelet, ch, median);
		threshold *= histogram.MAD() / 0.6745;

		for (float& w : image_channel(wavelet, ch)) {
			float val = fabsf(w);
			w = (val < threshold) ? w * amount : GetSign(w) * (val - threshold);
		}
	}
}

void Wavelet::MedianNoiseReduction(float threshold, float amount) {

	amount = fabsf(amount - 1);

	for (int ch = 0; ch < wavelet.Channels(); ++ch) {

		WaveletHistogram histogram(wavelet, ch);
		float median = histogram.Median();
		histogram = WaveletHistogram(wavelet, ch, median);
		threshold *= histogram.MAD() / 0.6745;

		for (float& w : image_channel(wavelet, ch))
			w = (abs(w) < threshold) ? w * amount : w;

	}

}

void Wavelet::TrinerizeImage(const Image32& input, Image8& output, float threshold) {

	for (int el = 0; el < input.Total(); ++el)
		output[el] = (input[el] >= threshold) ? 1 : 0;

	for (int y = 1; y < output.Rows() - 1; ++y)
		for (int x = 1; x < output.Cols() - 1; ++x)
			if (output(x, y) == 1)
				if (output(x - 1, y) != 0 && output(x + 1, y) != 0 && output(x, y - 1) != 0 && output(x, y + 1) != 0) output(x, y) = 2;

}

template<typename Image>
void Wavelet::WaveletTransform(Image& img, std::vector<Image32>& wavelet_vector, ScalingFunction sf, int scale_num, bool residual) {
	if (!img.Exists())
		return;

	*this = Wavelet(img);

	if (residual)
		wavelet_vector.reserve(scale_num + 1);
	else
		wavelet_vector.reserve(scale_num);

	for (int i = 0; i < scale_num; ++i) {

		wavelet = Image32(wavelet.Rows(), wavelet.Cols(), wavelet.Channels());

		Atrous(i, sf);

		wavelet_vector.emplace_back(std::move(wavelet));

	}

	if (residual)
		wavelet_vector.emplace_back(std::move(convolved));

}
template void Wavelet::WaveletTransform(Image8& img, std::vector<Image32>&, ScalingFunction, int, bool);
template void Wavelet::WaveletTransform(Image16& img, std::vector<Image32>&, ScalingFunction, int, bool);
template void Wavelet::WaveletTransform(Image32& img, std::vector<Image32>&, ScalingFunction, int, bool);

//copys data if both src&&dest are float
//otherwise scales fp src to dest respective numerical range 
template<typename B>
static void CopyData(Image32& src, Image<B>& dest) {
	if (dest.is_float())
		memcpy(dest.data.get(), src.data.get(), src.TotalPxCount() * 4);

	else
		for (int el = 0; el < dest.TotalPxCount(); ++el)
			Pixel<float>::fromType(src[el], dest[el]);
}

template<typename Image>
void Wavelet::WaveletLayerNR(Image& img, NRVector nrvector, ScalingFunction sf, int scale_num) {
	assert(scale_num <= 4 && nrvector.size() <= scale_num);
	if (!img.Exists())
		return;

	*this = Wavelet(img);

	Image32 result(img.Rows(), img.Cols(), img.Channels());

	for (int i = 0; i < scale_num; ++i) {

		Atrous(i, sf);

		if (i < nrvector.size() && nrvector[i].layer)
			LinearNoiseReduction(nrvector[i].threshold, nrvector[i].amount);

		result += wavelet;
	}

	result += convolved;

	result.Truncate(0, 1);

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
	source.CopyTo(convolved);

	Image32 result(img.Rows(), img.Cols(), img.Channels());

	for (int i = 0; i < scale_num; ++i) {

		GaussianFilter(2 * int(pow(2, i)) + 1).ApplyGaussianBlur(convolved, wavelet);
		GetWaveletLayer();

		if (i < nrvector.size() && nrvector[i].layer)
			LinearNoiseReduction(nrvector[i].threshold, nrvector[i].amount);

		result += wavelet;

	}

	result += convolved;

	result.Truncate(0, 1);

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

	Image32 result(img.Rows(), img.Cols(), img.Channels());

	for (int i = 0; i < scale_num; ++i) {

		source.CopyTo(convolved);
		ImageOP::Morphology(2 * (i + 1) + 1).Median(convolved);
		GetWaveletLayer();


		if (i < nrvector.size() && nrvector[i].layer)
			MedianNoiseReduction(nrvector[i].threshold, nrvector[i].amount);

		result += wavelet;

	}

	result += convolved;

	result.Truncate(0, 1);

	CopyData(result, img);
}
template void Wavelet::MultiscaleMedianNR(Image8&, NRVector, int);
template void Wavelet::MultiscaleMedianNR(Image16&, NRVector, int);
template void Wavelet::MultiscaleMedianNR(Image32&, NRVector, int);

void Wavelet::B3WaveletTransform_Trinerized(const Image32& img, Image8Vector& wavelet_vector, float thresh_mult, bool median_blur, int scale_num) {
	if (!img.Exists())
		return;

	wavelet_vector.reserve(scale_num);

	source = Image32(img.Rows(), img.Cols(), img.Channels());
	img.CopyTo(source);

	if (source.Channels() == 3)
		source.RGBtoGray();

	if (median_blur)
		ImageOP::Morphology(3).Median(source);

	convolved = Image32(img.Rows(), img.Cols());
	wavelet = Image32(img.Rows(), img.Cols());

	for (int i = 0; i < scale_num; ++i) {

		Atrous(i, ScalingFunction::b3spline_5);

		wavelet.Normalize();
		float median = wavelet.ComputeMedian(0, true);
		float avgdev = wavelet.ComputeAvgDev(0, median, true);

		Image8 tri_wavelet(img.Rows(), img.Cols());
		TrinerizeImage(wavelet, tri_wavelet, median + thresh_mult * avgdev);

		wavelet_vector.emplace_back(std::move(tri_wavelet));
	}
}