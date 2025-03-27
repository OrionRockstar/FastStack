#pragma once
#include "Image.h"
#include "ChannelCombination.h"

class LRGBCombination {

	bool m_enable_lum = true;
	const Image8* m_L = nullptr;

	ChannelCombination m_cc;

	double m_L_weight = 1.0;
	double m_R_weight = 1.0;
	double m_G_weight = 1.0;
	double m_B_weight = 1.0;

	double m_lightness_mtf = 0.5;
	double m_saturation_mtf = 0.5;

	bool m_reduce_chrominance = false;
	int m_layers_to_remove = 4;
	int m_layers_to_keep = 1;

public:
	LRGBCombination() = default;

	ChannelCombination& channelCombination() { return m_cc; }

	void enableLum(bool enable) { m_enable_lum = enable; }

	void setLum(const Image8* lum) { m_L = lum; }

	double lumWeight()const { return m_L_weight; }

	void setLumWeight(double weight) { m_L_weight = weight; }

	double redWeight()const { return m_R_weight; }

	void setRedWeight(double weight) { m_R_weight = weight; }

	double greenWeight()const { return m_G_weight; }

	void setGreenWeight(double weight) { m_G_weight = weight; }

	double blueWeight()const { return m_B_weight; }

	void setBlueWeight(double weight) { m_B_weight = weight; }


	Status isImagesSameSize()const;
private:
	template<typename T>
	void combineLuminance(Image32& rgb, const Image<T>& lum);

public:
	Image32 generateLRGBImage();
};

