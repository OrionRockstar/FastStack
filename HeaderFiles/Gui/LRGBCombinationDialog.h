#pragma once
#include "LRGBCombination.h"
#include "ProcessDialog.h"



class LRGBCombinationDialog : public ProcessDialog {

	LRGBCombination m_lrgbc;

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

	void apply();

	void applytoPreview() {}
};