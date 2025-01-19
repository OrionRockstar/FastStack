#pragma once
#include "Image.h"
#include "ProcessDialog.h"
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

	void enableLum(bool enable) { m_enable_lum = enable; }

	void enableRed(bool enable) { m_cc.enableRed(enable); }

	void enableGreen(bool enable) { m_cc.enableGreen(enable); }

	void enableBlue(bool enable) { m_cc.enableBlue(enable); }


	void setLum(const Image8* lum) { m_L = lum; }

	void setRed(const Image8* red) { m_cc.setRed(red); }

	void setGreen(const Image8* green) { m_cc.setGreen(green); }

	void setBlue(const Image8* blue) { m_cc.setBlue(blue); }


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






class LRGBCombinationDialog : public ProcessDialog {

	LRGBCombination m_lc;

	QToolBox* m_toolbox;

	
	GroupBox* m_image_selection_gb;
	QGridLayout* m_image_selection_layout;

	QPalette m_pal;
	CheckBox* m_lum_cb;
	ComboBox* m_lum_combo;

	CheckBox* m_red_cb;
	ComboBox* m_red_combo;

	CheckBox* m_green_cb;
	ComboBox* m_green_combo;

	CheckBox* m_blue_cb;
	ComboBox* m_blue_combo;

	////

	GroupBox* m_image_weight_gb;
	QGridLayout* m_image_weight_layout;

	DoubleLineEdit* m_lum_le;
	Slider* m_lum_slider;

	DoubleLineEdit* m_red_le;
	Slider* m_red_slider;

	DoubleLineEdit* m_green_le;
	Slider* m_green_slider;

	DoubleLineEdit* m_blue_le;
	Slider* m_blue_slider;

public:
	LRGBCombinationDialog(QWidget* parent);
private:
	void onWindowOpen();

	void onWindowClose();

	void addImageSelection();

	void addLumInputs();

	void addRedInputs();

	void addGreenInputs();

	void addBlueInputs();

	void addImageWeights();

	void addLWeightInputs();

	void addRWeightInputs();

	void addGWeightInputs();

	void addBWeightInputs();

	void addChrominaceNRInputs();

	void resetDialog();

	void showPreview() {}

	void Apply();

	void ApplytoPreview() {}
};