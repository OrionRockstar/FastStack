#pragma once
#include "Image.h"
#include "GaussianFilter.h"
//#include "ProcessDialog.h"

class Wavelet {

public:
	enum class ScalingFunction {
		linear_3,
		smallscale3_3,
		b3spline_5,
		gaussian_5
	};

private:
	const std::vector<float> linear = { 0.25f,0.5f,0.25f };
	const std::vector<float> ss3_3 = { 0.333333f,1.0f,0.333333f };
	const std::vector<float> b3 = { 0.0625f,0.25f,0.375f,0.25f,0.0625f };
	const std::vector<float> g_5 = { 0.01f,0.316228f,1.0f,0.316228f,0.01f };

	ScalingFunction m_scaling_func = ScalingFunction::b3spline_5;
	int m_layers = 5;

	const std::vector<float>& scalingFunctionKernel(ScalingFunction sf) const;

protected:
	struct Images {
		Image32 source;
		Image32 convolved;
		Image32 wavelet;

		template<typename T>
		Images(const Image<T> src, bool to_grayscale = false);
	};

public:
	Wavelet() = default;

	Wavelet(const Wavelet& other) {

		m_scaling_func = other.m_scaling_func;
		m_layers = other.m_layers;
	}

	Wavelet& operator=(const Wavelet& other) {

		if (this != &other) {
			m_scaling_func = other.m_scaling_func;
			m_layers = other.m_layers;
		}

		return *this;
	}

	ScalingFunction scalingFuntion()const { return m_scaling_func; }

	void setScalingFuntion(ScalingFunction scaling_func) { m_scaling_func = scaling_func; }

	int layers()const { return m_layers; }

	void setLayers(int layers) { m_layers = layers; }

protected:
	void atrous(uint8_t layer, Images& images);

	void LinearNoiseReduction(float threshold = 3, float amount = 1);

	void MedianNoiseReduction(float threshold = 3, float amount = 1);

};


class WaveletLayerCreator : public Wavelet {
	bool m_residual = false;

public:
	bool residual()const { return m_residual; }

	void setResidual(bool v) { m_residual = v; }

	template<typename T>
	ImageVector<float> generateWaveletLayers(const Image<T>& src);
};


class StructureMaps : public Wavelet {

	float m_K = 3.0;
	
public:
	StructureMaps() { setLayers(3); };

	float sigmaK()const { return m_K; }

	void setSigmaK(float K) { m_K = K; }

private:
	double kSigma(const Image32& img, float K = 3.0f, float eps = 0.01f, int n = 10);

public:
	template<typename T>
	Image8Vector generateMaps(const Image<T>& img);
};

