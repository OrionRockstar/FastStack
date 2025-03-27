#pragma once
#include "ProcessDialog.h"
#include "SCNR.h"

class SCNRDialog : public ProcessDialog {

	SCNR m_scnr;

	ComboBox* m_color_combo = nullptr;
	ComboBox* m_protection_combo = nullptr;

	DoubleLineEdit* m_amount_le = nullptr;
	Slider* m_amount_slider = nullptr;

	CheckBox* m_preserve_lightness_cb = nullptr;

public:
	SCNRDialog(QWidget* parent);

private:
	void addAmountInputs();

	void resetDialog();

	void showPreview() {}

	void apply();

	void applytoPreview(){}
};

