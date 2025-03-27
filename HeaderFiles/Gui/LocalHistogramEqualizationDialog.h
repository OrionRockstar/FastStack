#pragma once

#include "LocalHistogramEqualization.h"
#include "CustomWidgets.h"
#include "ProcessDialog.h"



class LocalHistogramEqualizationDialog : public ProcessDialog {
	Q_OBJECT

	LocalHistogramEqualization m_lhe;
	std::unique_ptr<ProgressDialog> m_pd;

	//kernel radius
	IntLineEdit* m_kr_le;
	Slider* m_kr_slider;

	//contrast limit
	DoubleLineEdit* m_cl_le;
	Slider* m_cl_slider;

	//amount
	DoubleLineEdit* m_amount_le;
	Slider* m_amount_slider;

	CheckBox* m_circular;

	ComboBox* m_histogram_resolution;
	std::array<int, 3> m_res = { 8,10,12 };

public:
	LocalHistogramEqualizationDialog(QWidget* parent = nullptr);

private:
signals:
	void finished();

private:
	void addKernelRadiusInputs();

	void addContrastLimitInputs();

	void addAmountInputs();

	void showPreview();

	void resetDialog();

	void apply();

	void applytoPreview();
};