#pragma once
#include "Image.h"
#include "ImageOperations.h"
#include "GaussianFilter.h"
#include "ProcessDialog.h"

/*struct NRLayers {
	bool layer = false;
	float threshold = 3;
	float amount = 1;

	NRLayers(bool l, float t, float a) :layer(l), threshold(t), amount(a) {};
	NRLayers() = default;
};

typedef std::vector<NRLayers> NRVector;*/


class Wavelet {

public:
	enum class ScalingFunction {
		linear_3,
		smallscale3_3,
		b3spline_5,
		gaussian_5
	};

protected:
	const std::vector<float> linear = { 0.25f,0.5f,0.25f };
	const std::vector<float> ss3_3 = { 0.333333f,1.0f,0.333333f };
	const std::vector<float> b3 = { 0.0625f,0.25f,0.375f,0.25f,0.0625f };
	const std::vector<float> g_5 = { 0.01f,0.316228f,1.0f,0.316228f,0.01f };

	ScalingFunction m_scaling_func = ScalingFunction::b3spline_5;
	uint8_t m_layers = 5;

private:
	const std::vector<float>& scalingFunctionKernel(ScalingFunction sf) const;

protected:
	Image32 m_source = Image32();
	Image32 m_convolved = Image32();
	Image32 m_wavelet = Image32();

	struct WaveletHistogram {
		std::unique_ptr<uint32_t[]> histogram;
		int m_size = 0;
		int m_count = 0;

		uint32_t& operator[](int val) { return histogram[val]; }

		//buids full channel histogram
		WaveletHistogram(Image32& img, int ch) {
			histogram = std::make_unique<uint32_t[]>(65535 * 2 + 1);
			m_size = 65535 * 2 + 1;

			for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel) {
				histogram[65535 * (*pixel + 1)]++;
				m_count++;
			}
		}

		//builds mad histogram
		WaveletHistogram(Image32& img, int ch, float median) {
			histogram = std::make_unique<uint32_t[]>(65536);
			m_size = 65536;

			for (auto pixel = img.begin(ch); pixel != img.end(ch); ++pixel) {
				histogram[abs(*pixel - median) * 65535]++;
				m_count++;
			}
		}

		//computes and returns median
		float Median();

		float MAD();
	};

public:
	Wavelet() = default;

	Wavelet& operator=(Wavelet&& other) {

		if (this != &other) {
			m_scaling_func = other.m_scaling_func;
			m_layers = other.m_layers;
			m_source = std::move(other.m_source);
			m_convolved = std::move(other.m_convolved);
			m_wavelet = std::move(other.m_wavelet);
		}

		return *this;
	}

protected:
	template<typename T>
	void waveletInit(const Image<T>& img) {
		m_source = Image32(img.rows(), img.cols(), img.channels());

		for (int el = 0; el < img.TotalPxCount(); ++el)
			m_source[el] = Pixel<float>::toType(img[el]);

		m_convolved = Image32(img.rows(), img.cols(), img.channels());
		m_wavelet = Image32(img.rows(), img.cols(), img.channels());
	}

	void cleanUp() {
		m_source = Image32();
		m_convolved = Image32();
		m_wavelet = Image32();
	}

public:
	ScalingFunction scalingFuntion()const { return m_scaling_func; }

	void setScalingFuntion(ScalingFunction scaling_func) { m_scaling_func = scaling_func; }

	uint8_t layers()const { return m_layers; }

	void setLayers(uint8_t layers) { m_layers = layers; }

protected:
	void atrous(uint8_t layer);

	void LinearNoiseReduction(float threshold = 3, float amount = 1);

	void MedianNoiseReduction(float threshold = 3, float amount = 1);

	//template<typename Image>
	//void WaveletLayerNR(Image& img, NRVector nrvector, ScalingFunction sf = ScalingFunction::b3spline_5, int scale_num = 4);

	//template<typename Image>
	//void MultiscaleLinearNR(Image& img, NRVector nrvector, int scale_num);

	//template<typename Image>
	//void MultiscaleMedianNR(Image& img, NRVector nrvector, int scale_num);
};


class WaveletLayerCreator : public Wavelet {
	bool m_residual = false;

public:
	bool residual()const { return m_residual; }

	void setResidual(bool v) { m_residual = v; }

	template<typename T>
	std::vector<Image32> generateWaveletLayers(const Image<T>& src);
};


class StructureMaps : public Wavelet {

	double m_K = 3.0;
	bool m_median_blur = true;

public:
	StructureMaps() = default;

	void setK(double K) { m_K = K; }

	bool medianBlur()const { return m_median_blur; }

	void applyMedianBlur(bool val) { m_median_blur = val; }

private:
	double kSigma(const Image32& img, float K = 3.0f, float eps = 0.01f, int n = 10);

public:
	template<typename T>
	Image8Vector generateMaps(const Image<T>& img);
};





class WaveletLayersDialog : public ProcessDialog {

	WaveletLayerCreator m_wavelet;

	SpinBox* m_layers_sb = nullptr;
	ComboBox* m_scaling_func_combo = nullptr;
	CheckBox* m_residual_cb = nullptr;

public:
	WaveletLayersDialog(QWidget* parent);

private:
	void resetDialog();

	void showPreview() {}

	void apply();
};