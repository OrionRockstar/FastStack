#pragma once
#include "CustomWidgets.h"
#include "ProcessDialog.h"
#include "BilateralFilter.h"

class BilateralFilterDialog : public ProcessDialog {

	BilateralFilter m_bf;

	DoubleLineEdit* m_sigma_s_le = nullptr;
	Slider* m_sigma_s_slider = nullptr;

	DoubleLineEdit* m_sigma_r_le = nullptr;
	Slider* m_sigma_r_slider = nullptr;

	CheckBox* m_circular_cb = nullptr;
	ComboBox* m_kernel_size_cb = nullptr;

public:
	BilateralFilterDialog(QWidget* parent = nullptr);

private:
	void addSigmaInputs();

	void addSigmaIntensityInputs();

	void addKernelSizeInputs();

	void resetDialog();

	void apply();

	void applyPreview();
};