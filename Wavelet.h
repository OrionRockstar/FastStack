#pragma once
#include "Image.h"
#include "ImageOperations.h"
#include "GaussianFilter.h"

struct NRLayers {
	bool layer = false;
	float threshold = 3;
	float amount = 1;

	NRLayers(bool l, float t, float a) :layer(l), threshold(t), amount(a) {};
	NRLayers() = default;
};

typedef std::vector<NRLayers> NRVector;

enum class ScalingFunction {
	linear_3,
	b3spline_5,
	smallscale3_3,
	gaussian_5
};

class Wavelet {

	std::vector<float> linear = { 0.25f,0.5f,0.25f };
	std::vector<float> b3 = { 0.0625f,0.25f,0.375f,0.25f,0.0625f };
	std::vector<float> ss3_3 = { 0.333333f,1.0f,0.333333f };
	std::vector<float> g_5 = { 0.01f,0.316228f,1.0f,0.316228f,0.01f };

	Image32 source;
	Image32 convolved;
	Image32 wavelet;
	ScalingFunction m_sf = ScalingFunction::b3spline_5;

	struct Histogram {
		std::unique_ptr<uint32_t[]> data;
		int m_size = 0;
		int m_count = 0;

		uint32_t& operator[](int val) { return data[val]; }

		//buids full channel histogram
		Histogram(Image32& img, int ch) {
			data = std::make_unique<uint32_t[]>(65535 * 2 + 1);
			m_size = 65535 * 2 + 1;

			for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel) {
				data[65535 * (*pixel + 1)]++;
				m_count++;
			}
		}

		//builds mad histogram
		Histogram(Image32& img, int ch, float median) {
			data = std::make_unique<uint32_t[]>(65536);
			m_size = 65536;

			for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel) {
				data[abs(*pixel - median) * 65535]++;
				m_count++;
			}
		}

		//computes and returns median
		float Median(bool mad = false);

	};

public:

	Wavelet() = default;

private:

	template<typename T>
	Wavelet(Image<T>& img) {
		source = Image32(img.Rows(), img.Cols(), img.Channels());


		if (img.is_float())
			memcpy(source.data.get(), img.data.get(), img.TotalPxCount() * 4);

		else
			for (int el = 0; el < img.TotalPxCount(); ++el)
				source[el] = Pixel<float>::toType(img[el]);

		convolved = Image32(source);
		wavelet = Image32(source);
	}

	std::vector<float> GetScalingFunction(ScalingFunction sf);

	void GetWaveletLayer();

	void Atrous(int scale_num, ScalingFunction sf);

	const int GetSign(float val);

	void LinearNoiseReduction(float threshold = 3, float amount = 1);

	void MedianNoiseReduction(float threshold = 3, float amount = 1);

	void TrinerizeImage(const Image32& input, Image8& output, float threshold);


public:

	template<typename Image>
	void WaveletTransform(Image& img, std::vector<Image32>& wavelet_vector, ScalingFunction sf = ScalingFunction::b3spline_5, int scale_num = 4, bool residual = false);

	template<typename Image>
	void WaveletLayerNR(Image& img, NRVector nrvector, ScalingFunction sf = ScalingFunction::b3spline_5, int scale_num = 4);

	template<typename Image>
	void MultiscaleLinearNR(Image& img, NRVector nrvector, int scale_num);

	template<typename Image>
	void MultiscaleMedianNR(Image& img, NRVector nrvector, int scale_num);

	void B3WaveletTransform_Trinerized(const Image32& img, Image8Vector& wavelet_vector, float thresh_mult = 3, bool median_blur = true, int scale_num = 5);

	void ChrominanceNoiseReduction(Image32& img, int layers_to_remove, int layers_to_keep);
};

