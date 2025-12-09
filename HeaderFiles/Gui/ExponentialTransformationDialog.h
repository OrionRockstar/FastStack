#pragma once
#include "ProcessDialog.h"
#include "ExponentialTransformation.h"

class ExponentialTransformationDialog : public ProcessDialog {

	ExponentialTransformation m_et;

	ComboBox* m_method_combo = nullptr;

	DoubleInput* m_order_inputs = nullptr;

	DoubleInput* m_smoothness_inputs = nullptr;

	CheckBox* m_lightness_cb = nullptr;

public:
	ExponentialTransformationDialog(Workspace* parent = nullptr);

private:
	void addMethodInput();

	void addOrderInputs();

	void addSmoothnessInputs();

	void resetDialog();

	void apply();

	void applyPreview();
};

