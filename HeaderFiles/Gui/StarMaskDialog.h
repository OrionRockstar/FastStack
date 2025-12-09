#pragma once
#include "StarMask.h"
#include "CustomWidgets.h"
#include "ProcessDialog.h"


class StarMaskDialog : public ProcessDialog {

	StarMask m_sm;

	DoubleInput* m_sigmaK_input = nullptr;
	DoubleInput* m_roundness_input = nullptr;
	DoubleInput* m_gblur_sigma_input = nullptr;

	ComboBox* m_psf_combo = nullptr;
	DoubleSpinBox* m_beta_sb = nullptr;
	CheckBox* m_real_value_cb = nullptr;

public:
	StarMaskDialog(Workspace* parent);

private:
	void addStarThresholdInputs();

	void addRoundnessInputs();

	void addGaussianBlurInputs();

	void addPSFInputs();

	void resetDialog();

	void showPreview() {}

	void apply();
};