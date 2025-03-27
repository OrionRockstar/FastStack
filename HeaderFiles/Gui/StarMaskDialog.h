#pragma once
#include "StarMask.h"
#include "CustomWidgets.h"
#include "ProcessDialog.h"


class StarMaskDialog : public ProcessDialog {

	StarMask m_sm;

	SpinBox* m_wavelet_layers_sb = nullptr;
	ComboBox* m_scale_func_combo = nullptr;
	CheckBox* m_median_blur_cb = nullptr;

	DoubleLineEdit* m_sigmaK_le = nullptr;
	Slider* m_sigmaK_slider = nullptr;

	DoubleLineEdit* m_peak_edge_le = nullptr;
	Slider* m_peak_edge_slider = nullptr;

	DoubleLineEdit* m_roundness_le = nullptr;
	Slider* m_roundness_slider = nullptr;

	DoubleLineEdit* m_gblur_sigma_le = nullptr;
	Slider* m_gblur_sigma_slider = nullptr;

	ComboBox* m_psf_combo = nullptr;
	DoubleSpinBox* m_beta_sb = nullptr;
	CheckBox* m_real_value_cb = nullptr;

public:
	StarMaskDialog(QWidget* parent);

private:
	void addWaveletInputs();

	void addThresholdInputs();

	void addPeakEdgeRatioInputs();

	void addRoundnessInputs();

	void addGaussianBlurInputs();

	void addPSFInputs();

	void resetDialog();

	void showPreview() {}

	void apply();
};