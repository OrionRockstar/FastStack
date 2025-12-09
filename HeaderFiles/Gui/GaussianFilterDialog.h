#pragma once
#include "GaussianFilter.h"
#include "ProcessDialog.h"
#include "CustomWidgets.h"


class GaussianFilterDialog : public ProcessDialog {

	GaussianFilter m_gf;

	DoubleLineEdit* m_sigma_le;
	Slider* m_sigma_slider;
	QString m_ksize = "Kernel Size:   ";
	QLabel* m_ks_label;

public:
	GaussianFilterDialog(Workspace* parent = nullptr);

private:
	void addSigmaInputs();

	void onValueChanged(float sigma);

	void resetDialog();

	void apply();

	void applyPreview();
};