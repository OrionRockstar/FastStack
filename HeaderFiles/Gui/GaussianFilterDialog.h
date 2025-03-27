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
	GaussianFilterDialog(QWidget* parent = nullptr);

private:
	void addSigmaInputs();

	void onValueChanged(float sigma);

	void resetDialog();

	void showPreview();

	void apply();

	void applytoPreview();
};