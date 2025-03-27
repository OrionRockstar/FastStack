#pragma once
#include"ProcessDialog.h"
#include "ASinhStretch.h"

class ASinhStretchDialog : public ProcessDialog {

	Q_OBJECT

	ASinhStretch m_ash;

	Slider* m_sf_slider;
	DoubleLineEdit* m_sf_le;

	Slider* m_bp_slider;
	DoubleLineEdit* m_bp_le;

	float m_current_bp = 0;
	Slider* m_fine_tune;

	CheckBox* m_rgb_cb;

	PushButton* m_bp_comp = nullptr;

public:
	ASinhStretchDialog(QWidget* parent = nullptr);

private:
	void addStretchFactorInputs();

	void addBlackpointInputs();

	void addFinetuneInputs();

	void computeBlackpoint();

	void resetDialog();

	void showPreview();

	void apply();

	void applytoPreview();
};