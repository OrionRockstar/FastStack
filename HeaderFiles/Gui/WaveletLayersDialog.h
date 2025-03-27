#pragma once
#include "Wavelet.h"
#include "ProcessDialog.h"

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