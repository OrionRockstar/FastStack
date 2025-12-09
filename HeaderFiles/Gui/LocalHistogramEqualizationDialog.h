#pragma once

#include "LocalHistogramEqualization.h"
#include "CustomWidgets.h"
#include "ProcessDialog.h"



class LocalHistogramEqualizationDialog : public ProcessDialog {
	Q_OBJECT

	LocalHistogramEqualization m_lhe;

	//kernel radius
	IntegerInput* m_kr_input = nullptr;

	//contrast limit
	DoubleInput* m_contrast_input = nullptr;

	DoubleInput* m_amount_input = nullptr;

	CheckBox* m_circular_cb = nullptr;

	ComboBox* m_hist_res_combo = nullptr;

public:
	LocalHistogramEqualizationDialog(Workspace* parent = nullptr);

private:
	void addKernelRadiusInputs();

	void addContrastLimitInputs();

	void addAmountInputs();

	void resetDialog();

	void apply();

	void applyPreview();
};